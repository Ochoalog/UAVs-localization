#include "core/types.hpp"
#include "core/vegetation_index.hpp"
#include "core/mask_generator.hpp"
#include "core/pixel_sampler.hpp"
#include "core/descriptor.hpp"
#include "localization/particle_filter.hpp"
#include "localization/phase_controller.hpp"
#include "localization/visual_odometry.hpp"
#include "dataset/dataset_loader.hpp"

#include <iostream>
#include <cassert>
#include <cmath>
#include <opencv2/imgproc.hpp>

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


void testLegacyQuantization() {
    std::cout << "[Test 11] Legacy 25-level colour quantisation (DroneRobot::quantization)...\n";
    cv::Mat img(1, 4, CV_8UC3);
    img.at<cv::Vec3b>(0, 0) = cv::Vec3b(0, 0, 0);
    img.at<cv::Vec3b>(0, 1) = cv::Vec3b(128, 128, 128);
    img.at<cv::Vec3b>(0, 2) = cv::Vec3b(255, 255, 255);
    img.at<cv::Vec3b>(0, 3) = cv::Vec3b(10, 11, 21);
    cv::Mat q = Descriptor::quantizeColors(img, 25);
    TEST_CHECK(q.at<cv::Vec3b>(0, 0)[0] == 0, "0 -> 0");
    TEST_CHECK(q.at<cv::Vec3b>(0, 1)[0] == 122, "128 -> floor(128*25/255)*10.2 = 122");
    TEST_CHECK(q.at<cv::Vec3b>(0, 2)[0] == 254, "255 -> 25*10.2f = 254.99999 -> 254 (legacy truncation)");
    TEST_CHECK(q.at<cv::Vec3b>(0, 3)[0] == 0 && q.at<cv::Vec3b>(0, 3)[1] == 10 && q.at<cv::Vec3b>(0, 3)[2] == 20,
               "10->0, 11->10, 21->20");
    cv::Mat off = Descriptor::quantizeColors(img, 0);
    TEST_CHECK(cv::norm(off, img, cv::NORM_INF) == 0, "levels=0 disables quantisation");
}

void testBayesianWeightAccumulation() {
    std::cout << "[Test 12] Bayesian weight update multiplies the prior (w_t ~ w_{t-1} * p(z|x))...\n";
    cv::Mat map_bgr(400, 400, CV_8UC3);
    cv::RNG rng(1234);
    rng.fill(map_bgr, cv::RNG::UNIFORM, 0, 256);
    cv::GaussianBlur(map_bgr, map_bgr, cv::Size(5, 5), 1.5);
    // Drone image = exact crop centred on (200,200), no rotation, scale 1.
    // With yaw = -pi/2 the projection angle theta = -yaw - pi/2 = 0.
    cv::Mat drone = map_bgr(cv::Rect(100, 100, 200, 200)).clone();
    PixelSampler sampler(7);
    auto pairs = sampler.sampleUniform(drone.size(), 256);
    auto desc = Descriptor::computeDroneDescriptor(drone, pairs, DescriptorColorMode::LAB_NO_L);
    cv::Mat map_lab = Descriptor::convertColorSpace(map_bgr, DescriptorColorMode::LAB_NO_L);

    ParticleFilterConfig pfc;
    pfc.num_particles = 2;
    pfc.resample_neff_ratio = 0.0; // never resample inside this test
    ParticleFilter pf(pfc);
    pf.initializeUniform(map_bgr.size());
    auto& P = const_cast<std::vector<Particle>&>(pf.getParticles());
    P[0].pose = Pose(200, 200, 1.0, -M_PI / 2);   // true pose -> d = 0
    P[1].pose = Pose(260, 230, 1.0, -M_PI / 2);   // wrong pose
    P[0].weight = 0.1;
    P[1].weight = 0.9;

    const double d1 = Descriptor::evaluateParticleDifference(map_lab, P[1].pose, drone.size(), pairs, desc,
                                                             DescriptorColorMode::LAB_NO_L);
    const double d0 = Descriptor::evaluateParticleDifference(map_lab, P[0].pose, drone.size(), pairs, desc,
                                                             DescriptorColorMode::LAB_NO_L);
    TEST_CHECK(d0 < 1e-9, "Hamming distance is 0 at the true pose");
    TEST_CHECK(d1 > 0.2, "Hamming distance is large at a wrong pose");

    pf.updateWeights(map_lab, drone.size(), pairs, desc, DescriptorColorMode::LAB_NO_L);
    const double l0 = 1.0, l1 = std::exp(-(d1 * d1) / (2.0 * pfc.var_brief));
    const double expected_w0 = 0.1 * l0 / (0.1 * l0 + 0.9 * l1);
    TEST_CHECK(std::abs(P[0].weight - expected_w0) < 1e-6, "Posterior weight = prior * likelihood (normalised)");
    TEST_CHECK(std::abs(P[0].weight + P[1].weight - 1.0) < 1e-9, "Weights normalised");
}

