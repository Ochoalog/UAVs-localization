#include "config/config.hpp"
#include "core/vegetation_index.hpp"
#include "core/mask_generator.hpp"
#include "core/pixel_sampler.hpp"
#include "core/descriptor.hpp"
#include "localization/visual_odometry.hpp"
#include "localization/particle_filter.hpp"
#include "localization/phase_controller.hpp"
#include "dataset/dataset_loader.hpp"

#include <opencv2/imgcodecs.hpp>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <filesystem>
#include <cstdlib>
#include <sstream>
#include <unordered_map>

namespace fs = std::filesystem;
using namespace lambda_brief;

int main(int argc, char* argv[]) {
    std::cout << "====================================================\n";
    std::cout << "  lambda-BRIEF Modern Localization Engine (C++17)   \n";
    std::cout << "====================================================\n";

    AppConfig config = AppConfig::parseCommandLine(argc, argv);

    if (config.map_path.empty() || config.dataset_dir.empty()) {
        std::cerr << "Error: Both --map and --dataset_dir are required.\n\n";
        AppConfig::printUsage(argv[0]);
        return 1;
    }

    // 1. Load satellite map
    cv::Mat satellite_map_bgr = cv::imread(config.map_path, cv::IMREAD_COLOR);
    if (satellite_map_bgr.empty()) {
        std::cerr << "Error: Could not load satellite map from: " << config.map_path << std::endl;
        return 1;
    }
    std::cout << "[Map] Loaded satellite map: " << satellite_map_bgr.cols << "x"
              << satellite_map_bgr.rows << " (" << config.map_path << ")\n";

    // Legacy "-mappp 1": the map is colour-quantised (25 levels) BEFORE the CIELAB conversion.
    if (config.quantize_levels > 0) {
        satellite_map_bgr = Descriptor::quantizeColors(satellite_map_bgr, config.quantize_levels);
    }
    // Pre-convert satellite map to the descriptor's operating color space (e.g. CIELAB)
    cv::Mat satellite_map = Descriptor::convertColorSpace(satellite_map_bgr, config.color_mode);

    // Optional pre-recorded odometry (legacy traj/odom-*.txt: "dx dy dz dyaw reliable" per line,
    // line k = motion between frame k and k+1 of the processed slice).
    std::vector<std::pair<Pose, bool>> offline_odom;
    if (!config.odom_file.empty()) {
        std::ifstream of(config.odom_file);
        double ox, oy, oz, oyaw; int orel;
        while (of >> ox >> oy >> oz >> oyaw >> orel) {
            offline_odom.push_back({Pose(ox, oy, oz, oyaw), orel != 0});
        }
        std::cout << "[Odometry] Loaded " << offline_odom.size() << " offline odometry steps from "
                  << config.odom_file << "\n";
        if (offline_odom.empty()) {
            std::cerr << "Error: could not read odometry file " << config.odom_file << std::endl;
            return 1;
        }
    }

    // Optional real altimeter: "traj_idx z_baro ..." per line ('#' = comment). The key is the
    // traj.txt line index (DatasetFrame::index), so the file is independent of the slice.
    std::unordered_map<size_t, double> altimeter_z;
    if (!config.altimeter_file.empty()) {
        std::ifstream af(config.altimeter_file);
        if (!af) {
            std::cerr << "Error: could not open altimeter file " << config.altimeter_file << std::endl;
            return 1;
        }
        std::string line;
        while (std::getline(af, line)) {
            if (line.empty() || line[0] == '#') continue;
            std::istringstream ls(line);
            long idx; double z;
            if (ls >> idx >> z && idx >= 0 && z > 0.0) altimeter_z[static_cast<size_t>(idx)] = z;
        }
        std::cout << "[Altimeter] Loaded " << altimeter_z.size() << " frames from " << config.altimeter_file << "\n";
        if (altimeter_z.empty()) {
            std::cerr << "Error: altimeter file " << config.altimeter_file << " has no valid lines" << std::endl;
            return 1;
        }
        if (config.scale_prior_sigma <= 0.0) {
            std::cerr << "Warning: --altimeter_file without --scale_prior_sigma > 0 has no effect\n";
        }
    }

    // 2. Load dataset
    DatasetLoader loader(config.toDatasetLoaderOptions());
    if (!loader.load() || loader.size() == 0) {
        std::cerr << "Error: Failed to load flight dataset from: " << config.dataset_dir << std::endl;
        return 1;
    }

    // 3. Prepare output directory
    fs::create_directories(config.output_dir);
    std::ofstream traj_out(fs::path(config.output_dir) / "traj_estimated.txt");
    std::ofstream error_out(fs::path(config.output_dir) / "errors.txt");
    std::ofstream particles_out;
    if (config.save_particles) {
        particles_out.open(fs::path(config.output_dir) / "particles_log.csv");
        particles_out << "frame,particle_id,x,y,z,yaw_deg,weight\n";
    }

    // Legacy format header
    traj_out << "meanPX meanPY meanPZ varPX varPY varPZ meanAngle angleStdev NEFF elapsedTime\n";
    error_out << "frame meanPX meanPY meanPZ trueX trueY trueZ error_pos_m error_yaw_deg\n";

    // 4. Initialize Localization Engine
    ParticleFilter pf(config.toParticleFilterConfig());
    if (config.init_around_gt_sigma_px > 0.0 && loader.size() > 0 && loader.getFrame(0).has_ground_truth) {
        const auto& gt0 = loader.getFrame(0).ground_truth;
        const double mean_z = 0.87 * gt0.z;
        const double sigma_z = 0.05 * mean_z;
        pf.initializeAroundPose(
            satellite_map.size(),
            Pose(gt0.x, gt0.y, mean_z, gt0.yaw_rad),
            config.init_around_gt_sigma_px, 5.0, mean_z, sigma_z
        );
        std::cout << "[Init] Diagnostic: cloud initialized around GT of frame 0 (sigma_xy="
                  << config.init_around_gt_sigma_px << "px, mean_z=" << mean_z << ")\n";
    } else {
        if (config.init_around_gt_sigma_px > 0.0) {
            std::cerr << "Warning: --init_around_gt requested but frame 0 has no ground truth; "
                         "falling back to uniform initialization.\n";
        }
        pf.initializeUniform(satellite_map.size());
    }

    VisualOdometryOptions vo_opt;
    vo_opt.type = FeatureType::SIFT;
    vo_opt.max_features = config.sift_features;
    vo_opt.matcher = (config.vo_matcher == "ratio") ? VOMatcher::RATIO : VOMatcher::LEGACY;
    vo_opt.ratio = config.vo_ratio;
    vo_opt.robust = (config.vo_robust == "magsac") ? VORobust::MAGSAC : VORobust::RANSAC;
    vo_opt.reproj_threshold_px = config.vo_reproj_px;
    vo_opt.min_inliers = config.vo_min_inliers;
    vo_opt.scale_deadband = config.vo_scale_deadband;
    VisualOdometry vo(vo_opt);

    // Constant-velocity fallback for unreliable transitions (--unreliable_motion cv):
    // exponential moving average of the last reliable (dx, dy, dyaw); dz is always 1.
    bool have_velocity = false;
    Pose velocity(0.0, 0.0, 1.0, 0.0);
    constexpr double kVelocityEma = 0.5;
    if (!offline_odom.empty() && offline_odom.size() + 1 < loader.size()) {
        std::cerr << "Warning: --odom_file has " << offline_odom.size() << " steps but the slice has "
                  << loader.size() << " frames; the last step will be repeated for the remaining frames.\n";
    }

    // Two-phase filter: priors / injection / global VO dead-band only until convergence.
    PhaseConfig phase_cfg;
    phase_cfg.enabled = config.two_phase;
    phase_cfg.converge_std_px = config.converge_std_px;
    phase_cfg.converge_frames = config.converge_frames;
    phase_cfg.converge_contrast = config.converge_contrast;
    phase_cfg.relocalize_std_px = config.relocalize_std_px;
    phase_cfg.relocalize_frames = config.relocalize_frames;
    PhaseController phase(phase_cfg);
    std::ofstream phase_out;
    if (config.two_phase) {
        phase_out.open(fs::path(config.output_dir) / "phase_log.txt");
        phase_out << "frame phase spread_px contrast_valid contrast injected neff_pre\n";
        if (config.converge_contrast > 0.0 && config.inject_ratio <= 0.0) {
            std::cerr << "Warning: --two_phase with --converge_contrast needs --inject_ratio > 0 to measure the "
                         "contrast; convergence will use the spread only.\n";
        }
    }
    PixelSampler sampler(config.random_seed);

    std::vector<double> position_errors;
    std::vector<double> yaw_errors;

    std::cout << "\nStarting Monte Carlo Localization with " << pf.getNumParticles()
              << " particles over " << loader.size() << " frames...\n";

    auto start_all = std::chrono::high_resolution_clock::now();

    for (size_t f = 0; f < loader.size(); ++f) {
        auto t_frame_start = std::chrono::high_resolution_clock::now();
        const auto& frame = loader.getFrame(f);
        const bool tracking = phase.tracking();
        if (config.two_phase) {
            vo.setScaleDeadband(tracking ? config.track_vo_scale_deadband : config.vo_scale_deadband);
            pf.setInjectionEnabled(!tracking);
        }

        cv::Mat rgb = frame.loadRGB();
        if (rgb.empty()) {
            std::cerr << "Warning: Could not read RGB image at frame " << f << std::endl;
            continue;
        }
        // Legacy: the drone frame is quantised exactly like the map (used by both
        // the descriptor and the online odometry).
        if (config.quantize_levels > 0) {
            rgb = Descriptor::quantizeColors(rgb, config.quantize_levels);
        }

        cv::Mat nir = loader.preprocessBand(frame.loadNIR());
        cv::Mat red = loader.preprocessBand(frame.loadRED());
        cv::Mat reg = loader.preprocessBand(frame.loadREG());
        cv::Mat gre = loader.preprocessBand(frame.loadGRE());

        // A. Odometry & Motion prediction (for f > 0)
        Pose rel_motion;
        bool is_reliable = false;

        // Online VO: features of the current frame are detected once and cached for the next
        // transition (the previous implementation re-detected SIFT on the previous frame every time).
        bool have_motion = false;
        if (offline_odom.empty()) {
            const bool primed = vo.hasPrevious();
            auto [motion, reliable] = vo.track(rgb);
            if (f > 0 && primed) {
                rel_motion = motion;
                is_reliable = reliable;
                have_motion = true;
            }
        } else if (f > 0) {
            const size_t k = std::min(f - 1, offline_odom.size() - 1);
            rel_motion = offline_odom[k].first;
            is_reliable = offline_odom[k].second;
            have_motion = true;
        }

        if (have_motion) {
            if (is_reliable) {
                const Pose u(rel_motion.x, rel_motion.y, 1.0, rel_motion.yaw);
                if (!have_velocity) {
                    velocity = u;
                    have_velocity = true;
                } else {
                    velocity.x = kVelocityEma * u.x + (1.0 - kVelocityEma) * velocity.x;
                    velocity.y = kVelocityEma * u.y + (1.0 - kVelocityEma) * velocity.y;
                    velocity.yaw = kVelocityEma * u.yaw + (1.0 - kVelocityEma) * velocity.yaw;
                }
            } else if (config.unreliable_motion == "zero") {
                // Legacy DroneRobot::run: u = 0 (only the wider "unreliable" noise is applied).
                rel_motion = Pose(0.0, 0.0, 1.0, 0.0);
            } else if (config.unreliable_motion == "cv") {
                rel_motion = have_velocity ? velocity : Pose(0.0, 0.0, 1.0, 0.0);
            }  // "apply": keep the motion of the rejected homography
            pf.predict(rel_motion, is_reliable);
            if (std::getenv("LB_DEBUG_ODOM")) {  // compare with traj/odom-*.txt
                const VOStats& s = vo.lastStats();
                std::cerr << "ODOM " << f << " " << rel_motion.x << " " << rel_motion.y << " "
                          << rel_motion.z << " " << rel_motion.yaw << " " << is_reliable
                          << " matches=" << s.matches << " inliers=" << s.inliers << "\n";
            }
        }

        // B. Multispectral Vegetation Index & Mask Generation
        cv::Mat vi_matrix;
        if (!nir.empty() && !red.empty()) {
            vi_matrix = VegetationIndex::computeMatrix(nir, reg, red, gre, config.vi_type);
        } else {
            // Fallback to grayscale luminance mask if multispectral bands not available
            cv::Mat gray;
            cv::cvtColor(rgb, gray, cv::COLOR_BGR2GRAY);
            vi_matrix = cv::Mat(gray.size(), CV_64FC1, 0.0);
        }

        cv::Mat inv_mask_8u = VegetationIndex::computeInvertedMask8U(vi_matrix);
        cv::Mat eq_mask = MaskGenerator::generateEqualizedVegetationMask(inv_mask_8u);
        cv::Mat gauss_mask = MaskGenerator::generateGaussianMask(rgb.size());

        double max_fitness = 1.0;
        cv::Mat combined_mask = MaskGenerator::combineMasks(
            eq_mask,
            gauss_mask,
            config.alpha,
            &max_fitness
        );

        // C. Pixel Sampling via Stochastic Acceptance Roulette
        std::vector<PixelPair> pairs = sampler.sampleRoulette(
            combined_mask,
            max_fitness,
            config.num_pairs
        );

        // D. Observed Drone Descriptor
        std::vector<uint8_t> drone_desc = Descriptor::computeDroneDescriptor(
            rgb,
            pairs,
            config.color_mode
        );

        // E. Weight Update & Resampling
        // Optional altimeter emulation. On ADM/0036 the GT scale is z_gt = 0.00616 x GPS altitude.
        // A pinhole model with the Sequoia RGB HFOV (63.9 deg, 1241 px wide RGBres frames) and
        // the 0.1887 m/px map gives z = 0.00536 x altitude = 0.87 z_gt, which is also where the
        // lambda-BRIEF distance is minimal and where the legacy filter settles -> gain 0.87.
        if (!tracking && config.scale_prior_sigma > 0.0) {
            if (!altimeter_z.empty()) {
                // Real altimeter: no ground truth is read by the filter.
                auto it = altimeter_z.find(frame.index);
                if (it != altimeter_z.end()) {
                    pf.setScalePrior(it->second * config.scale_prior_gain, config.scale_prior_sigma);
                }
            } else if (frame.has_ground_truth) {
                pf.setScalePrior(frame.ground_truth.z * config.scale_prior_gain, config.scale_prior_sigma);
            }
        }
        if (!tracking && config.yaw_prior_sigma_deg > 0.0 && frame.has_ground_truth) {
            pf.setYawPrior(frame.ground_truth.yaw_rad, config.yaw_prior_sigma_deg * M_PI / 180.0);
        }
        pf.updateWeights(
            satellite_map,
            rgb.size(),
            pairs,
            drone_desc,
            config.color_mode
        );

        // Diagnostic trace (LB_DEBUG_PF=1): Neff before resampling, particles near the GT
        // (xy < 40 px; and xy + |dyaw| < 20 deg + |dz| < 0.08) and the best particle vs the GT.
        if (std::getenv("LB_DEBUG_PF") && frame.has_ground_truth) {
            const auto& P = pf.getParticles();
            const auto& g = frame.ground_truth;
            size_t nxy = 0, nfull = 0; double best_w = 0; size_t best_i = 0;
            for (size_t i = 0; i < P.size(); ++i) {
                double dx = P[i].pose.x - g.x, dy = P[i].pose.y - g.y;
                double dyaw = std::remainder(P[i].pose.yaw - g.yaw_rad, 2 * M_PI);
                bool xy = dx * dx + dy * dy < 40.0 * 40.0;
                nxy += xy;
                nfull += xy && std::abs(dyaw) < 20 * M_PI / 180 && std::abs(P[i].pose.z - 0.87 * g.z) < 0.08;
                if (P[i].weight > best_w) { best_w = P[i].weight; best_i = i; }
            }
            Pose gp(g.x, g.y, g.z, g.yaw_rad);
            double d_gt = Descriptor::evaluateParticleDifference(satellite_map, gp, rgb.size(), pairs, drone_desc, config.color_mode);
            double d_best = Descriptor::evaluateParticleDifference(satellite_map, P[best_i].pose, rgb.size(), pairs, drone_desc, config.color_mode);
            std::cerr << "PF " << f << " neff_pre=" << pf.getNeff() << " contrast=" << pf.getObservationContrast()
                      << " injected=" << pf.getLastInjectedCount() << " near_xy40=" << nxy << " near_full=" << nfull << " d_gt=" << d_gt
                      << " d_best=" << d_best << " best=(" << P[best_i].pose.x << "," << P[best_i].pose.y << ","
                      << P[best_i].pose.z << "," << P[best_i].pose.yaw * 180 / M_PI << ") gt=(" << g.x << "," << g.y
                      << "," << g.z << "," << g.yaw_deg << ")\n";
        }
        pf.resampleIfNeeded();

        // F. State Estimation and Logging
        auto t_frame_end = std::chrono::high_resolution_clock::now();
        double elapsed_sec = std::chrono::duration<double>(t_frame_end - t_frame_start).count();

        EstimationState est = pf.getEstimationState();
        est.frame_index = f;
        est.elapsed_seconds = elapsed_sec;

        if (config.two_phase) {
            const double spread = std::sqrt(std::max(est.variance.x + est.variance.y, 0.0));
            phase_out << f << " " << (tracking ? "TRACKING" : "GLOBAL") << " " << std::fixed << std::setprecision(1)
                      << spread << " " << pf.hasObservationContrast() << " " << std::setprecision(3)
                      << pf.getObservationContrast() << " " << pf.getLastInjectedCount() << " "
                      << std::setprecision(0) << est.neff << "\n";
            if (phase.update(spread, pf.hasObservationContrast(), pf.getObservationContrast())) {
                std::cout << "[Phase] frame " << f << ": "
                          << (phase.tracking() ? "GLOBAL -> TRACKING (converged)" : "TRACKING -> GLOBAL (relocalizing)")
                          << " | spread " << std::fixed << std::setprecision(1) << spread << " px\n";
            }
        }

        // Log trajectory
        traj_out << std::fixed << std::setprecision(4)
                 << est.mean_pose.x << " "
                 << est.mean_pose.y << " "
                 << est.mean_pose.z << " "
                 << est.variance.x << " "
                 << est.variance.y << " "
                 << est.variance.z << " "
                 << (est.mean_pose.yaw * 180.0 / M_PI) << " "
                 << std::sqrt(est.variance.yaw) * 180.0 / M_PI << " "
                 << est.neff << " "
                 << elapsed_sec << "\n";

        // Export particle cloud for video rendering
        if (config.save_particles && particles_out.is_open()) {
            auto exported = pf.getExportParticles(config.export_particles);
            for (size_t pid = 0; pid < exported.size(); ++pid) {
                particles_out << f << ","
                              << pid << ","
                              << std::fixed << std::setprecision(2)
                              << exported[pid].pose.x << ","
                              << exported[pid].pose.y << ","
                              << exported[pid].pose.z << ","
                              << (exported[pid].pose.yaw * 180.0 / M_PI) << ","
                              << std::scientific << std::setprecision(6)
                              << exported[pid].weight << "\n";
            }
        }

        // Compute error if Ground Truth available
        if (frame.has_ground_truth) {
            double scale_m = loader.getPixelToMeter();
            double dx_m = (est.mean_pose.x - frame.ground_truth.x) * scale_m;
            double dy_m = (est.mean_pose.y - frame.ground_truth.y) * scale_m;
            double pos_err = std::sqrt(dx_m * dx_m + dy_m * dy_m);

            double yaw_diff = est.mean_pose.yaw - frame.ground_truth.yaw_rad;
            while (yaw_diff > M_PI) yaw_diff -= 2.0 * M_PI;
            while (yaw_diff < -M_PI) yaw_diff += 2.0 * M_PI;
            double yaw_err_deg = std::abs(yaw_diff) * 180.0 / M_PI;

            position_errors.push_back(pos_err);
            yaw_errors.push_back(yaw_err_deg);

            error_out << f << " "
                      << est.mean_pose.x << " " << est.mean_pose.y << " " << est.mean_pose.z << " "
                      << frame.ground_truth.x << " " << frame.ground_truth.y << " " << frame.ground_truth.z << " "
                      << pos_err << " " << yaw_err_deg << "\n";

            if (!config.quiet && (f % 5 == 0 || f == loader.size() - 1)) {
                std::cout << "Frame [" << std::setw(3) << f << "/" << loader.size() << "] "
                          << "PosErr: " << std::fixed << std::setprecision(2) << pos_err << " m | "
                          << "YawErr: " << yaw_err_deg << " deg | "
                          << "Neff(pre): " << std::setprecision(0) << est.neff << " | ";
                if (config.two_phase) std::cout << "Phase: " << (tracking ? "T" : "G") << " | ";
                if (config.inject_ratio > 0.0) {
                    std::cout << "Inj: " << pf.getLastInjectedCount() << " | C: " << std::setprecision(2)
                              << pf.getObservationContrast() << " | ";
                }
                std::cout << "Time: " << std::setprecision(3) << elapsed_sec << " s\n";
            }
        } else {
            if (!config.quiet && (f % 5 == 0 || f == loader.size() - 1)) {
                std::cout << "Frame [" << std::setw(3) << f << "/" << loader.size() << "] "
                          << "Pose: (" << std::fixed << std::setprecision(1)
                          << est.mean_pose.x << ", " << est.mean_pose.y << ") | "
                          << "Yaw: " << (est.mean_pose.yaw * 180.0 / M_PI) << " deg | "
                          << "Time: " << std::setprecision(3) << elapsed_sec << " s\n";
            }
        }

    }

    auto end_all = std::chrono::high_resolution_clock::now();
    double total_sec = std::chrono::duration<double>(end_all - start_all).count();

    traj_out.close();
    error_out.close();
    if (phase_out.is_open()) phase_out.close();
    if (particles_out.is_open()) particles_out.close();

    std::cout << "\n================ Localization Summary ================\n";
    std::cout << "Total Frames Processed: " << loader.size() << "\n";
    std::cout << "Total Execution Time:  " << std::fixed << std::setprecision(2) << total_sec << " s ("
              << (total_sec / loader.size()) << " s/frame)\n";

    if (!position_errors.empty()) {
        double sum_err = 0.0;
        double sum_sq_err = 0.0;
        for (double e : position_errors) {
            sum_err += e;
            sum_sq_err += e * e;
        }
        double mean_err = sum_err / position_errors.size();
        double rmse = std::sqrt(sum_sq_err / position_errors.size());

        std::cout << "Mean Position Error:   " << std::fixed << std::setprecision(2) << mean_err << " m\n";
        std::cout << "RMSE Position Error:   " << rmse << " m\n";
        std::cout << "Final Position Error:  " << position_errors.back() << " m\n";
    }

    std::cout << "Results saved to: " << config.output_dir << "\n";
    std::cout << "======================================================\n";

    return 0;
}
