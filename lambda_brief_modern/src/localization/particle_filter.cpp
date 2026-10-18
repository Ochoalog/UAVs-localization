#include "localization/particle_filter.hpp"
#include "core/descriptor.hpp"
#include <chrono>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <omp.h>
#include <limits>

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

void ParticleFilter::initializeAroundPose(
    const cv::Size& map_size,
    const Pose& center_xy_yaw,
    double sigma_xy_px,
    double sigma_yaw_deg,
    double mean_z,
    double sigma_z
) {
    map_size_ = map_size;
    particles_.resize(config_.num_particles);

    std::normal_distribution<double> distX(center_xy_yaw.x, sigma_xy_px);
    std::normal_distribution<double> distY(center_xy_yaw.y, sigma_xy_px);
    std::normal_distribution<double> distZ(mean_z, sigma_z);
    std::normal_distribution<double> distYaw(center_xy_yaw.yaw, sigma_yaw_deg * M_PI / 180.0);

    const double initial_w = 1.0 / static_cast<double>(config_.num_particles);
    const double minX = 0.0, maxX = static_cast<double>(map_size.width);
    const double minY = 0.0, maxY = static_cast<double>(map_size.height);

    for (size_t i = 0; i < config_.num_particles; ++i) {
        particles_[i].pose.x = std::clamp(distX(rng_), minX, maxX);
        particles_[i].pose.y = std::clamp(distY(rng_), minY, maxY);
        particles_[i].pose.z = std::clamp(distZ(rng_), config_.min_scale, config_.max_scale);
        particles_[i].pose.yaw = distYaw(rng_);
        particles_[i].pose.normalizeYaw();
        particles_[i].pose.roll = 0.0;
        particles_[i].pose.pitch = 0.0;
        particles_[i].weight = initial_w;
    }
}

size_t ParticleFilter::injectionCount() const {
    const size_t N = particles_.size();
    if (N == 0 || map_size_.width <= 0 || map_size_.height <= 0) return 0;
    double ratio = config_.inject_ratio;
    if (config_.inject_spread_px > 0.0) {
        double sw = 0.0, mx = 0.0, my = 0.0;
        for (const auto& p : particles_) { sw += p.weight; mx += p.weight * p.pose.x; my += p.weight * p.pose.y; }
        if (sw > 0.0) {
            mx /= sw; my /= sw;
            double v = 0.0;
            for (const auto& p : particles_) {
                v += p.weight * ((p.pose.x - mx) * (p.pose.x - mx) + (p.pose.y - my) * (p.pose.y - my));
            }
            const double spread = std::sqrt(v / sw);
            ratio *= std::clamp(spread / config_.inject_spread_px, 0.02, 1.0);
        }
    }
    return static_cast<size_t>(std::lround(ratio * static_cast<double>(N)));
}