void testResampleEveryFrame() {
    std::cout << "[Test 13] resample_neff_ratio >= 1 resamples every frame (legacy)...\n";
    ParticleFilterConfig pfc;
    pfc.num_particles = 50;
    pfc.resample_neff_ratio = 1.0;
    ParticleFilter pf(pfc);
    pf.initializeUniform(cv::Size(100, 100));
    TEST_CHECK(pf.resampleIfNeeded(), "Resampling happens even with uniform weights");
    TEST_CHECK(pf.getNumParticles() == 50, "Particle count conserved");
}

void testScaleAndYawPriors() {
    std::cout << "[Test 14] Altimeter / compass priors multiply the likelihood...\n";
    cv::Mat map_bgr(400, 400, CV_8UC3, cv::Scalar(128, 128, 128));
    cv::Mat drone(100, 100, CV_8UC3, cv::Scalar(128, 128, 128));
    PixelSampler sampler(3);
    auto pairs = sampler.sampleUniform(drone.size(), 64);
    auto desc = Descriptor::computeDroneDescriptor(drone, pairs, DescriptorColorMode::LAB_NO_L);
    cv::Mat map_lab = Descriptor::convertColorSpace(map_bgr, DescriptorColorMode::LAB_NO_L);

    ParticleFilterConfig pfc;
    pfc.num_particles = 3;
    pfc.resample_neff_ratio = 0.0;
    ParticleFilter pf(pfc);
    pf.initializeUniform(map_bgr.size());
    auto& P = const_cast<std::vector<Particle>&>(pf.getParticles());
    // Flat map -> identical Hamming distance for all three; only the priors differ.
    P[0].pose = Pose(200, 200, 0.50, 0.0);
    P[1].pose = Pose(200, 200, 0.60, 0.0);            // dz = 0.1 = 1 sigma
    P[2].pose = Pose(200, 200, 0.50, 20.0 * M_PI / 180.0); // dyaw = 20 deg = 1 sigma
    for (auto& p : P) p.weight = 1.0 / 3.0;
    pf.setScalePrior(0.5, 0.2);                       // sigma_z = 0.2 * 0.5 = 0.1
    pf.setYawPrior(0.0, 20.0 * M_PI / 180.0);
    pf.updateWeights(map_lab, drone.size(), pairs, desc, DescriptorColorMode::LAB_NO_L);
    const double e = std::exp(-0.5);
    TEST_CHECK(std::abs(P[1].weight / P[0].weight - e) < 1e-9, "Scale prior: 1-sigma particle weighs exp(-1/2)");
    TEST_CHECK(std::abs(P[2].weight / P[0].weight - e) < 1e-9, "Yaw prior: 1-sigma particle weighs exp(-1/2)");

    // Priors apply to one update only.
    for (auto& p : P) p.weight = 1.0 / 3.0;
    pf.updateWeights(map_lab, drone.size(), pairs, desc, DescriptorColorMode::LAB_NO_L);
    TEST_CHECK(std::abs(P[1].weight - P[0].weight) < 1e-12 && std::abs(P[2].weight - P[0].weight) < 1e-12,
               "Priors are cleared after one update");
}

