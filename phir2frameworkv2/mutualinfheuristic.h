#ifndef MUTUALINFHEURISTIC_H
#define MUTUALINFHEURISTIC_H
#include "Heuristic.h"
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/xfeatures2d.hpp"
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "ColorCPU.h"
#include "RadiusVolumeTransferFunctions.h"

#include "Utils.h"
#include <unistd.h>

class MutualInfHeuristic : public Heuristic
{
public:
    MutualInfHeuristic(STRATEGY s, int id, int cd, double l);

    double calculateValue(int x, int y, cv::Mat *image, cv::Mat* map);
    double calculateValue2(Pose3d p, cv::Mat& drone, cv::Mat *map);
//    vector<double> computeHistogram(cv::Mat image);
    vector<double> computeHistogram(Pose3d p, cv::Mat &drone, cv::Mat *map, bool *good);
//    vector< vector<double> > computeJointHistogram(cv::Mat drone, cv::Mat particle);
    vector< vector<double> > computeJointHistogram(Pose3d p, cv::Mat &drone, cv::Mat *map);
    double entropy(vector<double> histogram);
    double jointEntropy(vector<vector<double> > jointHistogram);
    void updateDroneDescriptor(cv::Mat& drone);
    bool boundingRectIsIn(cv::RotatedRect rRect, cv::Mat map);


    vector<double> droneDescriptor;

    int bins_images;




};

#endif // MUTUALINFHEURISTIC_H