Pose ParticleFilter::sampleRandomPose(std::normal_distribution<double>& gauss) {
    std::uniform_real_distribution<double> ux(0.0, map_size_.width), uy(0.0, map_size_.height);
    std::uniform_real_distribution<double> uyaw(-M_PI, M_PI), uz(config_.min_scale, config_.max_scale);
    const bool z_prior = prior_z_ > 0.0 && prior_rel_sigma_ > 0.0;
    const bool yaw_prior = prior_yaw_sigma_ > 0.0;
    Pose p;
    p.x = ux(rng_);
    p.y = uy(rng_);
    p.yaw = yaw_prior ? prior_yaw_ + prior_yaw_sigma_ * gauss(rng_) : uyaw(rng_);
    p.normalizeYaw();
    p.z = z_prior ? prior_z_ * (1.0 + prior_rel_sigma_ * gauss(rng_)) : uz(rng_);
    p.z = std::clamp(p.z, config_.min_scale, config_.max_scale);
    p.roll = 0.0;
    p.pitch = 0.0;
    return p;
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

        const double noise = pos_noise_mag + config_.motion_noise_frac * std::hypot(new_x, new_y);
        p.pose.x += cy * new_x - sy * new_y + dist_unit(rng_) * noise;
        p.pose.y += sy * new_x + cy * new_y + dist_unit(rng_) * noise;
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
    const double inv_two_var = 0.5 / var;
    const double NEG_INF = -std::numeric_limits<double>::infinity();

    // 0. Candidate random poses for the injection (they use the priors set for this update).
    //    They are scored like particles; whether they enter the cloud is decided in step 1b.
    std::vector<Pose> probes;
    last_injected_ = 0;
    contrast_valid_ = false;
    if (config_.inject_ratio > 0.0) {
        const size_t n = injectionCount();
        std::normal_distribution<double> gauss(0.0, 1.0);
        probes.reserve(n);
        for (size_t k = 0; k < n; ++k) probes.push_back(sampleRandomPose(gauss));
    }
    std::vector<double> probe_d(probes.size(), 1.0);
    #pragma omp parallel for schedule(dynamic, 64)
    for (size_t k = 0; k < probes.size(); ++k) {
        probe_d[k] = Descriptor::evaluateParticleDifference(
            map_bgr, probes[k], drone_size, pairs, drone_descriptor, mode);
    }

    // 1. Hamming distance for every particle whose centre lies inside the map.
    //    Legacy (Mcl.cpp::weighting): a centre outside the map gets probability 0.
    diff_.assign(N, -1.0);
    #pragma omp parallel for schedule(dynamic, 64)
    for (size_t i = 0; i < N; ++i) {
        const Pose& p = particles_[i].pose;
        if (p.x < 0.0 || p.y < 0.0 || p.x >= map_bgr.cols || p.y >= map_bgr.rows) {
            continue;
        }
        diff_[i] = Descriptor::evaluateParticleDifference(
            map_bgr, p, drone_size, pairs, drone_descriptor, mode);
    }

    // 1b. Observation contrast and injection decision.
    if (!probes.empty()) {
        double s0 = 0.0, s1 = 0.0, s2 = 0.0;
        for (double d : probe_d) {
            if (d >= 0.999) continue;  // footprint fully outside the map
            s0 += 1.0; s1 += d; s2 += d * d;
        }
        double cw = 0.0, cd = 0.0;
        for (size_t i = 0; i < N; ++i) {
            if (diff_[i] < 0.0 || diff_[i] >= 0.999) continue;
            cw += particles_[i].weight;
            cd += particles_[i].weight * diff_[i];
        }
        contrast_valid_ = false;
        if (s0 >= 10.0 && cw > 0.0) {
            const double mu0 = s1 / s0;
            const double sd0 = std::sqrt(std::max(s2 / s0 - mu0 * mu0, 1e-8));
            contrast_ = (mu0 - cd / cw) / sd0;
            contrast_valid_ = true;
        }

        const bool inject = injection_enabled_ &&
                            (config_.inject_gate <= 0.0 || !contrast_valid_ || contrast_ >= config_.inject_gate);
        if (inject) {
            double w_mean = 0.0;
            for (const auto& p : particles_) w_mean += p.weight;
            w_mean /= static_cast<double>(N);
            std::uniform_int_distribution<size_t> pick(0, N - 1);
            for (size_t k = 0; k < probes.size(); ++k) {
                const size_t i = pick(rng_);
                particles_[i].pose = probes[k];
                particles_[i].weight = w_mean;
                diff_[i] = probe_d[k];
            }
            last_injected_ = probes.size();
        }
    }

    // 2. Gaussian likelihood N(d; 0, var_brief) (constant factor omitted) + optional scale/heading priors.
    const bool use_prior = prior_z_ > 0.0 && prior_rel_sigma_ > 0.0;
    const double prior_inv_two_var = use_prior ? 0.5 / std::pow(prior_rel_sigma_ * prior_z_, 2.0) : 0.0;
    const bool use_yaw_prior = prior_yaw_sigma_ > 0.0;
    const double yaw_inv_two_var = use_yaw_prior ? 0.5 / (prior_yaw_sigma_ * prior_yaw_sigma_) : 0.0;
    log_lik_.assign(N, NEG_INF);
    for (size_t i = 0; i < N; ++i) {
        if (diff_[i] < 0.0) continue;
        double ll = -(diff_[i] * diff_[i]) * inv_two_var;
        if (use_prior) {
            const double dz = particles_[i].pose.z - prior_z_;
            ll -= dz * dz * prior_inv_two_var;
        }
        if (use_yaw_prior) {
            const double dyaw = std::remainder(particles_[i].pose.yaw - prior_yaw_, 2.0 * M_PI);
            ll -= dyaw * dyaw * yaw_inv_two_var;
        }
        log_lik_[i] = ll;
    }
    prior_z_ = -1.0;          // the priors apply to one update only
    prior_yaw_sigma_ = -1.0;

    // 3. Bayesian update  w_t ∝ w_{t-1} · p(z_t | x_t)  in the log domain.
    //    (The original implementation OVERWROTE w with p(z|x); combined with the
    //     Neff-triggered resampling this discarded all past evidence.)
    double max_lw = NEG_INF;
    std::vector<double> log_w(N);
    for (size_t i = 0; i < N; ++i) {
        const double prev = particles_[i].weight;
        log_w[i] = (prev > 0.0 && std::isfinite(log_lik_[i])) ? std::log(prev) + log_lik_[i] : NEG_INF;
        if (log_w[i] > max_lw) max_lw = log_w[i];
    }

    if (!std::isfinite(max_lw)) {
        // Every particle is outside the map / degenerate: fall back to uniform
        // weights (legacy "Correct zero error" branch).
        const double uniform_w = 1.0 / static_cast<double>(N);
        for (auto& p : particles_) p.weight = uniform_w;
        neff_pre_ = static_cast<double>(N);
        return;
    }

    double sum_w = 0.0;
    for (size_t i = 0; i < N; ++i) {
        const double w = std::isfinite(log_w[i]) ? std::exp(log_w[i] - max_lw) : 0.0;
        particles_[i].weight = w;
        sum_w += w;
    }
    const double inv_sum = 1.0 / sum_w;
    for (auto& p : particles_) p.weight *= inv_sum;
    neff_pre_ = getNeff();
}

