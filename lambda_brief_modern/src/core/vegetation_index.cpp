#include "core/vegetation_index.hpp"
#include <algorithm>
#include <omp.h>

namespace lambda_brief {

double VegetationIndex::computePixel(uchar nir, uchar reg, uchar red, uchar gre, VegetationIndexType type) {
    double n = static_cast<double>(nir);
    double re = static_cast<double>(reg);
    double r = static_cast<double>(red);
    double g = static_cast<double>(gre);

    double num = 0.0;
    double den = 0.0;

    switch (type) {
        case VegetationIndexType::NDVI:
            num = n - r;
            den = n + r;
            break;
        case VegetationIndexType::NDRE:
            num = n - re;
            den = n + re;
            break;
        case VegetationIndexType::NDVI_RED_RE: {
            double red_re = (r + re) * 0.5;
            num = n - red_re;
            den = n + red_re;
            break;
        }
        case VegetationIndexType::GRVI:
            num = g - r;
            den = g + r;
            break;
    }

    if (std::abs(den) < 1e-6) {
        return 0.0;
    }

    double vi = num / den;
    return std::clamp(vi, -1.0, 1.0);
}

cv::Mat VegetationIndex::computeMatrix(
    const cv::Mat& nir,
    const cv::Mat& reg,
    const cv::Mat& red,
    const cv::Mat& gre,
    VegetationIndexType type
) {
    CV_Assert(nir.rows == red.rows && nir.cols == red.cols);
    const int rows = nir.rows;
    const int cols = nir.cols;

    cv::Mat vi_matrix(rows, cols, CV_64FC1);

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < rows; ++i) {
        const uchar* nir_ptr = nir.ptr<uchar>(i);
        const uchar* reg_ptr = reg.empty() ? nullptr : reg.ptr<uchar>(i);
        const uchar* red_ptr = red.ptr<uchar>(i);
        const uchar* gre_ptr = gre.empty() ? nullptr : gre.ptr<uchar>(i);
        double* vi_ptr = vi_matrix.ptr<double>(i);

        for (int j = 0; j < cols; ++j) {
            uchar n = nir_ptr[j];
            uchar re = reg_ptr ? reg_ptr[j] : 0;
            uchar r = red_ptr[j];
            uchar g = gre_ptr ? gre_ptr[j] : 0;
            vi_ptr[j] = computePixel(n, re, r, g, type);
        }
    }

    return vi_matrix;
}

cv::Mat VegetationIndex::computeInvertedMask8U(const cv::Mat& vi_matrix) {
    const int rows = vi_matrix.rows;
    const int cols = vi_matrix.cols;
    cv::Mat mask(rows, cols, CV_8UC1);

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < rows; ++i) {
        const double* vi_ptr = vi_matrix.ptr<double>(i);
        uchar* out_ptr = mask.ptr<uchar>(i);
        for (int j = 0; j < cols; ++j) {
            // Invert VI: lower vegetation => higher value
            // val_inv = -vi in [-1, 1]
            // mapped linearly: -1 -> 0, +1 -> 255
            double val_inv = -vi_ptr[j];
            double normalized = (val_inv - (-1.0)) / (1.0 - (-1.0)) * 255.0;
            out_ptr[j] = static_cast<uchar>(std::clamp(std::round(normalized), 0.0, 255.0));
        }
    }

    return mask;
}

double VegetationIndex::computeVegetationPercentage(const cv::Mat& vi_matrix, double threshold) {
    const int rows = vi_matrix.rows;
    const int cols = vi_matrix.cols;
    const int total = rows * cols;
    if (total == 0) return 0.0;

    int veg_count = 0;
    #pragma omp parallel for reduction(+:veg_count) schedule(static)
    for (int i = 0; i < rows; ++i) {
        const double* vi_ptr = vi_matrix.ptr<double>(i);
        for (int j = 0; j < cols; ++j) {
            if (vi_ptr[j] >= threshold) {
                veg_count++;
            }
        }
    }

    return static_cast<double>(veg_count) / static_cast<double>(total);
}

} // namespace lambda_brief
