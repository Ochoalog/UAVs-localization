#include "core/pixel_sampler.hpp"
#include <chrono>
#include <algorithm>

namespace lambda_brief {

PixelSampler::PixelSampler(unsigned int seed) {
    if (seed == 0) {
        seed = static_cast<unsigned int>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count()
        );
    }
    rng_.seed(seed);
}

void PixelSampler::setSeed(unsigned int seed) {
    rng_.seed(seed);
}

std::vector<PixelPair> PixelSampler::sampleRoulette(
    const cv::Mat& combined_mask,
    double max_fitness,
    size_t num_pairs
) {
    CV_Assert(combined_mask.type() == CV_64FC1);
    const int cols = combined_mask.cols;
    const int rows = combined_mask.rows;

    std::uniform_int_distribution<int> distX(0, cols - 1);
    std::uniform_int_distribution<int> distY(0, rows - 1);
    std::uniform_real_distribution<double> distProb(0.0, 1.0);

    const double inv_max_fitness = (max_fitness > 1e-9) ? (1.0 / max_fitness) : 1.0;

    std::vector<PixelPair> pairs(num_pairs);

    for (size_t p = 0; p < num_pairs; ++p) {
        cv::Point pts[2];
        for (int i = 0; i < 2; ++i) {
            while (true) {
                int x = distX(rng_);
                int y = distY(rng_);
                double fitness = combined_mask.at<double>(y, x);
                double r = distProb(rng_);
                if (r < (fitness * inv_max_fitness)) {
                    pts[i] = cv::Point(x, y);
                    break;
                }
            }
        }
        pairs[p].pt1 = pts[0];
        pairs[p].pt2 = pts[1];
    }

    return pairs;
}

std::vector<PixelPair> PixelSampler::sampleGaussian(
    const cv::Size& img_size,
    size_t num_pairs
) {
    const double mean_x = img_size.width / 2.0;
    const double mean_y = img_size.height / 2.0;
    const double sigma_x = img_size.width / 5.0;
    const double sigma_y = img_size.height / 5.0;

    std::normal_distribution<double> distX(mean_x, sigma_x);
    std::normal_distribution<double> distY(mean_y, sigma_y);

    std::vector<PixelPair> pairs(num_pairs);

    for (size_t p = 0; p < num_pairs; ++p) {
        cv::Point pts[2];
        for (int i = 0; i < 2; ++i) {
            int x = std::clamp(static_cast<int>(std::round(distX(rng_))), 0, img_size.width - 1);
            int y = std::clamp(static_cast<int>(std::round(distY(rng_))), 0, img_size.height - 1);
            pts[i] = cv::Point(x, y);
        }
        pairs[p].pt1 = pts[0];
        pairs[p].pt2 = pts[1];
    }

    return pairs;
}

std::vector<PixelPair> PixelSampler::sampleUniform(
    const cv::Size& img_size,
    size_t num_pairs
) {
    std::uniform_int_distribution<int> distX(0, img_size.width - 1);
    std::uniform_int_distribution<int> distY(0, img_size.height - 1);

    std::vector<PixelPair> pairs(num_pairs);

    for (size_t p = 0; p < num_pairs; ++p) {
        pairs[p].pt1 = cv::Point(distX(rng_), distY(rng_));
        pairs[p].pt2 = cv::Point(distX(rng_), distY(rng_));
    }

    return pairs;
}

} // namespace lambda_brief