void testRandomInjection() {
    std::cout << "[Test 15] Random particle injection keeps diversity...\n";
    cv::Mat map_bgr(1000, 1000, CV_8UC3, cv::Scalar(128, 128, 128));
    cv::Mat drone(50, 50, CV_8UC3, cv::Scalar(128, 128, 128));
    PixelSampler sampler(5);
    auto pairs = sampler.sampleUniform(drone.size(), 32);
    auto desc = Descriptor::computeDroneDescriptor(drone, pairs, DescriptorColorMode::LAB_NO_L);
    cv::Mat map_lab = Descriptor::convertColorSpace(map_bgr, DescriptorColorMode::LAB_NO_L);

    ParticleFilterConfig pfc;
    pfc.num_particles = 2000;
    pfc.inject_ratio = 0.10;
    pfc.random_seed = 11;
    ParticleFilter pf(pfc);
    pf.initializeUniform(map_bgr.size());
    auto& P = const_cast<std::vector<Particle>&>(pf.getParticles());
    for (auto& p : P) { p.pose = Pose(500, 500, 1.0, 0.0); p.weight = 1.0 / P.size(); }  // collapsed cloud
    pf.setScalePrior(0.8, 0.05);
    pf.updateWeights(map_lab, drone.size(), pairs, desc, DescriptorColorMode::LAB_NO_L);
    size_t moved = 0; double zsum = 0.0;
    for (const auto& p : P) {
        if (std::abs(p.pose.x - 500) > 1e-9 || std::abs(p.pose.y - 500) > 1e-9) { ++moved; zsum += p.pose.z; }
    }
    // 200 draws with replacement from 2000 -> ~190 distinct particles replaced.
    TEST_CHECK(moved > 150 && moved <= 200, "About inject_ratio * N particles re-drawn over the map");
    TEST_CHECK(moved > 0 && std::abs(zsum / moved - 0.8) < 0.02, "Injected z follows the altitude prior");
}

void testModeEstimate() {
    std::cout << "[Test 16] Mode estimator picks the dominant cluster, not the midpoint...\n";
    ParticleFilterConfig pfc;
    pfc.num_particles = 1000;
    pfc.random_seed = 3;
    pfc.mode_estimate = true;
    pfc.mode_radius_px = 100.0;
    ParticleFilter pf(pfc);
    pf.initializeUniform(cv::Size(4800, 4800));
    auto& P = const_cast<std::vector<Particle>&>(pf.getParticles());
    std::mt19937 rng(9);
    std::normal_distribution<double> n(0.0, 15.0);
    for (size_t i = 0; i < P.size(); ++i) {
        const bool a = i < 600;                       // 60% at (1000,1000), 40% at (3000,3000)
        P[i].pose = Pose((a ? 1000.0 : 3000.0) + n(rng), (a ? 1000.0 : 3000.0) + n(rng), 0.5, 0.3);
        P[i].weight = 1.0 / P.size();
    }
    const auto est = pf.getEstimationState();
    TEST_CHECK(std::hypot(est.mean_pose.x - 1000.0, est.mean_pose.y - 1000.0) < 5.0,
               "Mode estimate lands on the 60% cluster");
    TEST_CHECK(std::abs(est.mean_pose.yaw - 0.3) < 1e-9 && std::abs(est.mean_pose.z - 0.5) < 1e-9,
               "Mode estimate keeps the cluster's yaw and z");
    TEST_CHECK(est.variance.x > 500.0 * 500.0, "Variance still reports the spread of the whole cloud");
}

