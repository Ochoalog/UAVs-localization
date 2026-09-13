#include "core/types.hpp"
#include "core/vegetation_index.hpp"
#include "core/mask_generator.hpp"
#include "core/pixel_sampler.hpp"
#include "core/descriptor.hpp"
#include "localization/particle_filter.hpp"
#include "dataset/dataset_loader.hpp"

#include <iostream>
#include <cassert>
#include <cmath>

using namespace lambda_brief;

static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST_CHECK(condition, msg) \
    do { \
        if (!(condition)) { \
            std::cerr << "  [FAIL] " << msg << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
            g_tests_failed++; \
        } else { \
            g_tests_passed++; \
        } \
    } while(0)

void testVegetationIndexFormulas() {
    std::cout << "[Test 1] Vegetation Index Mathematical Formulas...\n";

    // NIR=200, RED=50 -> NDVI = (200-50)/(200+50) = 150/250 = 0.6
    double ndvi = VegetationIndex::computePixel(200, 0, 50, 0, VegetationIndexType::NDVI);
    TEST_CHECK(std::abs(ndvi - 0.6) < 1e-5, "NDVI calculation exactness");

    // NIR=180, REG=90 -> NDRE = (180-90)/(180+90) = 90/270 = 0.333333
    double ndre = VegetationIndex::computePixel(180, 90, 0, 0, VegetationIndexType::NDRE);
    TEST_CHECK(std::abs(ndre - (1.0/3.0)) < 1e-5, "NDRE calculation exactness");

    // GRE=150, RED=50 -> GRVI = (150-50)/(150+50) = 100/200 = 0.5
    double grvi = VegetationIndex::computePixel(0, 0, 50, 150, VegetationIndexType::GRVI);
    TEST_CHECK(std::abs(grvi - 0.5) < 1e-5, "GRVI calculation exactness");

    // Zero division edge case
    double zero_div = VegetationIndex::computePixel(0, 0, 0, 0, VegetationIndexType::NDVI);
    TEST_CHECK(zero_div == 0.0, "Zero division safety");
}

void testVegetationMatrixInversion() {
    std::cout << "[Test 2] Vegetation Matrix Inversion and 8-bit Scaling...\n";

    cv::Mat vi_mat(2, 2, CV_64FC1);
    vi_mat.at<double>(0, 0) = 1.0;   // Maximum vegetation
    vi_mat.at<double>(0, 1) = -1.0;  // Minimum vegetation
    vi_mat.at<double>(1, 0) = 0.0;   // Neutral
    vi_mat.at<double>(1, 1) = 0.5;

    cv::Mat inv_8u = VegetationIndex::computeInvertedMask8U(vi_mat);

    // -VI = -1 -> 0
    TEST_CHECK(inv_8u.at<uchar>(0, 0) == 0, "High VI inverts to 0 in 8-bit mask");
    // -VI = +1 -> 255
    TEST_CHECK(inv_8u.at<uchar>(0, 1) == 255, "Low VI inverts to 255 in 8-bit mask");
    // -VI = 0 -> 128 (approx 127.5)
    TEST_CHECK(std::abs(static_cast<int>(inv_8u.at<uchar>(1, 0)) - 128) <= 1, "Neutral VI inverts to ~128");
}

void testGaussianMaskProperties() {
    std::cout << "[Test 3] 2D Gaussian Mask Properties...\n";

    cv::Size sz(100, 80);
    cv::Mat gauss = MaskGenerator::generateGaussianMask(sz);

    TEST_CHECK(gauss.size() == sz, "Gaussian mask size");
    // Center point (50, 40) must be maximum (1.0)
    double center_val = gauss.at<double>(40, 50);
    TEST_CHECK(std::abs(center_val - 1.0) < 1e-3, "Gaussian center equals 1.0");

    // Corner points must decay towards 0
    double corner_val = gauss.at<double>(0, 0);
    TEST_CHECK(corner_val < 0.1, "Gaussian corner decays towards 0");
}

