#include "core/descriptor.hpp"
#include <opencv2/imgproc.hpp>
#include <cmath>

namespace lambda_brief {

static cv::Mat convertColorSpace(const cv::Mat& bgr, DescriptorColorMode mode) {
    cv::Mat converted;
    if (mode == DescriptorColorMode::GRAY) {
        if (bgr.channels() == 3) {
            cv::cvtColor(bgr, converted, cv::COLOR_BGR2GRAY);
        } else {
            converted = bgr.clone();
        }
    } else {
        if (bgr.channels() == 3) {
            cv::cvtColor(bgr, converted, cv::COLOR_BGR2Lab);
        } else {
            cv::cvtColor(bgr, converted, cv::COLOR_GRAY2BGR);
            cv::cvtColor(converted, converted, cv::COLOR_BGR2Lab);
        }
    }
    return converted;
}

std::vector<uint8_t> Descriptor::computeDroneDescriptor(
    const cv::Mat& img_bgr,
    const std::vector<PixelPair>& pairs,
    DescriptorColorMode mode
) {
    cv::Mat img = convertColorSpace(img_bgr, mode);
    std::vector<uint8_t> descriptor;

    if (mode == DescriptorColorMode::GRAY) {
        descriptor.reserve(pairs.size());
        for (const auto& pair : pairs) {
            uchar val1 = img.at<uchar>(pair.pt1.y, pair.pt1.x);
            uchar val2 = img.at<uchar>(pair.pt2.y, pair.pt2.x);
            descriptor.push_back(val1 > val2 ? 1 : 0);
        }
    } else if (mode == DescriptorColorMode::LAB_NO_L) {
        // Legacy color heuristic: compares first two channels (channels 0 and 1)
        descriptor.reserve(pairs.size() * 2);
        for (const auto& pair : pairs) {
            const auto& c1 = img.at<cv::Vec3b>(pair.pt1.y, pair.pt1.x);
            const auto& c2 = img.at<cv::Vec3b>(pair.pt2.y, pair.pt2.x);
            descriptor.push_back(c1[0] > c2[0] ? 1 : 0);
            descriptor.push_back(c1[1] > c2[1] ? 1 : 0);
        }
    } else { // LAB full (3 channels)
        descriptor.reserve(pairs.size() * 3);
        for (const auto& pair : pairs) {
            const auto& c1 = img.at<cv::Vec3b>(pair.pt1.y, pair.pt1.x);
            const auto& c2 = img.at<cv::Vec3b>(pair.pt2.y, pair.pt2.x);
            descriptor.push_back(c1[0] > c2[0] ? 1 : 0);
            descriptor.push_back(c1[1] > c2[1] ? 1 : 0);
            descriptor.push_back(c1[2] > c2[2] ? 1 : 0);
        }
    }

    return descriptor;
}

std::vector<int8_t> Descriptor::computeExpectedDescriptor(
    const cv::Mat& map_bgr,
    const Pose& particle_pose,
    const cv::Size& drone_size,
    const std::vector<PixelPair>& pairs,
    DescriptorColorMode mode
) {
    cv::Mat map = convertColorSpace(map_bgr, mode);

    const double scale = particle_pose.z;
    const float theta = static_cast<float>(particle_pose.yaw * (-1.0) - (M_PI * 0.5));
    const int xPad = drone_size.width / 2;
    const int yPad = drone_size.height / 2;
    const float cosTheta = std::cos(theta);
    const float sinTheta = std::sin(theta);

    const int bits_per_pair = (mode == DescriptorColorMode::GRAY) ? 1 :
                              (mode == DescriptorColorMode::LAB_NO_L) ? 2 : 3;

    std::vector<int8_t> expected;
    expected.reserve(pairs.size() * bits_per_pair);

    for (const auto& pair : pairs) {
        // Project pt1
        double px1 = (pair.pt1.x - xPad) * scale;
        double py1 = (pair.pt1.y - yPad) * scale;
        int m_x1 = static_cast<int>(std::round((px1 * cosTheta + py1 * sinTheta) + particle_pose.x));
        int m_y1 = static_cast<int>(std::round((px1 * (-sinTheta) + py1 * cosTheta) + particle_pose.y));

        // Project pt2
        double px2 = (pair.pt2.x - xPad) * scale;
        double py2 = (pair.pt2.y - yPad) * scale;
        int m_x2 = static_cast<int>(std::round((px2 * cosTheta + py2 * sinTheta) + particle_pose.x));
        int m_y2 = static_cast<int>(std::round((px2 * (-sinTheta) + py2 * cosTheta) + particle_pose.y));

        bool pt1_valid = (m_x1 >= 0 && m_x1 < map.cols && m_y1 >= 0 && m_y1 < map.rows);
        bool pt2_valid = (m_x2 >= 0 && m_x2 < map.cols && m_y2 >= 0 && m_y2 < map.rows);

        if (!pt1_valid || !pt2_valid) {
            for (int b = 0; b < bits_per_pair; ++b) {
                expected.push_back(-1); // undefined
            }
            continue;
        }

        if (mode == DescriptorColorMode::GRAY) {
            uchar val1 = map.at<uchar>(m_y1, m_x1);
            uchar val2 = map.at<uchar>(m_y2, m_x2);
            expected.push_back(val1 > val2 ? 1 : 0);
        } else if (mode == DescriptorColorMode::LAB_NO_L) {
            const auto& c1 = map.at<cv::Vec3b>(m_y1, m_x1);
            const auto& c2 = map.at<cv::Vec3b>(m_y2, m_x2);
            expected.push_back(c1[0] > c2[0] ? 1 : 0);
            expected.push_back(c1[1] > c2[1] ? 1 : 0);
        } else {
            const auto& c1 = map.at<cv::Vec3b>(m_y1, m_x1);
            const auto& c2 = map.at<cv::Vec3b>(m_y2, m_x2);
            expected.push_back(c1[0] > c2[0] ? 1 : 0);
            expected.push_back(c1[1] > c2[1] ? 1 : 0);
            expected.push_back(c1[2] > c2[2] ? 1 : 0);
        }
    }

    return expected;
}

double Descriptor::computeHammingDistance(
    const std::vector<uint8_t>& drone_desc,
    const std::vector<int8_t>& map_desc
) {
    if (drone_desc.empty() || drone_desc.size() != map_desc.size()) {
        return 1.0;
    }

    size_t diff_count = 0;
    const size_t total_bits = drone_desc.size();

    for (size_t i = 0; i < total_bits; ++i) {
        if (map_desc[i] < 0 || map_desc[i] != static_cast<int8_t>(drone_desc[i])) {
            diff_count++;
        }
    }

    return static_cast<double>(diff_count) / static_cast<double>(total_bits);
}

double Descriptor::evaluateParticleDifference(
    const cv::Mat& map_bgr,
    const Pose& particle_pose,
    const cv::Size& drone_size,
    const std::vector<PixelPair>& pairs,
    const std::vector<uint8_t>& drone_descriptor,
    DescriptorColorMode mode
) {
    // Fast path: if particle center is far outside map, return 1.0 immediately
    if (particle_pose.x < -100 || particle_pose.x >= map_bgr.cols + 100 ||
        particle_pose.y < -100 || particle_pose.y >= map_bgr.rows + 100) {
        return 1.0;
    }

    const double scale = particle_pose.z;
    const float theta = static_cast<float>(particle_pose.yaw * (-1.0) - (M_PI * 0.5));
    const int xPad = drone_size.width / 2;
    const int yPad = drone_size.height / 2;
    const float cosTheta = std::cos(theta);
    const float sinTheta = std::sin(theta);

    const int bits_per_pair = (mode == DescriptorColorMode::GRAY) ? 1 :
                              (mode == DescriptorColorMode::LAB_NO_L) ? 2 : 3;

    size_t diff_count = 0;
    const size_t num_pairs = pairs.size();
    size_t bit_idx = 0;

    for (size_t p = 0; p < num_pairs; ++p) {
        const auto& pair = pairs[p];

        double px1 = (pair.pt1.x - xPad) * scale;
        double py1 = (pair.pt1.y - yPad) * scale;
        int m_x1 = static_cast<int>(std::round((px1 * cosTheta + py1 * sinTheta) + particle_pose.x));
        int m_y1 = static_cast<int>(std::round((px1 * (-sinTheta) + py1 * cosTheta) + particle_pose.y));

        double px2 = (pair.pt2.x - xPad) * scale;
        double py2 = (pair.pt2.y - yPad) * scale;
        int m_x2 = static_cast<int>(std::round((px2 * cosTheta + py2 * sinTheta) + particle_pose.x));
        int m_y2 = static_cast<int>(std::round((px2 * (-sinTheta) + py2 * cosTheta) + particle_pose.y));

        bool valid = (m_x1 >= 0 && m_x1 < map_bgr.cols && m_y1 >= 0 && m_y1 < map_bgr.rows &&
                      m_x2 >= 0 && m_x2 < map_bgr.cols && m_y2 >= 0 && m_y2 < map_bgr.rows);

        if (!valid) {
            diff_count += bits_per_pair;
            bit_idx += bits_per_pair;
            continue;
        }

        if (mode == DescriptorColorMode::LAB_NO_L) {
            const auto& c1 = map_bgr.at<cv::Vec3b>(m_y1, m_x1);
            const auto& c2 = map_bgr.at<cv::Vec3b>(m_y2, m_x2);
            uint8_t exp0 = (c1[0] > c2[0]) ? 1 : 0;
            uint8_t exp1 = (c1[1] > c2[1]) ? 1 : 0;

            if (exp0 != drone_descriptor[bit_idx]) diff_count++;
            bit_idx++;
            if (exp1 != drone_descriptor[bit_idx]) diff_count++;
            bit_idx++;
        } else if (mode == DescriptorColorMode::GRAY) {
            uchar val1 = map_bgr.at<uchar>(m_y1, m_x1);
            uchar val2 = map_bgr.at<uchar>(m_y2, m_x2);
            uint8_t exp = (val1 > val2) ? 1 : 0;
            if (exp != drone_descriptor[bit_idx]) diff_count++;
            bit_idx++;
        } else {
            const auto& c1 = map_bgr.at<cv::Vec3b>(m_y1, m_x1);
            const auto& c2 = map_bgr.at<cv::Vec3b>(m_y2, m_x2);
            for (int k = 0; k < 3; ++k) {
                uint8_t exp = (c1[k] > c2[k]) ? 1 : 0;
                if (exp != drone_descriptor[bit_idx]) diff_count++;
                bit_idx++;
            }
        }
    }

    return static_cast<double>(diff_count) / static_cast<double>(drone_descriptor.size());
}

} // namespace lambda_brief