void testInjectionGate() {
    std::cout << "[Test 17] Injection gate: contrast measured, no injection below the threshold...\n";
    cv::Mat map_bgr(400, 400, CV_8UC3);
    cv::RNG rng(1234);
    rng.fill(map_bgr, cv::RNG::UNIFORM, 0, 256);
    cv::GaussianBlur(map_bgr, map_bgr, cv::Size(5, 5), 1.5);
    cv::Mat drone = map_bgr(cv::Rect(100, 100, 200, 200)).clone();
    PixelSampler sampler(7);
    auto pairs = sampler.sampleUniform(drone.size(), 256);
    auto desc = Descriptor::computeDroneDescriptor(drone, pairs, DescriptorColorMode::LAB_NO_L);
    cv::Mat map_lab = Descriptor::convertColorSpace(map_bgr, DescriptorColorMode::LAB_NO_L);

    ParticleFilterConfig pfc;
    pfc.num_particles = 500;
    pfc.random_seed = 5;
    pfc.min_scale = 0.9;
    pfc.max_scale = 1.1;
    pfc.inject_ratio = 0.10;
    pfc.inject_gate = 1e9;   // no frame is informative enough -> never inject
    ParticleFilter pf(pfc);
    pf.initializeUniform(map_bgr.size());
    auto& P = const_cast<std::vector<Particle>&>(pf.getParticles());
    for (auto& p : P) { p.pose = Pose(200, 200, 1.0, -M_PI / 2); p.weight = 1.0 / P.size(); }  // on the truth
    pf.updateWeights(map_lab, drone.size(), pairs, desc, DescriptorColorMode::LAB_NO_L);
    TEST_CHECK(pf.getObservationContrast() > 0.5, "Cloud on the true pose has a high contrast");
    TEST_CHECK(pf.getLastInjectedCount() == 0, "Gate closed: nothing injected below the contrast threshold");
    size_t moved = 0;
    for (const auto& p : P) moved += (std::abs(p.pose.x - 200) > 1e-9 || std::abs(p.pose.y - 200) > 1e-9);
    TEST_CHECK(moved == 0, "Tracking cloud untouched");
    TEST_CHECK(pf.getNeffBeforeResampling() > 0.99 * P.size(), "Neff before resampling is recorded");
}

void testInitAroundPose() {
    std::cout << "[Test 18] Diagnostic init_around_gt: Gaussian cloud centred on a given pose...\n";
    ParticleFilterConfig pfc;
    pfc.num_particles = 20000;
    pfc.random_seed = 11;
    pfc.min_scale = 0.1;
    pfc.max_scale = 3.0;
    ParticleFilter pf(pfc);
    const Pose center(2000.0, 2500.0, 1.0, 0.6);
    const double sigma_xy = 20.0, mean_z = 0.87, sigma_z = 0.05 * mean_z;
    pf.initializeAroundPose(cv::Size(4800, 4800), center, sigma_xy, 5.0, mean_z, sigma_z);
    const auto& P = pf.getParticles();
    TEST_CHECK(P.size() == pfc.num_particles, "Cloud size matches num_particles");
    double mx = 0, my = 0, mz = 0, vx = 0, vy = 0, vz = 0;
    for (const auto& p : P) { mx += p.pose.x; my += p.pose.y; mz += p.pose.z; }
    mx /= P.size(); my /= P.size(); mz /= P.size();
    for (const auto& p : P) {
        vx += (p.pose.x - mx) * (p.pose.x - mx);
        vy += (p.pose.y - my) * (p.pose.y - my);
        vz += (p.pose.z - mz) * (p.pose.z - mz);
    }
    vx /= P.size(); vy /= P.size(); vz /= P.size();
    TEST_CHECK(std::abs(mx - center.x) < 2.0 && std::abs(my - center.y) < 2.0,
               "Cloud mean x,y lands on the requested centre");
    TEST_CHECK(std::abs(std::sqrt(vx) - sigma_xy) < 2.0 && std::abs(std::sqrt(vy) - sigma_xy) < 2.0,
               "Cloud x,y std-dev matches sigma_xy_px");
    TEST_CHECK(std::abs(mz - mean_z) < 0.02, "Cloud mean z lands on mean_z");
    TEST_CHECK(std::sqrt(vz) < sigma_z * 1.5, "Cloud z std-dev is close to sigma_z (clamped to [min,max]_scale)");
    bool all_in_bounds = true, all_z_in_range = true;
    for (const auto& p : P) {
        if (p.pose.x < 0.0 || p.pose.x > 4800.0 || p.pose.y < 0.0 || p.pose.y > 4800.0) all_in_bounds = false;
        if (p.pose.z < pfc.min_scale || p.pose.z > pfc.max_scale) all_z_in_range = false;
    }
    TEST_CHECK(all_in_bounds, "All particles stay within map bounds (clamped)");
    TEST_CHECK(all_z_in_range, "All particles' z stays within [min_scale, max_scale] (clamped)");
}

