#include "localization/visual_odometry.hpp"
#include <opencv2/imgproc.hpp>
#include <opencv2/calib3d.hpp>
#include <cmath>
#include <algorithm>

namespace lambda_brief {

static double getPointNorm(const cv::Point2f& pt) {
    return std::sqrt(pt.x * pt.x + pt.y * pt.y);
}

static double getAngleBetweenVectors(const cv::Point2f& v1, const cv::Point2f& v2) {
    double n1 = getPointNorm(v1);
    double n2 = getPointNorm(v2);
    if (n1 < 1e-6 || n2 < 1e-6) return 0.0;

    double dot = v1.x * v2.x + v1.y * v2.y;
    double cos_val = dot / (n1 * n2);
    cos_val = std::clamp(cos_val, -1.0, 1.0);
    return std::acos(cos_val) * 180.0 / M_PI;
}

static VisualOdometryOptions legacyOptions(FeatureType type, int max_features) {
    VisualOdometryOptions o;
    o.type = type;
    o.max_features = max_features;
    return o;
}

VisualOdometry::VisualOdometry(FeatureType type, int max_features)
    : VisualOdometry(legacyOptions(type, max_features)) {}

VisualOdometry::VisualOdometry(const VisualOdometryOptions& options)
    : opt_(options) {
    if (opt_.type == FeatureType::SIFT) {
        detector_ = cv::SIFT::create(opt_.max_features);
    } else {
        detector_ = cv::ORB::create(opt_.max_features > 0 ? opt_.max_features : 5000);
    }
}

void VisualOdometry::reset() {
    prev_ = Features();
    has_prev_ = false;
}

bool VisualOdometry::validateHomographyGeometry(
    const std::vector<cv::Point2f>& obj,
    const std::vector<cv::Point2f>& scene
) {
    if (obj.size() < 9 || scene.size() < 9) return false;

    // Check for NaN or Inf
    for (const auto& pt : scene) {
        if (std::isnan(pt.x) || std::isnan(pt.y) || std::isinf(pt.x) || std::isinf(pt.y)) {
            return false;
        }
    }

    // 1st Check: Displacement < 0.8 * half diagonal
    double orig_diag = getPointNorm(obj[1] - obj[3]);
    double center_disp = getPointNorm(scene[4] - obj[4]);
    double ratio_disp = center_disp / (orig_diag * 0.5);
    if (ratio_disp > 0.8) {
        return false;
    }

    // 2nd Check: Diagonal sizes must be similar to each other and original
    double diag1 = getPointNorm(scene[1] - scene[3]);
    double diag2 = getPointNorm(scene[0] - scene[2]);
    if (diag1 < 1e-3 || diag2 < 1e-3 || orig_diag < 1e-3) return false;

    double ratio_mod_diag = (diag1 > diag2) ? (diag1 / diag2) : (diag2 / diag1);
    double ratio_diag = (diag1 > diag2) ? (diag1 / orig_diag) : (diag2 / orig_diag);
    if (ratio_mod_diag > 1.4 || std::abs(ratio_diag - 1.0) > 0.4) {
        return false;
    }

    // 3rd Check: Angle between projected axes around 90 degrees (|angle - 90| <= 25)
    double angle = getAngleBetweenVectors(scene[5] - scene[4], scene[6] - scene[4]);
    if (std::abs(angle - 90.0) > 25.0) {
        return false;
    }

    // 4th Check: Ratio between opposed sides <= 1.8
    double a = getPointNorm(scene[1] - scene[0]);
    double b = getPointNorm(scene[2] - scene[1]);
    double c = getPointNorm(scene[3] - scene[2]);
    double d = getPointNorm(scene[0] - scene[3]);
    if (a < 1e-3 || b < 1e-3 || c < 1e-3 || d < 1e-3) return false;

    double ratio_x = std::max(a / c, c / a);
    double ratio_y = std::max(b / d, d / b);
    if (ratio_x > 1.8 || ratio_y > 1.8) {
        return false;
    }

    return true;
}

VisualOdometry::Features VisualOdometry::detect(const cv::Mat& img) const {
    Features f;
    if (img.empty()) return f;
    cv::Mat gray;
    if (img.channels() == 3) cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    else gray = img;
    detector_->detectAndCompute(gray, cv::noArray(), f.kp, f.desc);
    f.size = gray.size();
    return f;
}

std::vector<cv::DMatch> VisualOdometry::match(const Features& curr, const Features& prev) const {
    std::vector<cv::DMatch> good;
    const bool is_float = (opt_.type == FeatureType::SIFT);

    if (opt_.matcher == VOMatcher::LEGACY) {
        // Legacy: query = current frame, train = previous frame.
        std::vector<cv::DMatch> matches;
        if (is_float) {
            cv::FlannBasedMatcher matcher;
            matcher.match(curr.desc, prev.desc, matches);
        } else {
            cv::BFMatcher matcher(cv::NORM_HAMMING, true);
            matcher.match(curr.desc, prev.desc, matches);
        }
        if (matches.size() < 6) return good;
        double min_dist = 100.0;  // OpenCV tutorial heuristic kept for fidelity
        for (const auto& m : matches) min_dist = std::min(min_dist, static_cast<double>(m.distance));
        const double threshold_dist = std::max(2.0 * min_dist, 0.02);
        for (const auto& m : matches) {
            if (m.distance <= threshold_dist) good.push_back(m);
        }
        return good;
    }

    // RATIO: 2-NN + Lowe ratio test (+ optional mutual nearest-neighbour check).
    cv::Ptr<cv::DescriptorMatcher> matcher;
    if (is_float) matcher = cv::makePtr<cv::FlannBasedMatcher>();
    else matcher = cv::makePtr<cv::BFMatcher>(cv::NORM_HAMMING, false);

    std::vector<std::vector<cv::DMatch>> knn;
    matcher->knnMatch(curr.desc, prev.desc, knn, 2);

    std::vector<int> back;  // best curr index for every prev descriptor
    if (opt_.mutual_check) {
        std::vector<cv::DMatch> rev;
        matcher->match(prev.desc, curr.desc, rev);
        back.assign(prev.desc.rows, -1);
        for (const auto& m : rev) back[m.queryIdx] = m.trainIdx;
    }

    good.reserve(knn.size() / 2);
    for (const auto& pair : knn) {
        if (pair.size() < 2) continue;
        const cv::DMatch& best = pair[0];
        if (best.distance >= opt_.ratio * pair[1].distance) continue;
        if (opt_.mutual_check && back[best.trainIdx] != best.queryIdx) continue;
        good.push_back(best);
    }
    return good;
}

std::pair<Pose, bool> VisualOdometry::estimate(const Features& prev, const Features& curr) {
    stats_ = VOStats();
    stats_.keypoints_prev = static_cast<int>(prev.kp.size());
    stats_.keypoints_curr = static_cast<int>(curr.kp.size());

    if (prev.desc.empty() || curr.desc.empty() || prev.kp.size() < 6 || curr.kp.size() < 6) {
        return {Pose(), false};
    }

    const std::vector<cv::DMatch> good_matches = match(curr, prev);
    stats_.matches = static_cast<int>(good_matches.size());
    if (good_matches.size() < 6) {
        return {Pose(), false};
    }

    std::vector<cv::Point2f> pts1, pts2;
    pts1.reserve(good_matches.size());
    pts2.reserve(good_matches.size());
    for (const auto& m : good_matches) {
        pts1.push_back(curr.kp[m.queryIdx].pt);
        pts2.push_back(prev.kp[m.trainIdx].pt);
    }

    // H maps current-frame coordinates into the previous frame.
    const int method = (opt_.robust == VORobust::MAGSAC) ? cv::USAC_MAGSAC : cv::RANSAC;
    cv::Mat inlier_mask;
    cv::Mat H = cv::findHomography(pts1, pts2, method, opt_.reproj_threshold_px, inlier_mask);
    if (H.empty()) {
        return {Pose(), false};
    }
    stats_.inliers = inlier_mask.empty() ? 0 : cv::countNonZero(inlier_mask);

    // 9 canonical reference points on the current image
    const float w = static_cast<float>(curr.size.width);
    const float h = static_cast<float>(curr.size.height);

    std::vector<cv::Point2f> obj_corners(9);
    obj_corners[0] = cv::Point2f(0.0f, 0.0f);
    obj_corners[1] = cv::Point2f(w, 0.0f);
    obj_corners[2] = cv::Point2f(w, h);
    obj_corners[3] = cv::Point2f(0.0f, h);
    obj_corners[4] = cv::Point2f(w * 0.5f, h * 0.5f); // center
    obj_corners[5] = cv::Point2f(w, h * 0.5f);
    obj_corners[6] = cv::Point2f(w * 0.5f, 0.0f);
    obj_corners[7] = cv::Point2f(w * 0.5f + w * 0.25f, h * 0.5f + h * 0.25f);
    obj_corners[8] = cv::Point2f(w * 0.5f - w * 0.25f, h * 0.5f - h * 0.25f);

    std::vector<cv::Point2f> scene_corners(9);
    cv::perspectiveTransform(obj_corners, scene_corners, H);

    stats_.geometry_ok = validateHomographyGeometry(obj_corners, scene_corners);
    const bool enough_inliers = (opt_.min_inliers <= 0) || (stats_.inliers >= opt_.min_inliers);
    const bool is_good = stats_.geometry_ok && enough_inliers;

    Pose motion;
    // Translation / scale / yaw (legacy formulas in DroneRobot.cpp)
    motion.y = scene_corners[4].x - obj_corners[4].x;
    motion.x = -(scene_corners[4].y - obj_corners[4].y);

    double d_scene = getPointNorm(scene_corners[7] - scene_corners[8]);
    double d_obj = getPointNorm(obj_corners[7] - obj_corners[8]);
    motion.z = (d_obj > 1e-4) ? (d_scene / d_obj) : 1.0;
    if (std::abs(motion.z - 1.0) < opt_.scale_deadband) {  // legacy: 0.99 < dz < 1.01 -> 1
        motion.z = 1.0;
    }

    motion.yaw = std::atan2(scene_corners[5].y - scene_corners[4].y, scene_corners[5].x - scene_corners[4].x);

    return {motion, is_good};
}

std::pair<Pose, bool> VisualOdometry::estimateRelativeMotion(
    const cv::Mat& prev_img,
    const cv::Mat& curr_img
) {
    if (prev_img.empty() || curr_img.empty()) {
        return {Pose(), false};
    }
    const Features prev = detect(prev_img);
    const Features curr = detect(curr_img);
    return estimate(prev, curr);
}

std::pair<Pose, bool> VisualOdometry::track(const cv::Mat& curr_img) {
    if (curr_img.empty()) {
        return {Pose(), false};
    }
    Features curr = detect(curr_img);
    std::pair<Pose, bool> result{Pose(), false};
    if (has_prev_) {
        result = estimate(prev_, curr);
    } else {
        stats_ = VOStats();
        stats_.keypoints_curr = static_cast<int>(curr.kp.size());
    }
    prev_ = std::move(curr);
    has_prev_ = true;
    return result;
}

} // namespace lambda_brief
