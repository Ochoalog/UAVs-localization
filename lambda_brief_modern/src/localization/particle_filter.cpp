#include "localization/particle_filter.hpp"
#include "core/descriptor.hpp"
#include <chrono>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <omp.h>

namespace lambda_brief {

ParticleFilter::ParticleFilter(const ParticleFilterConfig& config)
    : config_(config) {
    unsigned int seed = config_.random_seed;
    if (seed == 0) {
        seed = static_cast<unsigned int>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count()
        );
    }
    rng_.seed(seed);
}

void ParticleFilter::initializeUniform(const cv::Size& map_size) {
    map_size_ = map_size;
    particles_.resize(config_.num_particles);

    std::uniform_real_distribution<double> distX(0.0, static_cast<double>(map_size.width));
    std::uniform_real_distribution<double> distY(0.0, static_cast<double>(map_size.height));
    std::uniform_real_distribution<double> distZ(config_.min_scale, config_.max_scale);
    std::uniform_real_distribution<double> distYaw(-M_PI, M_PI);

    const double initial_w = 1.0 / static_cast<double>(config_.num_particles);

    for (size_t i = 0; i < config_.num_particles; ++i) {
        particles_[i].pose.x = distX(rng_);
        particles_[i].pose.y = distY(rng_);
        particles_[i].pose.z = distZ(rng_);
        particles_[i].pose.yaw = distYaw(rng_);
        particles_[i].pose.roll = 0.0;
        particles_[i].pose.pitch = 0.0;
        particles_[i].weight = initial_w;
    }
}

void ParticleFilter::predict(const Pose& relative_motion, bool is_reliable) {
    std::uniform_real_distribution<double> dist_unit(-1.0, 1.0);
    std::uniform_real_distribution<double> dist_scale(-config_.scale_noise, config_.scale_noise);

    const double yaw_noise_mag = (is_reliable ? config_.yaw_noise_reliable_deg : config_.yaw_noise_unreliable_deg) * (M_PI / 180.0);
    const double pos_noise_mag = config_.pos_noise_m;

    for (auto& p : particles_) {
        double new_x = relative_motion.x * p.pose.z;
        double new_y = relative_motion.y * p.pose.z;
        double cy = std::cos(p.pose.yaw);
        double sy = std::sin(p.pose.yaw);

        p.pose.x += cy * new_x - sy * new_y + dist_unit(rng_) * pos_noise_mag;
        p.pose.y += sy * new_x + cy * new_y + dist_unit(rng_) * pos_noise_mag;
        p.pose.yaw += relative_motion.yaw + dist_unit(rng_) * yaw_noise_mag;
        p.pose.normalizeYaw();

        p.pose.z = (p.pose.z * relative_motion.z) + dist_scale(rng_);
        p.pose.z = std::clamp(p.pose.z, config_.min_scale, config_.max_scale);
    }
}

void ParticleFilter::updateWeights(
    const cv::Mat& map_bgr,
    const cv::Size& drone_size,
    const std::vector<PixelPair>& pairs,
    const std::vector<uint8_t>& drone_descriptor,
    DescriptorColorMode mode
) {
    const size_t N = particles_.size();
    const double var = config_.var_brief;
    const double norm_factor = 1.0 / std::sqrt(2.0 * M_PI * var);
    const double inv_two_var = 0.5 / var;

    #pragma omp parallel for schedule(dynamic, 64)
    for (size_t i = 0; i < N; ++i) {
        double diff = Descriptor::evaluateParticleDifference(
            map_bgr,
            particles_[i].pose,
            drone_size,
            pairs,
            drone_descriptor,
            mode
        );

        double prob = norm_factor * std::exp(-(diff * diff) * inv_two_var);
        particles_[i].weight = prob;
    }

    // Normalize weights
    double sum_w = 0.0;
    for (size_t i = 0; i < N; ++i) {
        sum_w += particles_[i].weight;
    }

    if (sum_w > 1e-12) {
        double inv_sum = 1.0 / sum_w;
        for (size_t i = 0; i < N; ++i) {
            particles_[i].weight *= inv_sum;
        }
    } else {
        double uniform_w = 1.0 / static_cast<double>(N);
        for (size_t i = 0; i < N; ++i) {
            particles_[i].weight = uniform_w;
        }
    }
}

double ParticleFilter::getNeff() const {
    double sum_sq = 0.0;
    for (const auto& p : particles_) {
        sum_sq += p.weight * p.weight;
    }
    return (sum_sq > 1e-12) ? (1.0 / sum_sq) : 0.0;
}

bool ParticleFilter::resampleIfNeeded() {
    double neff = getNeff();
    double threshold = static_cast<double>(particles_.size()) * config_.resample_neff_ratio;

    if (neff >= threshold) {
        return false;
    }

    // Low Variance Sampler
    const size_t N = particles_.size();
    std::uniform_real_distribution<double> dist_step(0.0, 1.0 / static_cast<double>(N));
    double r = dist_step(rng_);
    double c = particles_[0].weight;
    size_t i = 0;

    std::vector<Particle> new_particles;
    new_particles.reserve(N);

    for (size_t m = 0; m < N; ++m) {
        double U = r + static_cast<double>(m) * (1.0 / static_cast<double>(N));
        while (U > c && i + 1 < N) {
            i++;
            c += particles_[i].weight;
        }
        new_particles.push_back(particles_[i]);
    }

    const double uniform_w = 1.0 / static_cast<double>(N);
    for (auto& p : new_particles) {
        p.weight = uniform_w;
    }

    particles_ = std::move(new_particles);
    return true;
}

EstimationState ParticleFilter::getEstimationState() const {
    EstimationState state;
    const size_t N = particles_.size();
    if (N == 0) return state;

    double mean_x = 0.0;
    double mean_y = 0.0;
    double mean_z = 0.0;
    double sin_sum = 0.0;
    double cos_sum = 0.0;

    for (const auto& p : particles_) {
        mean_x += p.pose.x * p.weight;
        mean_y += p.pose.y * p.weight;
        mean_z += p.pose.z * p.weight;
        sin_sum += std::sin(p.pose.yaw) * p.weight;
        cos_sum += std::cos(p.pose.yaw) * p.weight;
    }

    state.mean_pose.x = mean_x;
    state.mean_pose.y = mean_y;
    state.mean_pose.z = mean_z;
    state.mean_pose.yaw = std::atan2(sin_sum, cos_sum);

    double var_x = 0.0;
    double var_y = 0.0;
    double var_z = 0.0;
    for (const auto& p : particles_) {
        var_x += std::pow(p.pose.x - mean_x, 2.0) * p.weight;
        var_y += std::pow(p.pose.y - mean_y, 2.0) * p.weight;
        var_z += std::pow(p.pose.z - mean_z, 2.0) * p.weight;
    }

    state.variance.x = var_x;
    state.variance.y = var_y;
    state.variance.z = var_z;

    double R = std::sqrt(sin_sum * sin_sum + cos_sum * cos_sum);
    state.variance.yaw = (R < 1.0) ? (-2.0 * std::log(R)) : 0.0; // Circular variance
    state.neff = getNeff();

    return state;
}

} // namespace lambda_brief