double ParticleFilter::getNeff() const {
    double sum_sq = 0.0;
    for (const auto& p : particles_) {
        sum_sq += p.weight * p.weight;
    }
    return (sum_sq > 1e-12) ? (1.0 / sum_sq) : 0.0;
}

bool ParticleFilter::resampleIfNeeded() {
    const bool always = config_.resample_neff_ratio >= 1.0;
    if (!always) {
        double neff = getNeff();
        double threshold = static_cast<double>(particles_.size()) * config_.resample_neff_ratio;
        if (neff >= threshold) {
            return false;
        }
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
    state.neff = (neff_pre_ > 0.0) ? neff_pre_ : getNeff();

    // Variances above describe the spread of the WHOLE cloud around its mean. With
    // mode_estimate the reported pose is the densest mode instead of the mean.
    if (config_.mode_estimate) {
        double w_mode = 0.0;
        const Pose m = modeEstimate(w_mode);
        if (w_mode > 0.0) state.mean_pose = m;
    }

    return state;
}

Pose ParticleFilter::modeEstimate(double& weight_in_mode) const {
    weight_in_mode = 0.0;
    const double cell = std::max(config_.mode_radius_px, 1.0);
    double min_x = 0.0, min_y = 0.0, max_x = map_size_.width, max_y = map_size_.height;
    if (map_size_.width <= 0 || map_size_.height <= 0) {
        min_x = min_y = std::numeric_limits<double>::max();
        max_x = max_y = std::numeric_limits<double>::lowest();
        for (const auto& p : particles_) {
            min_x = std::min(min_x, p.pose.x); max_x = std::max(max_x, p.pose.x);
            min_y = std::min(min_y, p.pose.y); max_y = std::max(max_y, p.pose.y);
        }
    }
    const int W = std::max(1, static_cast<int>(std::ceil((max_x - min_x) / cell)) + 1);
    const int H = std::max(1, static_cast<int>(std::ceil((max_y - min_y) / cell)) + 1);
    std::vector<double> grid(static_cast<size_t>(W) * H, 0.0);
    for (const auto& p : particles_) {
        const int gx = static_cast<int>(std::floor((p.pose.x - min_x) / cell));
        const int gy = static_cast<int>(std::floor((p.pose.y - min_y) / cell));
        if (gx < 0 || gy < 0 || gx >= W || gy >= H) continue;
        grid[static_cast<size_t>(gy) * W + gx] += p.weight;
    }
    // Densest 3x3 block of cells.
    double best = -1.0; int bx = 0, by = 0;
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            double acc = 0.0;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    const int xx = x + dx, yy = y + dy;
                    if (xx < 0 || yy < 0 || xx >= W || yy >= H) continue;
                    acc += grid[static_cast<size_t>(yy) * W + xx];
                }
            }
            if (acc > best) { best = acc; bx = x; by = y; }
        }
    }
    double cx = min_x + (bx + 0.5) * cell, cy = min_y + (by + 0.5) * cell;

    // Mean-shift refinement with a flat kernel of radius 1.5 cells.
    const double r2 = std::pow(1.5 * cell, 2.0);
    Pose out;
    for (int it = 0; it < 10; ++it) {
        double sw = 0.0, sx = 0.0, sy = 0.0, sz = 0.0, ss = 0.0, sc = 0.0;
        for (const auto& p : particles_) {
            const double dx = p.pose.x - cx, dy = p.pose.y - cy;
            if (dx * dx + dy * dy > r2) continue;
            sw += p.weight; sx += p.weight * p.pose.x; sy += p.weight * p.pose.y; sz += p.weight * p.pose.z;
            ss += p.weight * std::sin(p.pose.yaw); sc += p.weight * std::cos(p.pose.yaw);
        }
        if (sw <= 0.0) return out;
        const double nx = sx / sw, ny = sy / sw;
        out = Pose(nx, ny, sz / sw, std::atan2(ss, sc));
        weight_in_mode = sw;
        const bool done = std::hypot(nx - cx, ny - cy) < 0.5;
        cx = nx; cy = ny;
        if (done) break;
    }
    return out;
}

std::vector<Particle> ParticleFilter::getExportParticles(int max_count) const {
    if (max_count <= 0 || static_cast<size_t>(max_count) >= particles_.size()) {
        return particles_;
    }

    // Select top particles by weight for informative visualization
    std::vector<Particle> sorted_copy = particles_;
    size_t k = static_cast<size_t>(max_count);
    std::partial_sort(
        sorted_copy.begin(),
        sorted_copy.begin() + k,
        sorted_copy.end(),
        [](const Particle& a, const Particle& b) {
            return a.weight > b.weight;
        }
    );
    sorted_copy.resize(k);
    return sorted_copy;
}

} // namespace lambda_brief