void testPhaseController() {
    std::cout << "[Test 19] Two-phase controller: GLOBAL -> TRACKING -> GLOBAL...\n";
    PhaseConfig pc;
    pc.enabled = true;
    pc.converge_std_px = 100.0;
    pc.converge_frames = 3;
    pc.converge_contrast = 1.3;
    pc.relocalize_std_px = 400.0;
    pc.relocalize_frames = 2;
    PhaseController ctl(pc);
    TEST_CHECK(ctl.phase() == FilterPhase::GLOBAL, "Starts in GLOBAL");
    ctl.update(50.0, true, 1.5);
    ctl.update(50.0, true, 1.0);          // contrast too low -> streak reset
    TEST_CHECK(ctl.streak() == 0 && !ctl.tracking(), "Low contrast resets the convergence streak");
    ctl.update(50.0, true, 1.5);
    ctl.update(900.0, true, 1.5);         // spread too large -> reset
    TEST_CHECK(ctl.streak() == 0, "Large spread resets the convergence streak");
    ctl.update(50.0, true, 1.5);
    ctl.update(60.0, false, 0.0);         // contrast unavailable -> spread decides
    const bool changed = ctl.update(70.0, true, 1.4);
    TEST_CHECK(changed && ctl.tracking(), "Switches to TRACKING after converge_frames good frames");
    ctl.update(500.0, false, 0.0);
    TEST_CHECK(ctl.tracking(), "One spread-out frame does not trigger relocalization");
    ctl.update(80.0, false, 0.0);
    ctl.update(500.0, false, 0.0);
    const bool back = ctl.update(600.0, false, 0.0);
    TEST_CHECK(back && ctl.phase() == FilterPhase::GLOBAL, "Back to GLOBAL after relocalize_frames spread-out frames");

    PhaseConfig off;
    PhaseController ctl_off(off);
    for (int i = 0; i < 20; ++i) ctl_off.update(1.0, true, 5.0);
    TEST_CHECK(!ctl_off.tracking(), "Disabled controller stays in GLOBAL (legacy behaviour)");

    PhaseConfig never;
    never.enabled = true;
    never.converge_frames = 1;
    never.converge_contrast = 0.0;
    PhaseController ctl_never(never);
    ctl_never.update(10.0, false, 0.0);
    for (int i = 0; i < 20; ++i) ctl_never.update(5000.0, false, 0.0);
    TEST_CHECK(ctl_never.tracking(), "relocalize_std_px = 0 never leaves TRACKING");
}

void testInjectionDisabledStillMeasuresContrast() {
    std::cout << "[Test 20] Injection disabled: contrast still measured, cloud untouched...\n";
    cv::Mat map_bgr(400, 400, CV_8UC3);
    cv::RNG rng(1234);
    rng.fill(map_bgr, cv::RNG::UNIFORM, 0, 256);
    cv::GaussianBlur(map_bgr, map_bgr, cv::Size(5, 5), 1.5);
    cv::Mat drone = map_bgr(cv::Rect(100, 100, 200, 200)).clone();
    PixelSampler sampler(7);
    auto pairs = sampler.sampleUniform(drone.size(), 256);
    auto desc = Descriptor::computeDroneDescriptor(drone, pairs, DescriptorColorMode::LAB_NO_L);
    cv::Mat map_lab = Descriptor::convertColorSpace(map_bgr, DescriptorColorMode::LAB_NO_L);

    ParticleFilterConfig pfc;
    pfc.num_particles = 500;
    pfc.random_seed = 5;
    pfc.min_scale = 0.9;
    pfc.max_scale = 1.1;
    pfc.inject_ratio = 0.10;
    ParticleFilter pf(pfc);
    pf.initializeUniform(map_bgr.size());
    auto& P = const_cast<std::vector<Particle>&>(pf.getParticles());
    for (auto& p : P) { p.pose = Pose(200, 200, 1.0, -M_PI / 2); p.weight = 1.0 / P.size(); }
    pf.setInjectionEnabled(false);
    pf.updateWeights(map_lab, drone.size(), pairs, desc, DescriptorColorMode::LAB_NO_L);
    TEST_CHECK(pf.hasObservationContrast() && pf.getObservationContrast() > 0.5,
               "Contrast measured from the candidate poses");
    TEST_CHECK(pf.getLastInjectedCount() == 0, "Nothing injected while disabled");
    size_t moved = 0;
    for (const auto& p : P) moved += (std::abs(p.pose.x - 200) > 1e-9 || std::abs(p.pose.y - 200) > 1e-9);
    TEST_CHECK(moved == 0, "Cloud untouched while injection is disabled");
    pf.setInjectionEnabled(true);
    pf.updateWeights(map_lab, drone.size(), pairs, desc, DescriptorColorMode::LAB_NO_L);
    TEST_CHECK(pf.getLastInjectedCount() == 50, "Re-enabled: inject_ratio * N candidates injected");
}

