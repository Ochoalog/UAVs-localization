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

    /**
     * @brief Pattern of the legacy abBRIEF (phir2frameworkv2, BriefHeuristic::updateDroneDescriptor(cv::Mat&),
     *        BriefHeuristic.cpp:1132-1154): same generator (cv::RNG in its default state), same
     *        expressions and the same rejection loop.
     *
     * The legacy code creates a local `cv::RNG rng;` (default state 0xFFFFFFFF) on every call and draws each
     * point with rng.gaussian(S*S/25) + S/2, rejecting points outside the image. cv::RNG::gaussian takes the
     * standard deviation, so the variance S^2/25 is used as sigma (~61,600 px for S = 1241): the accepted
     * points are practically uniform over the image, and the pattern is the same on every frame and run.
     * Independent of the seed.
     */
    static std::vector<PixelPair> sampleLegacyAbBrief(
        const cv::Size& img_size,
        size_t num_pairs = 256
    );

    void setSeed(unsigned int seed);

private:
    std::mt19937 rng_;
};

} // namespace lambda_brief
