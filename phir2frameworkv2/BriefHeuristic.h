#ifndef BRIEFHEURISTIC_H
#define BRIEFHEURISTIC_H

#include "Heuristic.h"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/xfeatures2d.hpp"
#include "ColorCPU.h"
#include "RadiusVolumeTransferFunctions.h"
#include "Ndvi.h"
#include "VegetationIndex.h"
#include "config.h"
#include "Utils.h"
#include <exception>
#include <thread>
#include "VegetatonIndexGen.h"

class BriefHeuristic : public Heuristic
{
public:
    BriefHeuristic(STRATEGY s, int id, int cd, double l);
    double calculateValue(int x, int y, cv::Mat *image, cv::Mat* map=NULL);
    double calculateValue2(Pose3d p, cv::Mat *map);
    double calculateValue2(Pose3d p, cv::Mat *map, cv::Mat &redMap, cv::Mat &nirMap, cv::Mat &regMap, cv::Mat &greMap, cv::Mat &currentMapOriginal, Pose3d orientacao);
    double abBriefParicle(Pose3d dronePose, cv::Mat *map, cv::Mat &redMap, cv::Mat &nirMap, cv::Mat &regMap, cv::Mat &greMap, cv::Mat &currentMapOriginal, Pose3d orientacao, bool GT);
    void abBriefParicleProb(Pose3d GTpose, Pose3d correctedGTpose, cv::Mat *map, cv::Mat &redMap, cv::Mat &nirMap, cv::Mat &regMap, cv::Mat &greMap, cv::Mat &currentMapOriginal, Pose3d orientacao);
    bool pointIn(cv::Point points, cv::Mat image);
    void updateDroneDescriptor(cv::Mat& drone);
    void updateDroneDescriptor(cv::Mat& drone, cv::Mat &redMap, cv::Mat &nirMap, cv::Mat &regMap, cv::Mat &greMap, cv::Mat &currentMapOriginal, Pose3d orientacao, Pose3d realPose);
    void printInfo();
    void CalculateVIMask(cv::Mat &redMap, cv::Mat &nirMap, cv::Mat regMap, cv::Mat greMap, cv::Mat viMatrix);
    void drawPixelDistribution(cv::Mat& localMap);
    void pixelDistribution(cv::Mat& drone, cv::Mat &nirMap,cv::Mat &redMap, cv::Mat &regMap, cv::Mat &greMap, cv::Mat viMatrix);
    void gaussianMask(cv::Size &size, cv::Mat &output, int x0, int y0, float sigmaX, float sigmaY, float amplitude = 1.0f);
    void gaussianMaskD(cv::Size &size, cv::Mat &output, int x0, int y0, double sigmaX, double sigmaY, double amplitude = 1.0f);
    void visualizaCameraPoseOriCorr(Pose3d pose, Pose3d orientacao, bool invYaw);
    void visualizaCameraPose(Pose3d pose, Pose3d orientacao, bool GT);
    int totalPairs;
    float lowThreshold;
    float multiplierThreshold;
    int margin;
    int width, height;
    double totPx; //Número de pixeis da imagem
    vector<int> droneDescriptor;
    int type;
    vector< vector<cv::Point> > pairs;
    cv::Point transform(cv::Point pt, cv::Mat rot, cv::Point trans, int max_x, int max_y);
    void perspectiveProjection(double z, cv::Mat source, double roll, double pitch, double yaw, int width, int height);

    //Máscara de pesos gerada após o cálculo do VI - Vegetation Index
    vector< float > pairWeight;//Peso atribuido a cada par de pixeis, após o cálculo do NDVI.
    vector< vector<float> > pairVI;
    double sumNdviWeightsDrone;
    std::stringstream abBRIEFLog;
    //float viMatrix[913][1241];//float viMatrix[rows][cols]; tamanho das imagens 1247, 917
    //float viMatrix[917][1247];//float viMatrix[rows][cols]; tamanho das imagens 1247, 917
};

#endif // BRIEFHEURISTIC_H
