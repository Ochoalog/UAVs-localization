#pragma once

#include "core/types.hpp"
#include "core/pixel_sampler.hpp"
#include <opencv2/core.hpp>
#include <vector>
#include <cstdint>

namespace lambda_brief {

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
};

} // namespace lambda_brief
