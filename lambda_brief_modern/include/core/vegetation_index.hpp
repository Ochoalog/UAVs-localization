#pragma once

#include "core/types.hpp"
#include <opencv2/core.hpp>

namespace lambda_brief {

class VegetationIndex {
public:
    /**
     * @brief Calculate spectral index for a single pixel
     */
    static double computePixel(uchar nir, uchar reg, uchar red, uchar gre, VegetationIndexType type);

    /**
     * @brief Compute full dense vegetation index matrix for multispectral bands
     * @return cv::Mat of type CV_64FC1 with values in range [-1.0, 1.0]
     */
    static cv::Mat computeMatrix(
        const cv::Mat& nir,
        const cv::Mat& reg,
        const cv::Mat& red,
        const cv::Mat& gre,
        VegetationIndexType type = VegetationIndexType::NDVI
    );

    /**
     * @brief Computes inverted vegetation matrix scaled to 8-bit [0, 255]
     * Inverted so that lower vegetation has higher probability values:
     * -VI in [-1, 1] normalized to [0, 255]
     */
    static cv::Mat computeInvertedMask8U(const cv::Mat& vi_matrix);

    /**
     * @brief Computes vegetation percentage in the image (pixels with VI >= threshold)
     */
    static double computeVegetationPercentage(const cv::Mat& vi_matrix, double threshold = 0.3);
};

} // namespace lambda_brief
