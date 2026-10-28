#pragma once

#include "core/types.hpp"
#include "core/pixel_sampler.hpp"
#include <opencv2/core.hpp>
#include <vector>
#include <cstdint>

namespace lambda_brief {

/**
 * @brief How the per-pair map weights enter the distance (v3, --stability_map).
 *  NORMALIZED  d = sum(w_p * e_p) / sum(w_p)            (weighted mean of the pair errors)
 *  COIN        d = sum(w_p * e_p + (1 - w_p) * 0.5) / P  (an unreliable pair counts as a coin flip)
 * e_p = fraction of the pair's bits that disagree (1 for a pair outside the map, as today);
 * w_p = min(w(A), w(B)) read from the weight map at the two projected points (outside: 1).
 * With every weight equal to 1 both reduce to the plain normalised Hamming distance.
 */
enum class StabilityFormula { NORMALIZED, COIN };

struct PairWeightMap {
    const cv::Mat* map = nullptr;  // CV_8U, same size as the satellite map, 255 = weight 1
    StabilityFormula formula = StabilityFormula::NORMALIZED;
};

class Descriptor {
public:
    /**
     * @brief Convert image into descriptor operating color space (e.g. CIE L*a*b* or Grayscale)
     */
    static cv::Mat convertColorSpace(const cv::Mat& bgr, DescriptorColorMode mode);

    /**
     * @brief Uniform per-channel colour quantisation used by the legacy code
     *        (DroneRobot::quantization, "-mappp 1", 25 levels) on BOTH the satellite
     *        map and every drone RGB frame before the CIELAB conversion.
     *        v' = floor(v * L / 255) * (255 / L). levels <= 0 returns a copy.
     */
    static cv::Mat quantizeColors(const cv::Mat& bgr, int levels);

    /**
     * @brief Extract binary descriptor vector from drone image
     * Converts image to CIE L*a*b* and evaluates binary comparisons
     * on channels a* and b* for each pair.
     * @param img_bgr Drone RGB/BGR image
     * @param pairs Sampled pixel pairs
     * @param mode Descriptor color mode (default LAB_NO_L)
     * @return Binary descriptor vector (values 0 or 1, size = pairs.size() * bits_per_pair)
     */
    static std::vector<uint8_t> computeDroneDescriptor(
        const cv::Mat& img_bgr,
        const std::vector<PixelPair>& pairs,
        DescriptorColorMode mode = DescriptorColorMode::LAB_NO_L
    );

    /**
     * @brief Project pixel pairs and evaluate expected descriptor from satellite map for a particle
     * @param map_bgr Global satellite map in BGR (or pre-converted to Lab)
     * @param particle_pose Pose of the particle (x, y, scale z, yaw)
     * @param drone_size Size (width, height) of the drone image
     * @param pairs Pixel pairs in drone image coordinates
     * @param mode Descriptor color mode (default LAB_NO_L)
     * @return Expected binary descriptor vector (contains -1 for out-of-bounds pixels)
     */
    static std::vector<int8_t> computeExpectedDescriptor(
        const cv::Mat& map_bgr,
        const Pose& particle_pose,
        const cv::Size& drone_size,
        const std::vector<PixelPair>& pairs,
        DescriptorColorMode mode = DescriptorColorMode::LAB_NO_L
    );

    /**
     * @brief Computes normalized Hamming distance between observed and expected descriptors
     * @param drone_desc Observed descriptor from drone
     * @param map_desc Expected descriptor from map
     * @return Normalized distance in [0.0, 1.0]
     */
    static double computeHammingDistance(
        const std::vector<uint8_t>& drone_desc,
        const std::vector<int8_t>& map_desc
    );

    /**
     * @brief Direct evaluation of normalized Hamming distance for a particle on the map
     */
    static double evaluateParticleDifference(
        const cv::Mat& map_converted,
        const Pose& particle_pose,
        const cv::Size& drone_size,
        const std::vector<PixelPair>& pairs,
        const std::vector<uint8_t>& drone_descriptor,
        DescriptorColorMode mode = DescriptorColorMode::LAB_NO_L
    );

    /**
     * @brief Same as evaluateParticleDifference, with every pair weighted by the map stability
     *        at the points where it lands (v3). weights == nullptr or weights->map == nullptr
     *        falls back to evaluateParticleDifference (bit-identical).
     */
    static double evaluateWeightedParticleDifference(
        const cv::Mat& map_converted,
        const Pose& particle_pose,
        const cv::Size& drone_size,
        const std::vector<PixelPair>& pairs,
        const std::vector<uint8_t>& drone_descriptor,
        DescriptorColorMode mode,
        const PairWeightMap* weights
    );

    /** One pixel pair projected on the map at a pose (diagnostics / videos, --save_pairs). */
    struct PairDetail {
        cv::Point m1, m2;   // map pixels of the two points
        bool valid{false};  // both points inside the map
        double e{1.0};      // fraction of the pair's bits that disagree with the drone (1 if invalid)
        double w{1.0};      // stability weight min(W(A), W(B)) in [0, 1] (1 without a weight map)
    };

    /** Same projection as evaluateWeightedParticleDifference, but returns every pair. Read-only. */
    static std::vector<PairDetail> pairDetails(
        const cv::Mat& map_converted,
        const Pose& pose,
        const cv::Size& drone_size,
        const std::vector<PixelPair>& pairs,
        const std::vector<uint8_t>& drone_descriptor,
        DescriptorColorMode mode,
        const PairWeightMap* weights
    );
};

} // namespace lambda_brief
