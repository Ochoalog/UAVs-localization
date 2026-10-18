#pragma once

#include "core/types.hpp"
#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>
#include <utility>
#include <vector>
#include <memory>

namespace lambda_brief {

enum class FeatureType {
    SIFT,
    ORB
};

/// How putative correspondences are filtered before the robust homography fit.
enum class VOMatcher {
    LEGACY,  ///< 1-NN FLANN + "dist <= max(2*min_dist, 0.02)" (OpenCV tutorial heuristic used by
             ///< DroneRobot::findOdometryUsingFeaturesSift). Keeps ~50 of ~15k matches on ADM/0036.
    RATIO    ///< 2-NN + Lowe ratio test + mutual (cross) check. Keeps thousands of matches.
};

enum class VORobust {
    RANSAC,  ///< cv::RANSAC (legacy)
    MAGSAC   ///< cv::USAC_MAGSAC (OpenCV >= 4.5)
};

struct VisualOdometryOptions {
    FeatureType type{FeatureType::SIFT};
    int max_features{0};              ///< 0 = unlimited (legacy SIFT::create())
    VOMatcher matcher{VOMatcher::LEGACY};
    double ratio{0.75};               ///< Lowe ratio (RATIO matcher only)
    bool mutual_check{true};          ///< cross-check (RATIO matcher only)
    VORobust robust{VORobust::RANSAC};
    double reproj_threshold_px{3.0};  ///< homography inlier threshold (OpenCV default 3 px)
    int min_inliers{0};               ///< >0: transitions with fewer homography inliers are unreliable
    double scale_deadband{0.01};      ///< |dz-1| < deadband -> dz = 1 (legacy 0.01; >= 1 ignores VO scale)
};

/// Diagnostics of the last estimate (for LB_DEBUG_ODOM / logs).
struct VOStats {
    int keypoints_prev{0};
    int keypoints_curr{0};
    int matches{0};
    int inliers{0};
    bool geometry_ok{false};
};

class VisualOdometry {
public:
    explicit VisualOdometry(FeatureType type = FeatureType::SIFT, int max_features = 2000);
    explicit VisualOdometry(const VisualOdometryOptions& options);

    /**
     * @brief Estimate relative motion between two consecutive images (stateless; detects
     *        features on BOTH images). Kept for tests / tools.
     * @return (dx, dy, dz_scale, dyaw) in px of prev_img (x = forward/up, y = right) and whether
     *         the homography passed the 4 legacy geometric checks (+ min_inliers if set).
     */
    std::pair<Pose, bool> estimateRelativeMotion(const cv::Mat& prev_img, const cv::Mat& curr_img);

    /**
     * @brief Streaming interface: detects features on curr_img only, reusing the ones cached from
     *        the previous call (halves the SIFT cost: ~0.6 s per 1241x913 frame).
     * @return motion prev -> curr; on the first call (no previous frame) returns {Pose(), false}
     *         and hasPrevious() becomes true.
     */
    std::pair<Pose, bool> track(const cv::Mat& curr_img);

    /// Forget the cached previous frame (next track() call only primes the cache).
    void reset();
    bool hasPrevious() const { return has_prev_; }

    void setScaleDeadband(double deadband) { opt_.scale_deadband = deadband; }
    const VisualOdometryOptions& options() const { return opt_; }
    const VOStats& lastStats() const { return stats_; }

private:
    struct Features {
        std::vector<cv::KeyPoint> kp;
        cv::Mat desc;
        cv::Size size;
    };

    VisualOdometryOptions opt_;
    cv::Ptr<cv::Feature2D> detector_;
    Features prev_;
    bool has_prev_{false};
    VOStats stats_;

    Features detect(const cv::Mat& img) const;
    std::vector<cv::DMatch> match(const Features& curr, const Features& prev) const;
    std::pair<Pose, bool> estimate(const Features& prev, const Features& curr);

    static bool validateHomographyGeometry(
        const std::vector<cv::Point2f>& obj_corners,
        const std::vector<cv::Point2f>& scene_corners
    );
};

} // namespace lambda_brief
