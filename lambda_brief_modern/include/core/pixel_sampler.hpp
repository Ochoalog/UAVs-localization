#pragma once

#include "core/types.hpp"
#include <opencv2/core.hpp>
#include <vector>
#include <random>

namespace lambda_brief {

struct PixelPair {
    cv::Point pt1;
    cv::Point pt2;
};

class PixelSampler {
public:
    explicit PixelSampler(unsigned int seed = 0);

    /**
     * @brief Sample pixel pairs using stochastic acceptance roulette wheel (Paper Sec. IV-C)
     * @param combined_mask Probability density map \hat{\vartheta}
     * @param max_fitness Maximum value in combined_mask
     * @param num_pairs Number of pairs to sample (default 256)
     * @return std::vector<PixelPair> of size num_pairs
     */
    std::vector<PixelPair> sampleRoulette(
        const cv::Mat& combined_mask,
        double max_fitness,
        size_t num_pairs = 256
    );

    /**
     * @brief Sample pixel pairs using classical Gaussian distribution centered on image
     */
    std::vector<PixelPair> sampleGaussian(
        const cv::Size& img_size,
        size_t num_pairs = 256
    );

    /**
     * @brief Sample pixel pairs uniformly
     */
    std::vector<PixelPair> sampleUniform(
        const cv::Size& img_size,
        size_t num_pairs = 256
    );

    void setSeed(unsigned int seed);

private:
    std::mt19937 rng_;
};

} // namespace lambda_brief
