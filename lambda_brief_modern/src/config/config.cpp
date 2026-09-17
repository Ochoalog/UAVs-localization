#include "config/config.hpp"
#include <iostream>
#include <cstring>
#include <algorithm>

namespace lambda_brief {

ParticleFilterConfig AppConfig::toParticleFilterConfig() const {
    ParticleFilterConfig pfc;
    pfc.num_particles = num_particles;
    pfc.random_seed = random_seed;
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
              << "  --start_frame <idx>     Start frame index (-1 for start of dataset)\n"
              << "  --end_frame <idx>       End frame index (-1 for end of dataset)\n"
              << "  --pixel_to_meter <val>  Meters per pixel scale (default: 0.188679245)\n"
              << "  --ms_filter <filter>    Multispectral filter: none, gaussian, bilateral (default: none)\n"
              << "  --bf_d <val>            Bilateral filter diameter (default: 6)\n"
              << "  --bf_sc <val>           Bilateral filter sigmaColor (default: 5.0)\n"
              << "  --bf_ss <val>           Bilateral filter sigmaSpace (default: 5.0)\n"
              << "  --save_particles        Export particle cloud to particles_log.csv for video rendering\n"
              << "  --export_particles <N>  Max particles to export per frame (default: 1000, -1 for all)\n"
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
        } else if (arg == "--save_particles") {
            config.save_particles = true;
        } else if (arg == "--export_particles" && i + 1 < argc) {
            config.export_particles = std::stoi(argv[++i]);
        } else if (arg == "--seed" && i + 1 < argc) {
            config.random_seed = std::stoul(argv[++i]);
        } else if (arg == "--quiet") {
            config.quiet = true;
        }
    }

    return config;
}

} // namespace lambda_brief
