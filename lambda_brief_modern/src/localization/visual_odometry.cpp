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

VisualOdometry::VisualOdometry(FeatureType type, int max_features)
    : type_(type), max_features_(max_features) {
    if (type_ == FeatureType::SIFT) {
        detector_ = cv::SIFT::create(max_features_);
    } else {
        detector_ = cv::ORB::create(max_features_);
    }
}

void VisualOdometry::reset() {
    // Stateless per pair
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

std::pair<Pose, bool> VisualOdometry::estimateRelativeMotion(
    const cv::Mat& prev_img,
    const cv::Mat& curr_img
) {
    if (prev_img.empty() || curr_img.empty()) {
        return {Pose(), false};
    }

    cv::Mat g_prev, g_curr;
    if (prev_img.channels() == 3) cv::cvtColor(prev_img, g_prev, cv::COLOR_BGR2GRAY);
    else g_prev = prev_img;
    if (curr_img.channels() == 3) cv::cvtColor(curr_img, g_curr, cv::COLOR_BGR2GRAY);
    else g_curr = curr_img;

    std::vector<cv::KeyPoint> kp1, kp2;
    cv::Mat desc1, desc2;

    detector_->detectAndCompute(g_curr, cv::noArray(), kp1, desc1);
    detector_->detectAndCompute(g_prev, cv::noArray(), kp2, desc2);

    if (desc1.empty() || desc2.empty() || kp1.size() < 6 || kp2.size() < 6) {
        return {Pose(), false};
    }

    std::vector<cv::DMatch> matches;
    if (type_ == FeatureType::SIFT) {
        cv::FlannBasedMatcher matcher;
        matcher.match(desc1, desc2, matches);
    } else {
        cv::BFMatcher matcher(cv::NORM_HAMMING, true);
        matcher.match(desc1, desc2, matches);
    }

    if (matches.size() < 6) {
        return {Pose(), false};
    }

    double min_dist = 100.0;
    for (const auto& m : matches) {
        if (m.distance < min_dist) min_dist = m.distance;
    }

    std::vector<cv::DMatch> good_matches;
    double threshold_dist = std::max(2.0 * min_dist, 0.02);
    for (const auto& m : matches) {
        if (m.distance <= threshold_dist) {
            good_matches.push_back(m);
        }
    }

    if (good_matches.size() < 6) {
        return {Pose(), false};
    }

    std::vector<cv::Point2f> pts1, pts2;
    for (const auto& m : good_matches) {
        pts1.push_back(kp1[m.queryIdx].pt);
        pts2.push_back(kp2[m.trainIdx].pt);
    }

    cv::Mat H = cv::findHomography(pts1, pts2, cv::RANSAC);
    if (H.empty()) {
        return {Pose(), false};
    }

    // Define 9 canonical reference points on curr image
    const float w = static_cast<float>(g_curr.cols);
    const float h = static_cast<float>(g_curr.rows);

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

    bool is_good = validateHomographyGeometry(obj_corners, scene_corners);

    Pose motion;
    // Compute approximate translation, scale, and yaw (legacy formulas in DroneRobot.cpp)
    motion.y = scene_corners[4].x - obj_corners[4].x;
    motion.x = -(scene_corners[4].y - obj_corners[4].y);

    double d_scene = getPointNorm(scene_corners[7] - scene_corners[8]);
    double d_obj = getPointNorm(obj_corners[7] - obj_corners[8]);
    motion.z = (d_obj > 1e-4) ? (d_scene / d_obj) : 1.0;
    if (motion.z > 0.99 && motion.z < 1.01) {
        motion.z = 1.0;
    }

    motion.yaw = std::atan2(scene_corners[5].y - scene_corners[4].y, scene_corners[5].x - scene_corners[4].x);

    return {motion, is_good};
}

} // namespace lambda_brief
