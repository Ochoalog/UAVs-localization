#include "dataset/dataset_loader.hpp"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <regex>

namespace lambda_brief {

namespace fs = std::filesystem;

cv::Mat DatasetFrame::loadRGB() const {
    if (rgb_path.empty()) return cv::Mat();
    return cv::imread(rgb_path, cv::IMREAD_COLOR);
}

cv::Mat DatasetFrame::loadNIR() const {
    if (nir_path.empty()) return cv::Mat();
    return cv::imread(nir_path, cv::IMREAD_GRAYSCALE);
}

cv::Mat DatasetFrame::loadRED() const {
    if (red_path.empty()) return cv::Mat();
    return cv::imread(red_path, cv::IMREAD_GRAYSCALE);
}

cv::Mat DatasetFrame::loadREG() const {
    if (reg_path.empty()) return cv::Mat();
    return cv::imread(reg_path, cv::IMREAD_GRAYSCALE);
}

cv::Mat DatasetFrame::loadGRE() const {
    if (gre_path.empty()) return cv::Mat();
    return cv::imread(gre_path, cv::IMREAD_GRAYSCALE);
}

DatasetLoader::DatasetLoader(const DatasetLoaderOptions& options)
    : options_(options) {}

std::string DatasetLoader::resolvePath(const std::string& raw_path, const std::string& subfolder) const {
    if (raw_path.empty()) return "";

    // 1. Direct check
    if (fs::exists(raw_path)) {
        return fs::canonical(raw_path).string();
    }

    // 2. Extract basename
    std::string filename = fs::path(raw_path).filename().string();
    fs::path base_dir(options_.dataset_dir);

    // 3. Check inside base_dir / subfolder / filename
    fs::path candidate1 = base_dir / subfolder / filename;
    if (fs::exists(candidate1)) {
        return fs::canonical(candidate1).string();
    }

    // 4. Check inside base_dir / filename
    fs::path candidate2 = base_dir / filename;
    if (fs::exists(candidate2)) {
        return fs::canonical(candidate2).string();
    }

    // 5. Look without subfolder inside Images_*/
    fs::path candidate3 = base_dir / ("Images_" + subfolder) / filename;
    if (fs::exists(candidate3)) {
        return fs::canonical(candidate3).string();
    }

    return "";
}

bool DatasetLoader::load() {
    frames_.clear();
    fs::path base_dir(options_.dataset_dir);

    if (!fs::exists(base_dir)) {
        std::cerr << "[DatasetLoader] Error: Dataset dir does not exist: " << options_.dataset_dir << std::endl;
        return false;
    }

    // Determine trajectory file path
    fs::path traj_path = options_.traj_file.empty() ? (base_dir / "traj.txt") : fs::path(options_.traj_file);

    std::vector<std::string> raw_rgb_lines;
    std::vector<std::string> raw_red_lines;
    std::vector<std::string> raw_nir_lines;
    std::vector<std::string> raw_reg_lines;
    std::vector<std::string> raw_gre_lines;

    if (fs::exists(traj_path)) {
        auto read_file_lines = [](const fs::path& p) {
            std::vector<std::string> lines;
            std::ifstream file(p);
            std::string line;
            while (std::getline(file, line)) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                if (!line.empty()) lines.push_back(line);
            }
            return lines;
        };

        raw_rgb_lines = read_file_lines(traj_path);

        fs::path traj_red = traj_path;
        traj_red.replace_filename(traj_path.stem().string() + "_RED.txt");
        if (fs::exists(traj_red)) raw_red_lines = read_file_lines(traj_red);

        fs::path traj_nir = traj_path;
        traj_nir.replace_filename(traj_path.stem().string() + "_NIR.txt");
        if (fs::exists(traj_nir)) raw_nir_lines = read_file_lines(traj_nir);

        fs::path traj_reg = traj_path;
        traj_reg.replace_filename(traj_path.stem().string() + "_REG.txt");
        if (fs::exists(traj_reg)) raw_reg_lines = read_file_lines(traj_reg);

        fs::path traj_gre = traj_path;
        traj_gre.replace_filename(traj_path.stem().string() + "_GRE.txt");
        if (fs::exists(traj_gre)) raw_gre_lines = read_file_lines(traj_gre);
    } else {
        // Auto-discover files in Images_RGB
        fs::path rgb_folder = base_dir / "Images_RGB";
        if (fs::exists(rgb_folder)) {
            for (const auto& entry : fs::directory_iterator(rgb_folder)) {
                if (entry.is_regular_file()) {
                    raw_rgb_lines.push_back(entry.path().filename().string());
                }
            }
            std::sort(raw_rgb_lines.begin(), raw_rgb_lines.end());
        }
    }

    const size_t total_items = raw_rgb_lines.size();
    if (total_items == 0) {
        std::cerr << "[DatasetLoader] Error: No images found in dataset: " << options_.dataset_dir << std::endl;
        return false;
    }

    size_t start_idx = 0;
    size_t end_idx = total_items - 1;

