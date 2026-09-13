#pragma once

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

namespace lambda_brief {

class MaskGenerator {
public:
    /**
     * @brief Generate centered 2D Gaussian mask N normalized to [0, 1]
     * @param size Dimensions (width, height)
     * @param sigma_x Standard deviation in X (default width / 5.0)
     * @param sigma_y Standard deviation in Y (default height / 5.0)
     * @return cv::Mat of type CV_64FC1 with values in [0.0, 1.0]
     */
    static cv::Mat generateGaussianMask(
        const cv::Size& size,
        double sigma_x = -1.0,
        double sigma_y = -1.0
    );

    /**
     * @brief Creates the equalized inverted vegetation mask in [0, 1]
     * Performs histogram equalization on the inverted 8-bit mask and normalizes to [0.0, 1.0]
     */
    static cv::Mat generateEqualizedVegetationMask(const cv::Mat& inverted_mask_8u);

    /**
     * @brief Combines vegetation mask and Gaussian mask using alpha (Paper Eq. 1)
     * theta_hat = alpha * theta_inv_equalized + (1 - alpha) * N
     * @param veg_mask_norm Normalized equalized vegetation mask [0, 1]
     * @param gaussian_mask Normalized centered Gaussian mask [0, 1]
     * @param alpha Weighting factor in [0.0, 1.0]
     * @param out_max_fitness Output pointer to receive the maximum probability density
     * @return cv::Mat of type CV_64FC1
     */
    static cv::Mat combineMasks(
        const cv::Mat& veg_mask_norm,
        const cv::Mat& gaussian_mask,
        double alpha,
        double* out_max_fitness = nullptr
    );
};

} // namespace lambda_brief
