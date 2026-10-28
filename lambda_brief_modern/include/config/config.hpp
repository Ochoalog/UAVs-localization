#pragma once

#include "core/types.hpp"
#include "localization/particle_filter.hpp"
#include "dataset/dataset_loader.hpp"
#include <string>

namespace lambda_brief {

struct AppConfig {
    std::string map_path;
    std::string dataset_dir;
    std::string output_dir{"./results"};
    std::string traj_file;
    std::string ground_truth_file;

    size_t num_particles{50000};
    double alpha{0.5};
    VegetationIndexType vi_type{VegetationIndexType::NDVI};
    size_t num_pairs{256};
    DescriptorColorMode color_mode{DescriptorColorMode::LAB_NO_L};
    PixelSamplerMethod sampler_method{PixelSamplerMethod::ROULETTE_STOCHASTIC};

    int start_frame{-1};
    int end_frame{-1};
    double pixel_to_meter{0.188679245};

    ImagePreprocessFilter ms_filter{ImagePreprocessFilter::NONE};
    int bilateral_d{6};
    double bilateral_sigma_color{5.0};
    double bilateral_sigma_space{5.0};
    int gt_offset{-1};

    // --- Legacy-fidelity / filter parameters (see docs/diagnostico_convergencia.md)
    int quantize_levels{25};          // legacy "-mappp 1": 25-level colour quantisation (0 = off)
    double min_scale{0.1};            // legacy "-minsc" (ADM: 0.1, UFRGS: 0.5)
    double max_scale{3.0};            // legacy "-maxsc" (ADM: 3.0, UFRGS: 5.0)
    double var_brief{0.0225};         // sigma_brief^2 (legacy 0.15^2)
    double resample_neff_ratio{1.0};  // >=1.0 resamples every frame (legacy behaviour)
    double pos_noise_px{10.0};        // uniform +-noise on x,y per step (legacy 10 px)
    double motion_noise_frac{0.0};    // extra +-noise = frac * |odometry step| (map px); 0 = legacy
    double inject_ratio{0.0};         // fraction of random particles injected per frame (0 = legacy)
    double inject_spread_px{0.0};     // >0: injection scaled by cloud std / this (px)
    double inject_gate{0.0};          // >0: inject only when the frame contrast C >= this (informative frame)
    std::string estimator{"mean"};    // "mean" (whole cloud, legacy-like) or "mode" (densest cluster)
    double mode_radius_px{100.0};     // grid cell / kernel size of the mode estimator (px)
    double vo_scale_deadband{0.01};   // |dz-1| below this -> dz = 1 (legacy 0.01; 0 = keep all)
    int sift_features{0};             // 0 = unlimited, as legacy cv::xfeatures2d::SIFT::create()
    std::string odom_file;            // optional pre-recorded odometry (x y z yaw reliable per line)
    // What predict() receives when the homography fails the checks:
    //   "zero"  = legacy (u = 0, only the wide unreliable noise)
    //   "cv"    = constant velocity (EMA of the last reliable steps, dz = 1)
    //   "apply" = the rejected homography motion (old --apply_unreliable_odom)
    std::string unreliable_motion{"zero"};
    // Visual odometry front-end (defaults = legacy DroneRobot::findOdometryUsingFeaturesSift)
    std::string vo_matcher{"legacy"}; // "legacy" (min_dist heuristic) or "ratio" (Lowe + mutual check)
    double vo_ratio{0.75};            // Lowe ratio for vo_matcher=ratio
    std::string vo_robust{"ransac"};  // "ransac" or "magsac"
    double vo_reproj_px{3.0};         // homography inlier threshold (px)
    int vo_min_inliers{0};            // >0: fewer inliers => unreliable
    double scale_prior_sigma{0.0};    // >0: soft prior on z around gain * GT scale (altimeter emulation)
    double yaw_prior_sigma_deg{0.0};  // >0: soft prior on yaw around the GT heading (compass emulation)
    double scale_prior_gain{1.0};     // z_ref = gain * GT z. ADM/0036 RGBres: 0.87 (see docs, sec. 2.7)
    // Real altimeter (scripts/extract_altimeter.py -> <dataset>/altimeter_baro.txt: "traj_idx z ..." per line).
    // When set, the scale prior is centred on gain * z_baro of the frame instead of gain * GT z,
    // so the filter no longer reads the ground truth at run time. Frames missing from the file get no prior.
    std::string altimeter_file;
    // Diagnostic only (default 0 = off, legacy uniform init unchanged): initialize the cloud as a
    // Gaussian around the GT of the first frame of the slice (sigma_xy = this value in px,
    // sigma_yaw = 5 deg, z ~ N(0.87*z_GT, 5% of 0.87*z_GT)). See docs/analise_fim_do_voo_adm2.md.
    double init_around_gt_sigma_px{0.0};

    // Two-phase filter (include/localization/phase_controller.hpp). GLOBAL phase: priors,
    // injection and vo_scale_deadband as configured above. TRACKING phase (after convergence):
    // no priors, no injection, VO dead-band = track_vo_scale_deadband (legacy 0.01).
    bool two_phase{false};
    double converge_std_px{100.0};    // cloud spread sqrt(varX+varY) below this ...
    int converge_frames{5};           // ... for this many consecutive frames ...
    double converge_contrast{1.3};    // ... with observation contrast C >= this (0 = ignore C)
    double relocalize_std_px{0.0};    // TRACKING -> GLOBAL when spread > this (0 = never)
    int relocalize_frames{5};         // ... for this many consecutive frames
    double track_vo_scale_deadband{0.01};

    // v3 (Sentinel-2 stability weight, docs/MANUAL_TESTES_ARTIGO.md sec. 10). Empty = off (v2 unchanged).
    // 8-bit grayscale PNG with the same size as the map; 255 = weight 1. Each pixel pair is weighted by
    // min(W(A), W(B)) at the two projected map points (scripts/make_stability_map.py builds the raster).
    std::string stability_map;
    StabilityFormula stability_formula{StabilityFormula::NORMALIZED};

    bool save_particles{false};
    int export_particles{1000}; // Number of particles to export per frame (or -1 for all)
    // Diagnostics for videos (scripts/v3_video.py): pairs_log.csv, frames_log.csv and amostragem/
    // (sampling mask and drone NDVI per frame, half resolution). Read-only: results are unchanged.
    bool save_pairs{false};

    unsigned int random_seed{42};
    bool quiet{false};

    ParticleFilterConfig toParticleFilterConfig() const;
    DatasetLoaderOptions toDatasetLoaderOptions() const;

    static AppConfig parseCommandLine(int argc, char* argv[]);
    static void printUsage(const char* program_name);
};

} // namespace lambda_brief
