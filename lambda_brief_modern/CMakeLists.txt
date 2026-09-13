cmake_minimum_required(VERSION 3.20)
project(lambda_brief_modern VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optimization flags
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -O3 -march=native)
endif()

# Find Dependencies
find_package(OpenCV 4.0 REQUIRED COMPONENTS core imgproc highgui features2d calib3d)
find_package(Eigen3 3.3 REQUIRED)
find_package(OpenMP REQUIRED)

message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "Eigen3 version: ${Eigen3_VERSION}")
message(STATUS "OpenMP found: ${OpenMP_CXX_FOUND}")

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIRS}
)

# Core Library Sources
set(CORE_SOURCES
    src/core/vegetation_index.cpp
    src/core/mask_generator.cpp
    src/core/pixel_sampler.cpp
    src/core/descriptor.cpp
    src/localization/visual_odometry.cpp
    src/localization/particle_filter.cpp
    src/dataset/dataset_loader.cpp
    src/config/config.cpp
)

add_library(lambda_brief_core STATIC ${CORE_SOURCES})
target_link_libraries(lambda_brief_core PUBLIC
    ${OpenCV_LIBS}
    Eigen3::Eigen
    OpenMP::OpenMP_CXX
)

# Main Executable
add_executable(lambda_brief_loc apps/run_localization.cpp)
target_link_libraries(lambda_brief_loc PRIVATE lambda_brief_core)

# Tests
enable_testing()
add_subdirectory(tests)
