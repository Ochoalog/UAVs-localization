// diag_likelihood — discriminability of the lambda-BRIEF observation model.
//
// For every frame of the slice that has ground truth, this tool builds the drone
// descriptor exactly like lambda_brief_loc and reports:
//   d_gt            normalised Hamming distance at the ground-truth pose
//   d_gt_best_yaw   best distance when only the yaw is swept (+-180 deg, 5 deg steps)
//   best_yaw_off    yaw offset (deg) of that best value -> checks the GT yaw convention
//   d_rand_mean/std distance statistics over 20000 random poses whose centre is inside
//                   the map and whose descriptor is not fully out of bounds
//   d_rand_p01      1st percentile of the random distances
//   frac_below_gt   fraction of random poses that score better than the GT
//
// Usage: same options as lambda_brief_loc (--map, --dataset_dir, --start_frame,
//        --end_frame, --alpha, --vi, --quantize, --min_scale, --max_scale, ...).
#include "config/config.hpp"
#include "core/vegetation_index.hpp"
#include "core/mask_generator.hpp"
#include "core/pixel_sampler.hpp"
#include "core/descriptor.hpp"
#include "dataset/dataset_loader.hpp"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <random>
#include <algorithm>
#include <cmath>
#include <cstdlib>

using namespace lambda_brief;

int main(int argc, char* argv[]) {
    AppConfig config = AppConfig::parseCommandLine(argc, argv);
    if (config.map_path.empty() || config.dataset_dir.empty()) {
        std::cerr << "Usage: diag_likelihood --map <map> --dataset_dir <dir> [lambda_brief_loc options]\n";
        return 1;
    }

    cv::Mat map_bgr = cv::imread(config.map_path, cv::IMREAD_COLOR);
    if (map_bgr.empty()) {
        std::cerr << "Error: could not load map " << config.map_path << "\n";
        return 1;
    }
    if (config.quantize_levels > 0) {
        map_bgr = Descriptor::quantizeColors(map_bgr, config.quantize_levels);
    }
    cv::Mat map = Descriptor::convertColorSpace(map_bgr, config.color_mode);

    DatasetLoader loader(config.toDatasetLoaderOptions());
    if (!loader.load()) return 1;

    PixelSampler sampler(config.random_seed);
    std::mt19937 rng(7);
    std::uniform_real_distribution<double> ux(0, map.cols), uy(0, map.rows), uyaw(-M_PI, M_PI);
    std::uniform_real_distribution<double> uz(config.min_scale, config.max_scale);

    std::cout << "frame d_gt d_gt_best_yaw best_yaw_off_deg d_rand_mean d_rand_std d_rand_p01 frac_below_gt best_zfac d_best_z\n";
    for (size_t f = 0; f < loader.size(); ++f) {
        const auto& fr = loader.getFrame(f);
        if (!fr.has_ground_truth) continue;
        cv::Mat rgb = fr.loadRGB();
        if (rgb.empty()) continue;
        if (config.quantize_levels > 0) {
            rgb = Descriptor::quantizeColors(rgb, config.quantize_levels);
        }

        cv::Mat vi = VegetationIndex::computeMatrix(
            loader.preprocessBand(fr.loadNIR()), loader.preprocessBand(fr.loadREG()),
            loader.preprocessBand(fr.loadRED()), loader.preprocessBand(fr.loadGRE()), config.vi_type);
        cv::Mat eq = MaskGenerator::generateEqualizedVegetationMask(VegetationIndex::computeInvertedMask8U(vi));
        cv::Mat gauss = MaskGenerator::generateGaussianMask(rgb.size());
        double max_fitness = 1.0;
        cv::Mat comb = MaskGenerator::combineMasks(eq, gauss, config.alpha, &max_fitness);
        auto pairs = sampler.sampleRoulette(comb, max_fitness, config.num_pairs);
        auto desc = Descriptor::computeDroneDescriptor(rgb, pairs, config.color_mode);

        auto dist = [&](const Pose& p) {
            return Descriptor::evaluateParticleDifference(map, p, rgb.size(), pairs, desc, config.color_mode);
        };

        const Pose gt(fr.ground_truth.x, fr.ground_truth.y, fr.ground_truth.z, fr.ground_truth.yaw_rad);
        const double d_gt = dist(gt);

        double best = 2.0, best_off = 0.0;
        for (int off = -180; off < 180; off += 5) {
            Pose p = gt;
            p.yaw = gt.yaw + off * M_PI / 180.0;
            const double d = dist(p);
            if (d < best) {
                best = d;
                best_off = off;
            }
        }

        // Scale sweep at the GT position/yaw: which multiple of the GT scale fits best?
        double best_zf = 1.0, d_best_z = 2.0;
        for (double zf = 0.50; zf <= 1.501; zf += 0.02) {
            Pose p = gt;
            p.z = gt.z * zf;
            const double d = dist(p);
            if (d < d_best_z) {
                d_best_z = d;
                best_zf = zf;
            }
        }

        std::vector<double> dr;
        dr.reserve(20000);
        while (dr.size() < static_cast<size_t>(std::getenv("LB_DIAG_NRAND") ? std::atoi(std::getenv("LB_DIAG_NRAND")) : 20000)) {
            const double d = dist(Pose(ux(rng), uy(rng), uz(rng), uyaw(rng)));
            if (d < 0.999) dr.push_back(d);
        }
        double m = 0.0, s = 0.0;
        for (double d : dr) m += d;
        m /= dr.size();
        for (double d : dr) s += (d - m) * (d - m);
        s = std::sqrt(s / dr.size());
        std::sort(dr.begin(), dr.end());
        const double below = static_cast<double>(std::lower_bound(dr.begin(), dr.end(), d_gt) - dr.begin());

        std::cout << f << " " << d_gt << " " << best << " " << best_off << " " << m << " " << s << " "
                  << dr[dr.size() / 100] << " " << below / dr.size() << " " << best_zf << " " << d_best_z << "\n";
    }
    return 0;
}
