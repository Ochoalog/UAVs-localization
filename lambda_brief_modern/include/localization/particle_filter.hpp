#pragma once

#include "core/types.hpp"
#include "core/pixel_sampler.hpp"
#include <opencv2/core.hpp>
#include <vector>
#include <random>

namespace lambda_brief {

struct ParticleFilterConfig {
    size_t num_particles{50000};
    double min_scale{0.1};
    double max_scale{3.0};
    double var_brief{0.0225}; // sigma = 0.15 -> var = 0.15^2
    double pos_noise_m{10.0};
    double yaw_noise_reliable_deg{5.0};
    double yaw_noise_unreliable_deg{30.0};
    double scale_noise{0.02578};
    double resample_neff_ratio{0.5}; // Resample when Neff < N * ratio
    unsigned int random_seed{0};
};

class ParticleFilter {
public:
    explicit ParticleFilter(const ParticleFilterConfig& config = ParticleFilterConfig{});

    /**
     * @brief Initialize particle cloud uniformly across the satellite map
     */
    void initializeUniform(const cv::Size& map_size);

    /**
     * @brief Motion prediction step applying relative odometry and Gaussian noise
     */
    void predict(const Pose& relative_motion, bool is_reliable);

    /**
     * @brief Update particle weights using the observed drone descriptor and satellite map
     * Parallelized via OpenMP.
     */
    void updateWeights(
        const cv::Mat& map_bgr,
        const cv::Size& drone_size,
        const std::vector<PixelPair>& pairs,
        const std::vector<uint8_t>& drone_descriptor,
        DescriptorColorMode mode = DescriptorColorMode::LAB_NO_L
    );

    /**
     * @brief Perform Low Variance Resampling if effective particle count is below threshold
     */
    bool resampleIfNeeded();

    /**
     * @brief Computes weighted mean pose and uncertainty
     */
    EstimationState getEstimationState() const;

    /**
     * @brief Accessors
     */
    const std::vector<Particle>& getParticles() const { return particles_; }
    size_t getNumParticles() const { return particles_.size(); }
    double getNeff() const;

private:
    ParticleFilterConfig config_;
    cv::Size map_size_{0, 0};
    std::vector<Particle> particles_;
    mutable std::mt19937 rng_;
};

} // namespace lambda_brief