    if (options_.start_frame >= 0) {
        start_idx = std::min(static_cast<size_t>(options_.start_frame), total_items - 1);
    }
    if (options_.end_frame >= 0) {
        end_idx = std::min(static_cast<size_t>(options_.end_frame), total_items - 1);
    }
    if (start_idx > end_idx) {
        std::swap(start_idx, end_idx);
    }

    for (size_t i = start_idx; i <= end_idx; ++i) {
        DatasetFrame frame;
        frame.index = i;

        frame.rgb_path = resolvePath(raw_rgb_lines[i], "Images_RGB");
        if (i < raw_red_lines.size()) frame.red_path = resolvePath(raw_red_lines[i], "Images_RED");
        if (i < raw_nir_lines.size()) frame.nir_path = resolvePath(raw_nir_lines[i], "Images_NIR");
        if (i < raw_reg_lines.size()) frame.reg_path = resolvePath(raw_reg_lines[i], "Images_REG");
        if (i < raw_gre_lines.size()) frame.gre_path = resolvePath(raw_gre_lines[i], "Images_GRE");

        frames_.push_back(frame);
    }

    loadGroundTruth();

    std::cout << "[DatasetLoader] Loaded " << frames_.size()
              << " frames (sliced from " << start_idx << " to " << end_idx
              << " of total " << total_items << ")" << std::endl;
    return true;
}

bool DatasetLoader::loadGroundTruth() {
    fs::path base_dir(options_.dataset_dir);
    fs::path gt_path;

    if (!options_.ground_truth_file.empty()) {
        gt_path = options_.ground_truth_file;
    } else {
        std::vector<std::string> candidates = {"traj_truth.txt", "GT2.txt", "GT_traj.txt"};
        for (const auto& c : candidates) {
            if (fs::exists(base_dir / c)) {
                gt_path = base_dir / c;
                break;
            }
        }
    }

    if (gt_path.empty() || !fs::exists(gt_path)) {
        return false;
    }

    std::ifstream file(gt_path);
    std::vector<GroundTruthSample> all_samples;
    double x, y, z, yaw;

    while (file >> x >> y >> z >> yaw) {
        GroundTruthSample s;
        s.x = x;
        s.y = y;
        s.z = z;
        s.yaw_deg = yaw;
        s.yaw_rad = yaw * M_PI / 180.0;
        all_samples.push_back(s);
    }

    if (all_samples.empty()) {
        return false;
    }

    // Determine Ground Truth offset relative to traj.txt lines
    // In legacy datasets, traj_truth.txt starts at the flight execution segment (iniexec)
    // rather than image 0 (takeoff/ascent).
    int effective_offset = 0;
    if (options_.gt_offset >= 0) {
        effective_offset = options_.gt_offset;
    } else {
        // Auto-detect known legacy offsets based on dataset directory
        std::string dir_str = options_.dataset_dir;
        if (dir_str.find("0036") != std::string::npos) {
            effective_offset = 84;  // Image 0199
        } else if (dir_str.find("0035") != std::string::npos) {
            effective_offset = 18;  // Image 0103
        } else if (dir_str.find("0040") != std::string::npos) {
            effective_offset = 55;  // Image 0200
        } else if (dir_str.find("voo_100m") != std::string::npos || dir_str.find("UFRGS") != std::string::npos) {
            effective_offset = 204; // Image 0390
        }
    }

    size_t matched_count = 0;
    for (size_t i = 0; i < frames_.size(); ++i) {
        size_t frame_idx = frames_[i].index;

        // GT line k always corresponds to traj.txt line (k + offset). The former
        // "exact size match" shortcut paired GT[0] with the FIRST SLICED FRAME whenever the
        // slice happened to have the same length as the GT file (e.g. --start_frame 0
        // --end_frame 195 on ADM/0036 paired the take-off image 0115 with the GPS fix of
        // image 0199), silently shifting the ground truth by `offset` frames.
        if (static_cast<int>(frame_idx) >= effective_offset) {
            size_t gt_idx = frame_idx - effective_offset;
            if (gt_idx < all_samples.size()) {
                frames_[i].has_ground_truth = true;
                frames_[i].ground_truth = all_samples[gt_idx];
                matched_count++;
            }
        }
    }

    std::cout << "[DatasetLoader] Ground truth loaded: " << all_samples.size()
              << " samples, matched " << matched_count << " of " << frames_.size()
              << " frames (using GT offset " << effective_offset << ")" << std::endl;

    return true;
}

cv::Mat DatasetLoader::preprocessBand(const cv::Mat& band) const {
    if (band.empty() || options_.ms_filter == ImagePreprocessFilter::NONE) {
        return band;
    }

    cv::Mat filtered;
    if (options_.ms_filter == ImagePreprocessFilter::BILATERAL) {
        cv::bilateralFilter(
            band,
            filtered,
            options_.bilateral_d,
            options_.bilateral_sigma_color,
            options_.bilateral_sigma_space
        );
    } else if (options_.ms_filter == ImagePreprocessFilter::GAUSSIAN) {
        cv::GaussianBlur(band, filtered, cv::Size(5, 5), 1.5, 1.5);
    }
    return filtered;
}

} // namespace lambda_brief
