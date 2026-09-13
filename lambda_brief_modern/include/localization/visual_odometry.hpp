#pragma once

#include "core/types.hpp"
#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>
#include <utility>
#include <memory>

namespace lambda_brief {

enum class FeatureType {
    SIFT,
    ORB
};

class VisualOdometry {
public:
    explicit VisualOdometry(FeatureType type = FeatureType::SIFT, int max_features = 2000);

    /**
     * @brief Estimate relative motion between two consecutive images
     * @param prev_img Previous frame (grayscale or BGR)
     * @param curr_img Current frame (grayscale or BGR)
     * @return std::pair<Pose, bool> where Pose has relative (dx, dy, dz_scale, dyaw)
     *         and bool indicates if odometry passed the 4 geometric validity checks.
     */
    std::pair<Pose, bool> estimateRelativeMotion(
        const cv::Mat& prev_img,
        const cv::Mat& curr_img
    );

    /**
     * @brief Reset internal state / previous frame cache
     */
    void reset();

private:
    FeatureType type_;
    int max_features_;
    cv::Ptr<cv::Feature2D> detector_;

    bool validateHomographyGeometry(
        const std::vector<cv::Point2f>& obj_corners,
        const std::vector<cv::Point2f>& scene_corners
    );
};

} // namespace lambda_brief