void testVisualOdometryStreaming() {
    std::cout << "[Test 21] Visual odometry: legacy vs ratio matcher, track() == stateless estimate...\n";
    // Textured synthetic ground: blurred noise + random rectangles.
    cv::RNG rng(7);
    cv::Mat canvas(900, 1100, CV_8UC3);
    rng.fill(canvas, cv::RNG::UNIFORM, 0, 255);
    cv::GaussianBlur(canvas, canvas, cv::Size(0, 0), 2.0);
    for (int k = 0; k < 400; ++k) {
        cv::Point a(rng.uniform(0, 1100), rng.uniform(0, 900));
        cv::rectangle(canvas, a, a + cv::Point(rng.uniform(5, 60), rng.uniform(5, 60)),
                      cv::Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255)), cv::FILLED);
    }
    // Camera moves 20 px "forward" (up in the image) and 8 px to the right between frames.
    const cv::Rect prev_roi(200, 250, 620, 456), curr_roi(208, 230, 620, 456);
    const cv::Mat prev = canvas(prev_roi).clone(), curr = canvas(curr_roi).clone();

    for (VOMatcher m : {VOMatcher::LEGACY, VOMatcher::RATIO}) {
        VisualOdometryOptions o;
        o.matcher = m;
        o.robust = (m == VOMatcher::RATIO) ? VORobust::MAGSAC : VORobust::RANSAC;
        VisualOdometry stateless(o), streaming(o);
        auto [u, ok] = stateless.estimateRelativeMotion(prev, curr);
        const std::string tag = (m == VOMatcher::RATIO) ? "ratio" : "legacy";
        TEST_CHECK(ok, tag + ": pure translation is reliable");
        TEST_CHECK(std::abs(u.x - 20.0) < 0.5 && std::abs(u.y - 8.0) < 0.5,
                   tag + ": dx = 20 (forward), dy = 8 (right)");
        TEST_CHECK(std::abs(u.yaw) < 0.01 && std::abs(u.z - 1.0) < 0.011, tag + ": no rotation / scale");

        auto first = streaming.track(prev);
        TEST_CHECK(!first.second && streaming.hasPrevious(), tag + ": first track() only primes the cache");
        auto [v, ok2] = streaming.track(curr);
        TEST_CHECK(ok2 && std::abs(v.x - u.x) < 1e-6 && std::abs(v.y - u.y) < 1e-6,
                   tag + ": track() reproduces the stateless estimate");
        if (m == VOMatcher::RATIO) {
            TEST_CHECK(streaming.lastStats().inliers > 200, "ratio: hundreds of homography inliers");
        }
    }
}

