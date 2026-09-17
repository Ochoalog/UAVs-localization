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

    bool save_particles{false};
    int export_particles{1000}; // Number of particles to export per frame (or -1 for all)

    unsigned int random_seed{42};
    bool quiet{false};

    ParticleFilterConfig toParticleFilterConfig() const;
    DatasetLoaderOptions toDatasetLoaderOptions() const;

    static AppConfig parseCommandLine(int argc, char* argv[]);
    static void printUsage(const char* program_name);
};

} // namespace lambda_brief
