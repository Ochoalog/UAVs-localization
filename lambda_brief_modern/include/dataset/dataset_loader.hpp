#pragma once

#include "core/types.hpp"
#include <opencv2/core.hpp>
#include <string>
#include <vector>
#include <optional>
#include <filesystem>

namespace lambda_brief {

struct DatasetFrame {
    size_t index{0};
    std::string rgb_path;
    std::string nir_path;
    std::string red_path;
    std::string reg_path;
    std::string gre_path;

    bool has_ground_truth{false};
    GroundTruthSample ground_truth;

    // Lazy image loaders
    cv::Mat loadRGB() const;
    cv::Mat loadNIR() const;
    cv::Mat loadRED() const;
    cv::Mat loadREG() const;
    cv::Mat loadGRE() const;
};

struct DatasetLoaderOptions {
    std::string dataset_dir;
    std::string traj_file;       // Optional custom traj.txt
    std::string ground_truth_file; // Optional custom traj_truth.txt
    int start_frame{-1};         // -1 for full dataset
    int end_frame{-1};           // -1 for full dataset
    double pixel_to_meter{0.188679245}; // Default ADM scale
    ImagePreprocessFilter ms_filter{ImagePreprocessFilter::NONE};
    int bilateral_d{6};
    double bilateral_sigma_color{5.0};
    double bilateral_sigma_space{5.0};
    int gt_offset{-1};           // Ground truth offset in traj.txt (-1 for auto-detection)
};

class DatasetLoader {
public:
    explicit DatasetLoader(const DatasetLoaderOptions& options);

    /**
     * @brief Load and index all frames, ground truth, and attitude data
     */
    bool load();

    size_t size() const { return frames_.size(); }
    const DatasetFrame& getFrame(size_t idx) const { return frames_.at(idx); }
    const std::vector<DatasetFrame>& getAllFrames() const { return frames_; }
    double getPixelToMeter() const { return options_.pixel_to_meter; }

    /**
     * @brief Apply configured preprocessing (e.g. bilateral filter) to a multispectral band
     */
    cv::Mat preprocessBand(const cv::Mat& band) const;

private:
    DatasetLoaderOptions options_;
    std::vector<DatasetFrame> frames_;

    std::string resolvePath(const std::string& raw_path, const std::string& subfolder) const;
    bool loadGroundTruth();
};

} // namespace lambda_brief