void testMaskCombinationAlpha() {
    std::cout << "[Test 4] Mask Combination with Alpha Weighting (Eq. 1)...\n";

    cv::Size sz(20, 20);
    cv::Mat mask1(sz, CV_64FC1, 0.8);
    cv::Mat mask2(sz, CV_64FC1, 0.2);

    double max_f = 0.0;
    cv::Mat combined_half = MaskGenerator::combineMasks(mask1, mask2, 0.5, &max_f);
    TEST_CHECK(std::abs(combined_half.at<double>(0, 0) - 0.5) < 1e-5, "Alpha = 0.5 combination");
    TEST_CHECK(std::abs(max_f - 0.5) < 1e-5, "Max fitness evaluation");

    cv::Mat combined_pure1 = MaskGenerator::combineMasks(mask1, mask2, 1.0);
    TEST_CHECK(std::abs(combined_pure1.at<double>(0, 0) - 0.8) < 1e-5, "Alpha = 1.0 (pure mask1)");

    cv::Mat combined_pure2 = MaskGenerator::combineMasks(mask1, mask2, 0.0);
    TEST_CHECK(std::abs(combined_pure2.at<double>(0, 0) - 0.2) < 1e-5, "Alpha = 0.0 (pure mask2)");
}

void testStochasticAcceptanceRoulette() {
    std::cout << "[Test 5] Stochastic Acceptance Roulette Sampling...\n";

    cv::Size sz(50, 50);
    cv::Mat density(sz, CV_64FC1, 0.0);
    // Left half = 0.0, Right half = 1.0
    for (int y = 0; y < 50; ++y) {
        for (int x = 25; x < 50; ++x) {
            density.at<double>(y, x) = 1.0;
        }
    }

    PixelSampler sampler(12345);
    auto pairs = sampler.sampleRoulette(density, 1.0, 100);
    TEST_CHECK(pairs.size() == 100, "Sampled exact requested pair count");

    // All sampled points should fall strictly in the non-zero (right) region
    int right_count = 0;
    for (const auto& p : pairs) {
        if (p.pt1.x >= 25) right_count++;
        if (p.pt2.x >= 25) right_count++;
    }
    TEST_CHECK(right_count == 200, "Zero-probability region is never sampled");
}

void testDescriptorExtraction() {
    std::cout << "[Test 6] Binary Descriptor Size and Value Range...\n";

    cv::Mat drone_img(60, 80, CV_8UC3, cv::Scalar(100, 150, 200));
    // Add variations
    drone_img.at<cv::Vec3b>(10, 10) = cv::Vec3b(255, 0, 0);
    drone_img.at<cv::Vec3b>(20, 20) = cv::Vec3b(0, 255, 0);

    std::vector<PixelPair> pairs(256);
    for (size_t i = 0; i < 256; ++i) {
        pairs[i].pt1 = cv::Point(10, 10);
        pairs[i].pt2 = cv::Point(20, 20);
    }

    auto desc = Descriptor::computeDroneDescriptor(drone_img, pairs, DescriptorColorMode::LAB_NO_L);
    TEST_CHECK(desc.size() == 512, "LAB_NO_L produces exactly 512 bits (256 pairs * 2)");

    bool valid_bits = true;
    for (uint8_t b : desc) {
        if (b != 0 && b != 1) valid_bits = false;
    }
    TEST_CHECK(valid_bits, "Descriptor contains strictly binary 0/1 bits");
}

void testHammingDistance() {
    std::cout << "[Test 7] Normalized Hamming Distance Calculation...\n";

    std::vector<uint8_t> drone_d = {1, 0, 1, 1, 0, 0, 1, 0};
    std::vector<int8_t> map_d_identical = {1, 0, 1, 1, 0, 0, 1, 0};
    std::vector<int8_t> map_d_opposite = {0, 1, 0, 0, 1, 1, 0, 1};
    std::vector<int8_t> map_d_half = {1, 0, 1, 1, 1, 1, 0, 1};

    TEST_CHECK(Descriptor::computeHammingDistance(drone_d, map_d_identical) == 0.0, "Identical distance == 0.0");
    TEST_CHECK(Descriptor::computeHammingDistance(drone_d, map_d_opposite) == 1.0, "Opposite distance == 1.0");
    TEST_CHECK(Descriptor::computeHammingDistance(drone_d, map_d_half) == 0.5, "Half mismatch distance == 0.5");
}