void testLegacyAbPattern() {
    std::cout << "[Test 22] Legacy abBRIEF pair pattern: fixed, near-uniform, same cv::RNG sequence as BriefHeuristic.cpp...\n";
    const cv::Size sz(1241, 913);
    auto a = PixelSampler::sampleLegacyAbBrief(sz, 256);
    auto b = PixelSampler::sampleLegacyAbBrief(sz, 256);
    TEST_CHECK(a.size() == 256, "legacy_ab: a.size() == 256");
    bool same = true, inside = true;
    double sx = 0, sxx = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        same &= (a[i].pt1 == b[i].pt1 && a[i].pt2 == b[i].pt2);
        for (const cv::Point& p : {a[i].pt1, a[i].pt2}) {
            inside &= (p.x >= 0 && p.x < sz.width && p.y >= 0 && p.y < sz.height);
            sx += p.x; sxx += double(p.x) * p.x;
        }
    }
    TEST_CHECK(same, "legacy_ab: same");    // the same pattern on every call (every frame, every run)
    TEST_CHECK(inside, "legacy_ab: inside");
    // Reference: the legacy loop written out with cv::RNG in its default state.
    cv::RNG rng;
    const double s = 1.0 / 25.0 * std::pow(1241.0, 2), t = 1.0 / 25.0 * std::pow(913.0, 2);
    cv::Point p0;
    do { p0 = cv::Point((int)rng.gaussian(s) + 1241 / 2, (int)rng.gaussian(t) + 913 / 2); }
    while (!(p0.x >= 0 && p0.x < 1241 && p0.y >= 0 && p0.y < 913));
    TEST_CHECK(a[0].pt1 == p0, "legacy_ab: a[0].pt1 == p0");
    // Near-uniform: std of x close to W/sqrt(12) = 358 px, far from the intended W/5 = 248 px.
    const double n = 2.0 * a.size(), mean = sx / n, sd = std::sqrt(sxx / n - mean * mean);
    TEST_CHECK(sd > 310.0, "legacy_ab: sd > 310.0");
    std::cout << "  -> fixed pattern, all points inside, std(x) = " << sd << " px (uniform: 358, W/5: 248)\n";
}

