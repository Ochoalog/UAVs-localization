#include "config/config.hpp"
#include <iostream>
#include <cstring>
#include <algorithm>

namespace lambda_brief {

ParticleFilterConfig AppConfig::toParticleFilterConfig() const {
    ParticleFilterConfig pfc;
    pfc.num_particles = num_particles;
    pfc.random_seed = random_seed;
    pfc.min_scale = min_scale;
    pfc.max_scale = max_scale;
    pfc.var_brief = var_brief;
    pfc.resample_neff_ratio = resample_neff_ratio;
    pfc.pos_noise_m = pos_noise_px;
    pfc.inject_ratio = inject_ratio;
    pfc.inject_spread_px = inject_spread_px;
    pfc.inject_gate = inject_gate;
    pfc.motion_noise_frac = motion_noise_frac;
    pfc.mode_estimate = (estimator == "mode");
    pfc.mode_radius_px = mode_radius_px;
    return pfc;
}

DatasetLoaderOptions AppConfig::toDatasetLoaderOptions() const {
    DatasetLoaderOptions dlo;
    dlo.dataset_dir = dataset_dir;
    dlo.traj_file = traj_file;
    dlo.ground_truth_file = ground_truth_file;
    dlo.start_frame = start_frame;
    dlo.end_frame = end_frame;
    dlo.pixel_to_meter = pixel_to_meter;
    dlo.ms_filter = ms_filter;
    dlo.bilateral_d = bilateral_d;
    dlo.bilateral_sigma_color = bilateral_sigma_color;
    dlo.bilateral_sigma_space = bilateral_sigma_space;
    dlo.gt_offset = gt_offset;
    return dlo;
}

void AppConfig::printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options]\n\n"
              << "Options:\n"
              << "  --map <path>            Path to global satellite map image (required)\n"
              << "  --dataset_dir <path>    Path to dataset flight directory (required)\n"
              << "  --output_dir <path>     Path to output directory for results (default: ./results)\n"
              << "  --traj <path>           Custom traj.txt path (optional)\n"
              << "  --gt <path>             Custom ground truth file (optional)\n"
              << "  --gt_offset <idx>       Ground truth line 0 offset in traj.txt (-1 for auto, e.g. 84 for ADM/0036)\n"
              << "  --num_particles <N>     Number of particles (default: 50000)\n"
              << "  --alpha <val>           Vegetation mask weighting factor alpha [0..1] (default: 0.5)\n"
              << "  --vi <type>             Vegetation index: NDVI, NDRE, NDVI_RED_RE, GRVI (default: NDVI)\n"
              << "  --pairs <N>             Number of pixel pairs (default: 256)\n"
              << "  --color_mode <mode>     Color mode: LAB_NO_L, LAB, GRAY (default: LAB_NO_L)\n"
              << "  --pair_pattern <p>      How the pixel pairs are chosen (default: roulette):\n"
              << "                          roulette        roulette over the alpha-combined mask, new pairs every frame\n"
              << "                          roulette_fixed  the same, drawn once on the first frame and kept\n"
              << "                          uniform         uniform over the image, new pairs every frame (ignores --alpha)\n"
              << "                          legacy_ab       abBRIEF of the legacy code: one fixed, near-uniform pattern,\n"
              << "                                          same cv::RNG sequence as BriefHeuristic.cpp:1132-1154 (ignores --alpha, --seed)\n"
              << "  --stability_map <png>   v3: per-pair weight raster (8-bit, map size, 255 = 1). Default: off\n"
              << "  --stability_formula <f> v3: normalized (sum w*e / sum w, default) or coin (unstable pair = 0.5)\n"
              << "  --start_frame <idx>     Start frame index (-1 for start of dataset)\n"
              << "  --end_frame <idx>       End frame index (-1 for end of dataset)\n"
              << "  --pixel_to_meter <val>  Meters per pixel scale (default: 0.188679245)\n"
              << "  --ms_filter <filter>    Multispectral filter: none, gaussian, bilateral (default: none)\n"
              << "  --bf_d <val>            Bilateral filter diameter (default: 6)\n"
              << "  --bf_sc <val>           Bilateral filter sigmaColor (default: 5.0)\n"
              << "  --bf_ss <val>           Bilateral filter sigmaSpace (default: 5.0)\n"
              << "  --quantize <L>          Colour quantisation levels for map+drone RGB (default 25 = legacy -mappp 1; 0 = off)\n"
              << "  --min_scale <v>         Minimum particle scale z (default 0.1; UFRGS legacy 0.5)\n"
              << "  --max_scale <v>         Maximum particle scale z (default 3.0; UFRGS legacy 5.0)\n"
              << "  --var_brief <v>         Variance of the Gaussian lambda-BRIEF likelihood (default 0.0225)\n"
              << "  --resample_neff <r>     Resample when Neff < r*N; r>=1 resamples every frame (default 1.0, legacy)\n"
              << "  --pos_noise <px>        Uniform +-noise on x,y per prediction step (default 10)\n"
              << "  --vo_scale_deadband <e> VO scale change |dz-1| < e is snapped to 1 (default 0.01 = legacy; 0 = off)\n"
              << "  --sift_features <N>     Max SIFT features for visual odometry (default 0 = unlimited)\n"
              << "  --odom_file <path>      Use pre-recorded odometry (legacy traj/odom-*.txt) instead of online VO\n"
              << "  --apply_unreliable_odom Same as --unreliable_motion apply (kept for old scripts)\n"
              << "  --unreliable_motion <m> Motion used when the homography fails the checks: zero (legacy),\n"
              << "                          cv (constant velocity from the last reliable steps), apply\n"
              << "  --vo_matcher <m>        legacy (1-NN + min_dist heuristic, default) or ratio (Lowe + mutual check)\n"
              << "  --vo_ratio <r>          Lowe ratio for --vo_matcher ratio (default 0.75)\n"
              << "  --vo_robust <m>         ransac (default, legacy) or magsac (cv::USAC_MAGSAC)\n"
              << "  --vo_reproj <px>        Homography inlier threshold (default 3.0)\n"
              << "  --vo_min_inliers <n>    Mark transitions with fewer homography inliers as unreliable (default 0 = off)\n"
              << "  --scale_prior_sigma <s> Soft prior on z (altimeter emulation) with relative sigma s (e.g. 0.10)\n"
              << "  --scale_prior_gain <g>  Prior centre z_ref = g * GT z (default 1.0; ADM/0036 RGBres: 0.87)\n"
              << "  --altimeter_file <path> Real altimeter per frame (altimeter_baro.txt from scripts/extract_altimeter.py):\n"
              << "                          the scale prior uses g * z_baro instead of the GT z\n"
              << "  --inject_ratio <r>      Fraction of random particles injected every frame (recovery; e.g. 0.05)\n"
              << "  --motion_noise <f>      Extra +-noise = f * |odometry step| on x,y (e.g. 0.25; default 0)\n"
              << "  --inject_gate <c>       Inject only in informative frames (observation contrast >= c), e.g. 0.8\n"
              << "  --estimator <mean|mode> Pose estimate: cloud mean (default) or densest mode\n"
              << "  --mode_radius <px>      Cell/kernel size of the mode estimator (default 100)\n"
              << "  --inject_spread <px>    Scale the injection by cloud std / px (tight cloud -> ~no injection), e.g. 300\n"
              << "  --yaw_prior_sigma <deg> Soft prior on yaw around the GT heading (compass emulation), e.g. 15\n"
              << "  --two_phase             Priors/injection only until convergence, then legacy tracking\n"
              << "  --converge_std <px>     Two-phase: convergence when cloud spread < px (default 100)\n"
              << "  --converge_frames <n>   Two-phase: ... for n consecutive frames (default 5)\n"
              << "  --converge_contrast <c> Two-phase: ... and observation contrast >= c (default 1.3; 0 = ignore)\n"
              << "  --relocalize_std <px>   Two-phase: back to GLOBAL when spread > px (default 0 = never)\n"
              << "  --relocalize_frames <n> Two-phase: ... for n consecutive frames (default 5)\n"
              << "  --track_vo_scale_deadband <e>  VO scale dead-band in TRACKING (default 0.01 = legacy)\n"
              << "  --init_around_gt <px>   Diagnostic: init the cloud as a Gaussian around the 1st frame's GT\n"
              << "                          (sigma_xy=px, sigma_yaw=5deg, z~N(0.87*z_GT,5%)) instead of uniform\n"
              << "  --save_particles        Export particle cloud to particles_log.csv for video rendering\n"
              << "  --export_particles <N>  Max particles to export per frame (default: 1000, -1 for all)\n"
              << "  --save_pairs            Export the pixel pairs of every frame (pairs_log.csv, frames_log.csv) and the\n"
              << "                          sampling mask / drone NDVI (amostragem/*.png) for scripts/v3_video.py\n"
              << "  --seed <val>            Random seed for reproducibility (default: 42)\n"
              << "  --quiet                 Run without verbose console output\n"
              << "  -h, --help              Show this help message\n";
}

