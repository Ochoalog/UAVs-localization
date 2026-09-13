#include "core/mask_generator.hpp"
#include <cmath>
#include <algorithm>
#include <omp.h>

namespace lambda_brief {

cv::Mat MaskGenerator::generateGaussianMask(
    const cv::Size& size,
    double sigma_x,
    double sigma_y
) {
    if (sigma_x <= 0.0) sigma_x = size.width / 5.0;
    if (sigma_y <= 0.0) sigma_y = size.height / 5.0;

    const int x0 = size.width / 2;
    const int y0 = size.height / 2;

    cv::Mat mask(size, CV_64FC1);
    const double denom = 2.0 * sigma_x * sigma_y;

    #pragma omp parallel for schedule(static)
    for (int y = 0; y < size.height; ++y) {
        double* row_ptr = mask.ptr<double>(y);
        double y_diff = y - y0;
        double y_term = (y_diff * y_diff) / denom;

        for (int x = 0; x < size.width; ++x) {
            double x_diff = x - x0;
            double x_term = (x_diff * x_diff) / denom;
            row_ptr[x] = std::exp(-(x_term + y_term));
        }
    }

    cv::normalize(mask, mask, 0.0, 1.0, cv::NORM_MINMAX);
    return mask;
}

cv::Mat MaskGenerator::generateEqualizedVegetationMask(const cv::Mat& inverted_mask_8u) {
    cv::Mat equalized_8u;
    cv::equalizeHist(inverted_mask_8u, equalized_8u);

    cv::Mat mask_norm(equalized_8u.size(), CV_64FC1);
    const int rows = equalized_8u.rows;
    const int cols = equalized_8u.cols;

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < rows; ++i) {
        const uchar* in_ptr = equalized_8u.ptr<uchar>(i);
        double* out_ptr = mask_norm.ptr<double>(i);
        for (int j = 0; j < cols; ++j) {
            out_ptr[j] = static_cast<double>(in_ptr[j]) / 255.0;
        }
    }

    return mask_norm;
}

cv::Mat MaskGenerator::combineMasks(
    const cv::Mat& veg_mask_norm,
    const cv::Mat& gaussian_mask,
    double alpha,
    double* out_max_fitness
) {
    CV_Assert(veg_mask_norm.size() == gaussian_mask.size());
    const int rows = veg_mask_norm.rows;
    const int cols = veg_mask_norm.cols;

    cv::Mat combined(rows, cols, CV_64FC1);
    double max_val = -1.0;

    #pragma omp parallel for reduction(max:max_val) schedule(static)
    for (int i = 0; i < rows; ++i) {
        const double* v_ptr = veg_mask_norm.ptr<double>(i);
        const double* g_ptr = gaussian_mask.ptr<double>(i);
        double* c_ptr = combined.ptr<double>(i);

        for (int j = 0; j < cols; ++j) {
            double val = alpha * v_ptr[j] + (1.0 - alpha) * g_ptr[j];
            c_ptr[j] = val;
            if (val > max_val) {
                max_val = val;
            }
        }
    }

    if (out_max_fitness) {
        *out_max_fitness = (max_val > 0.0) ? max_val : 1.0;
    }

    return combined;
}

} // namespace lambda_brief