void testParticleFilterInitialization() {
    std::cout << "[Test 8] Particle Filter Uniform Initialization and Neff...\n";

    ParticleFilterConfig pfc;
    pfc.num_particles = 1000;
    ParticleFilter pf(pfc);

    cv::Size map_sz(500, 400);
    pf.initializeUniform(map_sz);

    TEST_CHECK(pf.getNumParticles() == 1000, "Particle count matches config");
    double neff = pf.getNeff();
    // Initially all weights are 1/N -> Neff = 1 / (N * (1/N)^2) = N
    TEST_CHECK(std::abs(neff - 1000.0) < 1e-3, "Initial Neff equals N");

    const auto& p = pf.getParticles();
    bool inside_bounds = true;
    for (const auto& part : p) {
        if (part.pose.x < 0 || part.pose.x > 500 || part.pose.y < 0 || part.pose.y > 400) {
            inside_bounds = false;
        }
    }
    TEST_CHECK(inside_bounds, "All particles initialized inside map bounds");
}

void testLowVarianceResampling() {
    std::cout << "[Test 9] Low Variance Resampling Conservation...\n";

    ParticleFilterConfig pfc;
    pfc.num_particles = 100;
    pfc.resample_neff_ratio = 0.9;
    ParticleFilter pf(pfc);
    pf.initializeUniform(cv::Size(200, 200));

    // Initially uniform: Neff = 100, threshold = 90 -> no resample needed
    TEST_CHECK(!pf.resampleIfNeeded(), "No resampling when weights are uniform");

    // Artificially skew weights to simulate weight update where Neff drops
    auto& particles = const_cast<std::vector<Particle>&>(pf.getParticles());
    for (size_t i = 0; i < particles.size(); ++i) {
        particles[i].weight = (i == 0) ? 0.9 : (0.1 / 99.0);
    }
    // Now Neff is low (~1.2), threshold is 90 -> must trigger resampling
    bool resampled = pf.resampleIfNeeded();
    TEST_CHECK(resampled, "Resampling was triggered when Neff < threshold");
    TEST_CHECK(pf.getNumParticles() == 100, "Particle count conserved after resampling");
}

void testPoseNormalization() {
    std::cout << "[Test 10] Pose Yaw Angle Wrapping...\n";

    Pose p1(0, 0, 1, 3.5 * M_PI);
    p1.normalizeYaw();
    TEST_CHECK(p1.yaw >= -M_PI && p1.yaw <= M_PI, "Angle normalized to [-pi, pi]");

    Pose p2(0, 0, 1, -2.5 * M_PI);
    p2.normalizeYaw();
    TEST_CHECK(p2.yaw >= -M_PI && p2.yaw <= M_PI, "Negative angle normalized to [-pi, pi]");
}

int main() {
    std::cout << "========================================\n";
    std::cout << " Running lambda-BRIEF Regression Tests  \n";
    std::cout << "========================================\n";

    testVegetationIndexFormulas();
    testVegetationMatrixInversion();
    testGaussianMaskProperties();
    testMaskCombinationAlpha();
    testStochasticAcceptanceRoulette();
    testDescriptorExtraction();
    testHammingDistance();
    testParticleFilterInitialization();
    testLowVarianceResampling();
    testPoseNormalization();

    std::cout << "\n========================================\n";
    std::cout << " Test Results: " << g_tests_passed << " Passed, "
              << g_tests_failed << " Failed.\n";
    std::cout << "========================================\n";

    return (g_tests_failed == 0) ? 0 : 1;
}