void testStabilityWeightedDistance() {
    std::cout << "[Test 23] v3 stability weight: per-pair weight min(W(A),W(B)), normalized and coin formulas...\n";
    cv::Mat map_bgr(400, 400, CV_8UC3);
    cv::RNG rng(4321);
    rng.fill(map_bgr, cv::RNG::UNIFORM, 0, 256);
    cv::GaussianBlur(map_bgr, map_bgr, cv::Size(5, 5), 1.5);
    cv::Mat map_lab = Descriptor::convertColorSpace(map_bgr, DescriptorColorMode::LAB_NO_L);
    cv::Mat drone = map_bgr(cv::Rect(100, 100, 200, 200)).clone();
    PixelSampler sampler(11);
    auto pairs = sampler.sampleUniform(drone.size(), 256);
    auto desc = Descriptor::computeDroneDescriptor(drone, pairs, DescriptorColorMode::LAB_NO_L);
    const auto mode = DescriptorColorMode::LAB_NO_L;
    const Pose wrong(240, 215, 1.0, -M_PI / 2);   // overlapping but wrong pose -> e in (0,1)

    // (a) all weights = 255 -> both formulas reproduce the v2 distance exactly
    cv::Mat ones(map_bgr.size(), CV_8U, cv::Scalar(255));
    const double d_plain = Descriptor::evaluateParticleDifference(map_lab, wrong, drone.size(), pairs, desc, mode);
    PairWeightMap wn{&ones, StabilityFormula::NORMALIZED}, wc{&ones, StabilityFormula::COIN};
    TEST_CHECK(std::abs(Descriptor::evaluateWeightedParticleDifference(map_lab, wrong, drone.size(), pairs, desc, mode, &wn) - d_plain) < 1e-12,
               "w = 1 everywhere: normalized == v2 distance");
    TEST_CHECK(std::abs(Descriptor::evaluateWeightedParticleDifference(map_lab, wrong, drone.size(), pairs, desc, mode, &wc) - d_plain) < 1e-12,
               "w = 1 everywhere: coin == v2 distance");
    TEST_CHECK(std::abs(Descriptor::evaluateWeightedParticleDifference(map_lab, wrong, drone.size(), pairs, desc, mode, nullptr) - d_plain) < 1e-12,
               "null weights fall back to the v2 distance");

    // (b) left half of the map weight 0.2 (51/255), right half weight 1: compare with a hand computation
    cv::Mat half(map_bgr.size(), CV_8U, cv::Scalar(255));
    half(cv::Rect(0, 0, 230, 400)).setTo(51);
    const double theta = -wrong.yaw - M_PI / 2;   // = 0 here
    double swe = 0, sw = 0, scoin = 0;
    for (size_t p = 0; p < pairs.size(); ++p) {
        auto proj = [&](cv::Point q) {
            double x = (q.x - 100) * wrong.z, y = (q.y - 100) * wrong.z;
            return cv::Point((int)std::round(x * std::cos(theta) + y * std::sin(theta) + wrong.x),
                             (int)std::round(-x * std::sin(theta) + y * std::cos(theta) + wrong.y));
        };
        cv::Point a = proj(pairs[p].pt1), b = proj(pairs[p].pt2);
        const auto& c1 = map_lab.at<cv::Vec3b>(a), &c2 = map_lab.at<cv::Vec3b>(b);
        int diff = ((c1[0] > c2[0]) != (bool)desc[2 * p]) + ((c1[1] > c2[1]) != (bool)desc[2 * p + 1]);
        double e = diff / 2.0, w = std::min(half.at<uchar>(a), half.at<uchar>(b)) / 255.0;
        swe += w * e; sw += w; scoin += w * e + (1 - w) * 0.5;
    }
    PairWeightMap hn{&half, StabilityFormula::NORMALIZED}, hc{&half, StabilityFormula::COIN};
    const double dn = Descriptor::evaluateWeightedParticleDifference(map_lab, wrong, drone.size(), pairs, desc, mode, &hn);
    const double dc = Descriptor::evaluateWeightedParticleDifference(map_lab, wrong, drone.size(), pairs, desc, mode, &hc);
    TEST_CHECK(std::abs(dn - swe / sw) < 1e-9, "normalized = sum(w e) / sum(w)");
    TEST_CHECK(std::abs(dc - scoin / pairs.size()) < 1e-9, "coin = mean(w e + (1 - w) / 2)");
    TEST_CHECK(std::abs(dn - d_plain) > 1e-6, "non-uniform weights change the distance");

    // (c) the true pose stays at distance 0 under both formulas
    const Pose truth(200, 200, 1.0, -M_PI / 2);
    TEST_CHECK(Descriptor::evaluateWeightedParticleDifference(map_lab, truth, drone.size(), pairs, desc, mode, &hn) < 1e-12,
               "normalized: d = 0 at the true pose");
    // coin at the true pose = mean((1 - w) / 2) > 0, the same for every pose with the same weights
    TEST_CHECK(Descriptor::evaluateWeightedParticleDifference(map_lab, truth, drone.size(), pairs, desc, mode, &hc) < 0.5,
               "coin: d < 0.5 at the true pose");

    // (d) the filter uses the weights when set
    ParticleFilterConfig pfc;
    pfc.num_particles = 2;
    pfc.resample_neff_ratio = 0.0;
    ParticleFilter pf(pfc);
    pf.initializeUniform(map_bgr.size());
    pf.setPairWeightMap(&half, StabilityFormula::NORMALIZED);
    auto& P = const_cast<std::vector<Particle>&>(pf.getParticles());
    P[0].pose = truth; P[1].pose = wrong; P[0].weight = P[1].weight = 0.5;
    pf.updateWeights(map_lab, drone.size(), pairs, desc, mode);
    const double l1 = std::exp(-(dn * dn) / (2.0 * pfc.var_brief));
    TEST_CHECK(std::abs(P[0].weight - 1.0 / (1.0 + l1)) < 1e-6, "updateWeights uses the weighted distance");
    // (e) pairDetails (--save_pairs) reproduces the weighted distance pair by pair
    auto det = Descriptor::pairDetails(map_lab, wrong, drone.size(), pairs, desc, mode, &hn);
    double we = 0, ww = 0;
    for (const auto& d : det) { we += d.w * d.e; ww += d.w; }
    TEST_CHECK(det.size() == pairs.size() && std::abs(we / ww - dn) < 1e-9, "pairDetails matches the weighted distance");
    std::cout << "  -> v2 d=" << d_plain << ", normalized d=" << dn << ", coin d=" << dc << "\n";
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
    testLegacyQuantization();
    testBayesianWeightAccumulation();
    testResampleEveryFrame();
    testScaleAndYawPriors();
    testRandomInjection();
    testModeEstimate();
    testInjectionGate();
    testInitAroundPose();
    testPhaseController();
    testInjectionDisabledStillMeasuresContrast();
    testVisualOdometryStreaming();
    testLegacyAbPattern();
    testStabilityWeightedDistance();

    std::cout << "\n========================================\n";
    std::cout << " Test Results: " << g_tests_passed << " Passed, "
              << g_tests_failed << " Failed.\n";
    std::cout << "========================================\n";

    return (g_tests_failed == 0) ? 0 : 1;
}
