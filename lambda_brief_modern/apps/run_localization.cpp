#include "config/config.hpp"
#include "core/vegetation_index.hpp"
#include "core/mask_generator.hpp"
#include "core/pixel_sampler.hpp"
#include "core/descriptor.hpp"
#include "localization/visual_odometry.hpp"
#include "localization/particle_filter.hpp"
#include "dataset/dataset_loader.hpp"

#include <opencv2/imgcodecs.hpp>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <filesystem>

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
    cv::Mat satellite_map = cv::imread(config.map_path, cv::IMREAD_COLOR);
    if (satellite_map.empty()) {
        std::cerr << "Error: Could not load satellite map from: " << config.map_path << std::endl;
        return 1;
    }
    std::cout << "[Map] Loaded satellite map: " << satellite_map.cols << "x"
              << satellite_map.rows << " (" << config.map_path << ")\n";

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
    pf.initializeUniform(satellite_map.size());

    VisualOdometry vo(FeatureType::SIFT, 2000);
    PixelSampler sampler(config.random_seed);

    cv::Mat prev_rgb;
    std::vector<double> position_errors;
    std::vector<double> yaw_errors;

    std::cout << "\nStarting Monte Carlo Localization with " << pf.getNumParticles()
              << " particles over " << loader.size() << " frames...\n";

    auto start_all = std::chrono::high_resolution_clock::now();

    for (size_t f = 0; f < loader.size(); ++f) {
        auto t_frame_start = std::chrono::high_resolution_clock::now();
        const auto& frame = loader.getFrame(f);

        cv::Mat rgb = frame.loadRGB();
        if (rgb.empty()) {
            std::cerr << "Warning: Could not read RGB image at frame " << f << std::endl;
            continue;
        }

        cv::Mat nir = loader.preprocessBand(frame.loadNIR());
        cv::Mat red = loader.preprocessBand(frame.loadRED());
        cv::Mat reg = loader.preprocessBand(frame.loadREG());
        cv::Mat gre = loader.preprocessBand(frame.loadGRE());

        // A. Odometry & Motion prediction (for f > 0)
        Pose rel_motion;
        bool is_reliable = false;

        if (f > 0 && !prev_rgb.empty()) {
            auto [motion, reliable] = vo.estimateRelativeMotion(prev_rgb, rgb);
            rel_motion = motion;
            is_reliable = reliable;
            pf.predict(rel_motion, is_reliable);
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
        pf.updateWeights(
            satellite_map,
            rgb.size(),
            pairs,
            drone_desc,
            config.color_mode
        );

        pf.resampleIfNeeded();

        // F. State Estimation and Logging
        auto t_frame_end = std::chrono::high_resolution_clock::now();
        double elapsed_sec = std::chrono::duration<double>(t_frame_end - t_frame_start).count();

        EstimationState est = pf.getEstimationState();
        est.frame_index = f;
        est.elapsed_seconds = elapsed_sec;

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
                          << "Neff: " << std::setprecision(0) << est.neff << " | "
                          << "Time: " << std::setprecision(3) << elapsed_sec << " s\n";
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

        prev_rgb = rgb;
    }

    auto end_all = std::chrono::high_resolution_clock::now();
    double total_sec = std::chrono::duration<double>(end_all - start_all).count();

    traj_out.close();
    error_out.close();
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