AppConfig AppConfig::parseCommandLine(int argc, char* argv[]) {
    AppConfig config;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            std::exit(0);
        } else if (arg == "--map" && i + 1 < argc) {
            config.map_path = argv[++i];
        } else if (arg == "--dataset_dir" && i + 1 < argc) {
            config.dataset_dir = argv[++i];
        } else if (arg == "--output_dir" && i + 1 < argc) {
            config.output_dir = argv[++i];
        } else if (arg == "--traj" && i + 1 < argc) {
            config.traj_file = argv[++i];
        } else if (arg == "--gt" && i + 1 < argc) {
            config.ground_truth_file = argv[++i];
        } else if (arg == "--gt_offset" && i + 1 < argc) {
            config.gt_offset = std::stoi(argv[++i]);
        } else if (arg == "--num_particles" && i + 1 < argc) {
            config.num_particles = std::stoul(argv[++i]);
        } else if (arg == "--alpha" && i + 1 < argc) {
            config.alpha = std::stod(argv[++i]);
        } else if (arg == "--vi" && i + 1 < argc) {
            std::string vi_str = argv[++i];
            std::transform(vi_str.begin(), vi_str.end(), vi_str.begin(), ::toupper);
            if (vi_str == "NDVI") config.vi_type = VegetationIndexType::NDVI;
            else if (vi_str == "NDRE") config.vi_type = VegetationIndexType::NDRE;
            else if (vi_str == "NDVI_RED_RE") config.vi_type = VegetationIndexType::NDVI_RED_RE;
            else if (vi_str == "GRVI") config.vi_type = VegetationIndexType::GRVI;
            else std::cerr << "Warning: Unknown VI type '" << vi_str << "', defaulting to NDVI\n";
        } else if (arg == "--pairs" && i + 1 < argc) {
            config.num_pairs = std::stoul(argv[++i]);
        } else if (arg == "--color_mode" && i + 1 < argc) {
            std::string cm_str = argv[++i];
            std::transform(cm_str.begin(), cm_str.end(), cm_str.begin(), ::toupper);
            if (cm_str == "LAB_NO_L") config.color_mode = DescriptorColorMode::LAB_NO_L;
            else if (cm_str == "LAB") config.color_mode = DescriptorColorMode::LAB;
            else if (cm_str == "GRAY") config.color_mode = DescriptorColorMode::GRAY;
        } else if (arg == "--pair_pattern" && i + 1 < argc) {
            std::string pp = argv[++i];
            std::transform(pp.begin(), pp.end(), pp.begin(), ::tolower);
            if (pp == "roulette") config.sampler_method = PixelSamplerMethod::ROULETTE_STOCHASTIC;
            else if (pp == "roulette_fixed") config.sampler_method = PixelSamplerMethod::ROULETTE_FIXED;
            else if (pp == "uniform") config.sampler_method = PixelSamplerMethod::UNIFORM;
            else if (pp == "legacy_ab") config.sampler_method = PixelSamplerMethod::LEGACY_AB;
            else std::cerr << "Warning: unknown --pair_pattern '" << pp << "', using roulette\n";
        } else if (arg == "--stability_map" && i + 1 < argc) {
            config.stability_map = argv[++i];
        } else if (arg == "--stability_formula" && i + 1 < argc) {
            std::string sf = argv[++i];
            std::transform(sf.begin(), sf.end(), sf.begin(), ::tolower);
            if (sf == "normalized") config.stability_formula = StabilityFormula::NORMALIZED;
            else if (sf == "coin") config.stability_formula = StabilityFormula::COIN;
            else std::cerr << "Warning: unknown --stability_formula '" << sf << "', using normalized\n";
        } else if (arg == "--start_frame" && i + 1 < argc) {
            config.start_frame = std::stoi(argv[++i]);
        } else if (arg == "--end_frame" && i + 1 < argc) {
            config.end_frame = std::stoi(argv[++i]);
        } else if (arg == "--pixel_to_meter" && i + 1 < argc) {
            config.pixel_to_meter = std::stod(argv[++i]);
        } else if (arg == "--ms_filter" && i + 1 < argc) {
            std::string f_str = argv[++i];
            std::transform(f_str.begin(), f_str.end(), f_str.begin(), ::tolower);
            if (f_str == "bilateral") config.ms_filter = ImagePreprocessFilter::BILATERAL;
            else if (f_str == "gaussian") config.ms_filter = ImagePreprocessFilter::GAUSSIAN;
            else config.ms_filter = ImagePreprocessFilter::NONE;
        } else if (arg == "--bf_d" && i + 1 < argc) {
            config.bilateral_d = std::stoi(argv[++i]);
        } else if (arg == "--bf_sc" && i + 1 < argc) {
            config.bilateral_sigma_color = std::stod(argv[++i]);
        } else if (arg == "--bf_ss" && i + 1 < argc) {
            config.bilateral_sigma_space = std::stod(argv[++i]);
        } else if (arg == "--quantize" && i + 1 < argc) {
            config.quantize_levels = std::stoi(argv[++i]);
        } else if (arg == "--min_scale" && i + 1 < argc) {
            config.min_scale = std::stod(argv[++i]);
        } else if (arg == "--max_scale" && i + 1 < argc) {
            config.max_scale = std::stod(argv[++i]);
        } else if (arg == "--var_brief" && i + 1 < argc) {
            config.var_brief = std::stod(argv[++i]);
        } else if (arg == "--resample_neff" && i + 1 < argc) {
            config.resample_neff_ratio = std::stod(argv[++i]);
        } else if (arg == "--pos_noise" && i + 1 < argc) {
            config.pos_noise_px = std::stod(argv[++i]);
        } else if (arg == "--vo_scale_deadband" && i + 1 < argc) {
            config.vo_scale_deadband = std::stod(argv[++i]);
        } else if (arg == "--sift_features" && i + 1 < argc) {
            config.sift_features = std::stoi(argv[++i]);
        } else if (arg == "--odom_file" && i + 1 < argc) {
            config.odom_file = argv[++i];
        } else if (arg == "--scale_prior_sigma" && i + 1 < argc) {
            config.scale_prior_sigma = std::stod(argv[++i]);
        } else if (arg == "--motion_noise" && i + 1 < argc) {
            config.motion_noise_frac = std::stod(argv[++i]);
        } else if (arg == "--inject_gate" && i + 1 < argc) {
            config.inject_gate = std::stod(argv[++i]);
        } else if (arg == "--estimator" && i + 1 < argc) {
            config.estimator = argv[++i];
            if (config.estimator != "mean" && config.estimator != "mode") {
                std::cerr << "Warning: --estimator must be 'mean' or 'mode'; using 'mean'\n";
                config.estimator = "mean";
            }
        } else if (arg == "--mode_radius" && i + 1 < argc) {
            config.mode_radius_px = std::stod(argv[++i]);
        } else if (arg == "--inject_spread" && i + 1 < argc) {
            config.inject_spread_px = std::stod(argv[++i]);
        } else if (arg == "--inject_ratio" && i + 1 < argc) {
            config.inject_ratio = std::stod(argv[++i]);
        } else if (arg == "--yaw_prior_sigma" && i + 1 < argc) {
            config.yaw_prior_sigma_deg = std::stod(argv[++i]);
        } else if (arg == "--scale_prior_gain" && i + 1 < argc) {
            config.scale_prior_gain = std::stod(argv[++i]);
        } else if (arg == "--altimeter_file" && i + 1 < argc) {
            config.altimeter_file = argv[++i];
        } else if (arg == "--two_phase") {
            config.two_phase = true;
        } else if (arg == "--converge_std" && i + 1 < argc) {
            config.converge_std_px = std::stod(argv[++i]);
        } else if (arg == "--converge_frames" && i + 1 < argc) {
            config.converge_frames = std::stoi(argv[++i]);
        } else if (arg == "--converge_contrast" && i + 1 < argc) {
            config.converge_contrast = std::stod(argv[++i]);
        } else if (arg == "--relocalize_std" && i + 1 < argc) {
            config.relocalize_std_px = std::stod(argv[++i]);
        } else if (arg == "--relocalize_frames" && i + 1 < argc) {
            config.relocalize_frames = std::stoi(argv[++i]);
        } else if (arg == "--track_vo_scale_deadband" && i + 1 < argc) {
            config.track_vo_scale_deadband = std::stod(argv[++i]);
        } else if (arg == "--init_around_gt" && i + 1 < argc) {
            config.init_around_gt_sigma_px = std::stod(argv[++i]);
        } else if (arg == "--apply_unreliable_odom") {
            config.unreliable_motion = "apply";
        } else if (arg == "--unreliable_motion" && i + 1 < argc) {
            config.unreliable_motion = argv[++i];
            if (config.unreliable_motion != "zero" && config.unreliable_motion != "cv" &&
                config.unreliable_motion != "apply") {
                std::cerr << "Warning: --unreliable_motion must be zero, cv or apply; using zero\n";
                config.unreliable_motion = "zero";
            }
        } else if (arg == "--vo_matcher" && i + 1 < argc) {
            config.vo_matcher = argv[++i];
            if (config.vo_matcher != "legacy" && config.vo_matcher != "ratio") {
                std::cerr << "Warning: --vo_matcher must be legacy or ratio; using legacy\n";
                config.vo_matcher = "legacy";
            }
        } else if (arg == "--vo_ratio" && i + 1 < argc) {
            config.vo_ratio = std::stod(argv[++i]);
        } else if (arg == "--vo_robust" && i + 1 < argc) {
            config.vo_robust = argv[++i];
            if (config.vo_robust != "ransac" && config.vo_robust != "magsac") {
                std::cerr << "Warning: --vo_robust must be ransac or magsac; using ransac\n";
                config.vo_robust = "ransac";
            }
        } else if (arg == "--vo_reproj" && i + 1 < argc) {
            config.vo_reproj_px = std::stod(argv[++i]);
        } else if (arg == "--vo_min_inliers" && i + 1 < argc) {
            config.vo_min_inliers = std::stoi(argv[++i]);
        } else if (arg == "--save_pairs") {
            config.save_pairs = true;
        } else if (arg == "--save_particles") {
            config.save_particles = true;
        } else if (arg == "--export_particles" && i + 1 < argc) {
            config.export_particles = std::stoi(argv[++i]);
        } else if (arg == "--seed" && i + 1 < argc) {
            config.random_seed = std::stoul(argv[++i]);
        } else if (arg == "--quiet") {
            config.quiet = true;
        } else {
            std::cerr << "Warning: unknown or incomplete option '" << arg << "' ignored\n";
        }
    }

    return config;
}

} // namespace lambda_brief
