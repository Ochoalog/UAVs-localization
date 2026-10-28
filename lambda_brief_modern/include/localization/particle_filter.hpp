#pragma once

#include "core/types.hpp"
#include "core/pixel_sampler.hpp"
#include "core/descriptor.hpp"
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
    // Extra translation noise proportional to the odometry step: +-(pos_noise + frac * |dxy| * z).
    // The VO translation is off by 25-40% in some ADM/0036 segments (turns, low altitude); a fixed
    // +-10 px cannot absorb that bias. 0 = legacy (fixed noise only).
    double motion_noise_frac{0.0};
    double yaw_noise_reliable_deg{5.0};
    double yaw_noise_unreliable_deg{30.0};
    double scale_noise{0.02578};
    // Resample when Neff < N * ratio. A value >= 1.0 resamples at EVERY frame,
    // which is what the legacy MCL (phir2frameworkv2/Mcl.cpp::run) does and what
    // the paper results were produced with.
    double resample_neff_ratio{1.0};
    unsigned int random_seed{0};

    // Random particle injection (global-localization recovery, cf. Augmented MCL).
    // Right before every weight update, this fraction of the cloud is replaced by fresh
    // samples: x, y and yaw uniform (yaw ~ N(compass, sigma) when a heading prior is set),
    // z uniform in [min_scale, max_scale] (z ~ N(z_ref, rel_sigma z_ref) with an altitude
    // prior). With 50k uniform particles only ~0.3 fall inside the likelihood basin of the true
    // pose; without injection, a cloud that concentrated on a false mode can never recover.
    // 0 = legacy behaviour.
    double inject_ratio{0.0};
    // > 0: scale the injection by the cloud spread, r_eff = r * clamp(std_xy / inject_spread_px, 0.02, 1).
    // A tight cloud receives almost no random particles. Caveat (ADM/0036 end of flight): in weak
    // frames the cloud spreads, the injection returns to full rate and a lucky random particle
    // can take the mass. Prefer inject_gate.
    double inject_spread_px{0.0};
    // > 0: observation-quality gate for the injection. Every frame the candidate random poses are
    // drawn and scored anyway (a sample of the "null" distribution of d); the frame contrast
    //   C = (mean_null(d) - mean_cloud(d)) / std_null(d)
    // measures how much better the current cloud explains the frame than random poses. They are
    // injected only when C >= inject_gate (informative frame). In weak frames (map/scene mismatch,
    // turns, low-altitude end of ADM/0036) C drops and nothing is injected, so a lucky random pose
    // cannot take the mass from a cloud that is tracking. Note: C cannot tell a converged false
    // mode from real tracking (both give C ~ 1 on ADM/0036), so it is NOT used as a "lost" detector.
    double inject_gate{0.0};

    // Pose estimate: false = weighted mean of the whole cloud (legacy-like), true = mode of the
    // cloud (densest cell of a mode_radius_px grid refined by mean-shift within 1.5 x radius).
    // The mean of a multimodal cloud falls between the modes.
    bool mode_estimate{false};
    double mode_radius_px{100.0};
};

class ParticleFilter {
public:
    explicit ParticleFilter(const ParticleFilterConfig& config = ParticleFilterConfig{});

    /**
     * @brief Initialize particle cloud uniformly across the satellite map
     */
    void initializeUniform(const cv::Size& map_size);

    /**
     * @brief Diagnostic initialization: particle cloud drawn as a Gaussian around a
     *        known pose (e.g. the ground truth of the first frame of a slice), instead
     *        of the legacy uniform spread. Used to isolate tracking from global
     *        convergence (see docs/analise_fim_do_voo_adm2.md). Not used by default.
     * @param center_xy_yaw Pose whose x, y, yaw are the Gaussian centre (z is ignored; use mean_z).
     * @param sigma_xy_px   Std-dev of x, y (map px).
     * @param sigma_yaw_deg Std-dev of yaw (degrees).
     * @param mean_z        Mean of the initial scale z.
     * @param sigma_z       Std-dev of the initial scale z.
     */
    void initializeAroundPose(
        const cv::Size& map_size,
        const Pose& center_xy_yaw,
        double sigma_xy_px,
        double sigma_yaw_deg,
        double mean_z,
        double sigma_z
    );

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
     * @brief Soft scale prior for the NEXT updateWeights() call (altimeter / barometer):
     *        w *= exp(-(z - z_ref)^2 / (2 (rel_sigma * z_ref)^2)). z_ref <= 0 disables it.
     *        Without it the lambda-BRIEF likelihood is almost flat in z and small-scale
     *        false modes win (see docs/diagnostico_convergencia_e_pipeline.md, sec. 2.7).
     */
    void setScalePrior(double z_ref, double rel_sigma) { prior_z_ = z_ref; prior_rel_sigma_ = rel_sigma; }

    /**
     * @brief Soft heading prior for the NEXT updateWeights() call (magnetometer / compass):
     *        w *= exp(-wrap(yaw - yaw_ref)^2 / (2 sigma^2)). sigma <= 0 disables it.
     */
    void setYawPrior(double yaw_ref, double sigma_rad) { prior_yaw_ = yaw_ref; prior_yaw_sigma_ = sigma_rad; }

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

    /** @brief Neff computed right after the last weight update (before resampling). */
    double getNeffBeforeResampling() const { return neff_pre_; }

    /**
     * @brief Enable/disable the random injection at run time (two-phase mode). When disabled
     *        and inject_ratio > 0, the candidate random poses are still drawn and scored, so the
     *        observation contrast C keeps being measured, but none enters the cloud.
     */
    void setInjectionEnabled(bool enabled) { injection_enabled_ = enabled; }

    /**
     * @brief v3: weight every pair by the map stability where it lands (--stability_map).
     *        nullptr turns it off (default: the plain Hamming distance, bit-identical to v2).
     *        The map must outlive the filter and have the size of the satellite map.
     */
    void setPairWeightMap(const cv::Mat* weights, StabilityFormula formula) {
        pair_weights_.map = weights;
        pair_weights_.formula = formula;
    }
    bool injectionEnabled() const { return injection_enabled_; }

    /** @brief Observation contrast C of the last frame (see inject_gate) and particles injected. */
    double getObservationContrast() const { return contrast_; }
    /** @brief True when C was measured in the last updateWeights() (needs inject_ratio > 0). */
    bool hasObservationContrast() const { return contrast_valid_; }
    size_t getLastInjectedCount() const { return last_injected_; }

    /**
     * @brief Retrieve a representative subset of particles for visualization/logging
     * @param max_count Maximum number of particles to export (-1 for all)
     */
    std::vector<Particle> getExportParticles(int max_count) const;

private:
    size_t injectionCount() const;
    Pose sampleRandomPose(std::normal_distribution<double>& gauss);
    Pose modeEstimate(double& weight_in_mode) const;

    ParticleFilterConfig config_;
    cv::Size map_size_{0, 0};
    std::vector<Particle> particles_;
    std::vector<double> log_lik_;  // per-particle log-likelihood scratch buffer
    std::vector<double> diff_;     // per-particle Hamming distance scratch buffer
    PairWeightMap pair_weights_;   // v3 (--stability_map); map == nullptr = off
    double prior_z_{-1.0};
    double prior_rel_sigma_{0.15};
    double prior_yaw_{0.0};
    double prior_yaw_sigma_{-1.0};
    double neff_pre_{-1.0};
    double contrast_{0.0};
    bool contrast_valid_{false};
    bool injection_enabled_{true};
    size_t last_injected_{0};
    mutable std::mt19937 rng_;
};

} // namespace lambda_brief
