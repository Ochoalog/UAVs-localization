#pragma once

#include <cmath>
#include <string>
#include <vector>
#include <iostream>
#include <opencv2/core.hpp>

namespace lambda_brief {

/**
 * @brief 4-DoF / 6-DoF Pose representation for UAV localization
 * Coordinates (x, y) are in map pixel space.
 * z represents the scale factor relative to reference map resolution.
 * yaw is in radians [-pi, pi].
 * roll and pitch are in radians.
 */
struct Pose {
    double x{0.0};
    double y{0.0};
    double z{1.0};
    double roll{0.0};
    double pitch{0.0};
    double yaw{0.0};

    Pose() = default;
    Pose(double x_, double y_, double z_, double yaw_)
        : x(x_), y(y_), z(z_), yaw(yaw_) {}
    Pose(double x_, double y_, double z_, double roll_, double pitch_, double yaw_)
        : x(x_), y(y_), z(z_), roll(roll_), pitch(pitch_), yaw(yaw_) {}

    void normalizeYaw() {
        while (yaw > M_PI) yaw -= 2.0 * M_PI;
        while (yaw < -M_PI) yaw += 2.0 * M_PI;
    }
};

inline std::ostream& operator<<(std::ostream& os, const Pose& p) {
    os << "Pose(x=" << p.x << ", y=" << p.y << ", z=" << p.z
       << ", yaw=" << p.yaw * 180.0 / M_PI << " deg)";
    return os;
}

/**
 * @brief MCL Particle structure
 */
struct Particle {
    Pose pose;
    double weight{1.0};

    Particle() = default;
    Particle(const Pose& p, double w = 1.0) : pose(p), weight(w) {}
};

/**
 * @brief Supported vegetation indices
 */
enum class VegetationIndexType {
    NDVI = 1,        // (NIR - RED) / (NIR + RED)
    NDRE = 2,        // (NIR - REG) / (NIR + REG)
    NDVI_RED_RE = 3, // (NIR - (RED + REG)/2) / (NIR + (RED + REG)/2)
    GRVI = 4         // (GRE - RED) / (GRE + RED)
};

/**
 * @brief Descriptor color space evaluation mode
 */
enum class DescriptorColorMode {
    GRAY = 0,
    LAB = 1,
    LAB_NO_L = 4  // Legacy default: CIE L*a*b* using only a* and b* channels
};

/**
 * @brief Pixel distribution method for sampling BRIEF pairs
 */
enum class PixelSamplerMethod {
    ROULETTE_STOCHASTIC = 2,  // Lipowski & Lipowska (2012) stochastic acceptance
    GAUSSIAN = 11,            // Classical BRIEF Gaussian sampling
    UNIFORM = 0               // Uniform spatial sampling
};

/**
 * @brief Image pre-processing filter
 */
enum class ImagePreprocessFilter {
    NONE = 0,
    GAUSSIAN = 1,
    BILATERAL = 2
};

/**
 * @brief Ground Truth sample from trajectory files
 */
struct GroundTruthSample {
    double x{0.0};       // map pixel x
    double y{0.0};       // map pixel y
    double z{1.0};       // scale factor or altitude in meters
    double yaw_deg{0.0}; // yaw in degrees
    double yaw_rad{0.0}; // yaw in radians
};

/**
 * @brief Pose estimation state summary at each timestep
 */
struct EstimationState {
    size_t frame_index{0};
    Pose mean_pose;
    Pose variance;
    double neff{0.0};
    double elapsed_seconds{0.0};
    double position_error_m{0.0};
    double yaw_error_deg{0.0};
    bool has_ground_truth{false};
};

} // namespace lambda_brief
