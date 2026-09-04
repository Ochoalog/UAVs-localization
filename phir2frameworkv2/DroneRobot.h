#ifndef DRONEROBOT_H
#define DRONEROBOT_H

#define USE_SURF 1

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

//using namespace cv;

#include "Robot.h"
#include "densityheuristic.h"
#include "colorheuristic.h"
#include "mutualinfheuristic.h"
#include "miheuristic.h"
#include "MeanShiftHeuristic.h"
#include "SiftHeuristic.h"
#include "BriefHeuristic.h"
#include "CorrelativeSM.h"
#include "Utils.h"
#include "config.h"
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/registration/icp.h>

int selectMapID(int colorDiff);

class DroneRobot: public Robot
{
public:
    DroneRobot();
    DroneRobot(string& mapPath, string& trajectoryPath, vector< heuristicType* > &heuristicTypes, bool quiet, string& outputName, int start, int finish);
    ~DroneRobot();

    void initialize(ConnectionMode cmode, LogMode lmode, string fname, int numParticles);
    //void initialize(int numParticles);
    void run();

private:

    int getNextProperHeuristicID(STRATEGY type);
    void generateObservations(string imagePath);
    bool readRawOdometryFromFile(Pose3d& p);
    Pose3d readOdometry();
    pair<Pose3d, bool> readOdometryNew();    
    Pose3d readGroundTruth();
    void readCorrectedGroundTruth();
    pair<Pose3d, bool> findOdometryUsingECC(cv::Mat &prevImage, cv::Mat &curImage);
    pair<Pose3d, bool> findOdometry(cv::Mat &prevImage, cv::Mat &curImage);
    pair<Pose3d, bool> findOdometryUsingFeatures(cv::Mat &prevImage, cv::Mat &curImage, double cT=0.04);
    pair<Pose3d,bool>  findOdometryUsingFeaturesSurf(cv::Mat& prevImage, cv::Mat& curImage, double cT);
    pair<Pose3d,bool>  findOdometryUsingFeaturesSift(cv::Mat& prevImage, cv::Mat& curImage, double cT);
    pair<Pose3d,bool>  findOdometryUsingFeaturesOrb(cv::Mat& prevImage, cv::Mat& curImage, double cT);
    pair<Pose,bool> findOdometryUsingICP(cv::Mat& prevImage, cv::Mat& curImage);
    pair<Pose, bool> findOdometryUsingCorrelativeSM(cv::Mat& prevImage, cv::Mat& curImage);
    pair<Pose3d,bool> findOdometryUsingTemplateMatching(cv::Mat& prevImage, cv::Mat& curImage);
    cv::Mat GetGradient(cv::Mat src_gray);
    cv::Mat quantization(cv::Mat &curImage, float value);
    cv::Mat msImagePreProcessing(cv::Mat &img);
    void drawMatchedImages(cv::Mat& prevImage, cv::Mat& curImage, const cv::Mat& warp_matrix, const int warp_mode = cv::MOTION_EUCLIDEAN);
    void reinitialize();
    void initializeFeatureMatching();
    void localizeWithTemplateMatching(cv::Mat &currentMap);
    void localizeWithFeatureMatching(cv::Mat& currentMap);
    void localizeWithHierarchicalFeatureMatching(cv::Mat& currentMap);
    void readDroneAngles();

    bool slowMethod;
    bool offlineOdom;
    bool isRawOdom;
    bool availableGTruth;
    bool availableCorrectedGTruth;
    bool availableAngles;
    bool isBRIEF;
    Pose3d prevRawOdom;
    Pose3d prevOdometry;
    Pose3d realPose;
    fstream odomFile;
    fstream truthFile;
    fstream corrTruthFile;
    fstream anglesFile;
    vector<Pose3d>droneAngles;
    Pose3d curentAngles;
    vector<Pose3d>CorrectedGT;

    int start;
    int finish;
    int current;
    string rawname;
    string outputName;

    STRATEGY locTechnique;

    // Heuristics vectors
    vector<Heuristic*> heuristics;
    vector<MapGrid*> cachedMaps;

//    vector<ColorHeuristic*> ssdHeuristics;
//    vector<ColorHeuristic*> colorHeuristics;
//    vector<DensityHeuristic*> densityHeuristic;

    vector<cv::Mat> globalMaps;
    vector<string> imagesNames;

    //As imagens RED, NIR e REG serão utilizadas no cálculo dos diferentes índices, inclusive o NDVI
    vector<string> imageNamesRED;
    vector<string> imageNamesNIR;
    vector<string> imageNamesREG;
    vector<string> imageNamesGRE;

    // Used for feature matching
    cv::Ptr<cv::Feature2D> feature_detector;
    cv::Ptr<cv::Feature2D> feature_extractor;
    cv::FlannBasedMatcher feature_matcher;
    std::vector<cv::KeyPoint> keypoints_globalMap;
    cv::Mat descriptors_globalMap;
    vector< vector<cv::FlannBasedMatcher*> > hMatcher;
    vector< vector< vector<unsigned int>* > > idKeypoints;
    cv::Mat likelihood;

    cv::Mat prevMap;
    cv::Mat currentNIRMap;
    cv::Mat currentREDMap;
    cv::Mat currentREGMap;
    cv::Mat currentGREMap;
    cv::Mat currentNDVIMap;
    cv::Mat prevNDVIMap;

    unsigned int step;
    int numQuantization;

    bool flPrimeiraLeituraGTRawOdom; //Quando for usado raw odom, esse flag controla a sincronia entre a odometria e o gorund truth, pq senão ocorre de o GT ficar uma poe a frente da odometria.

};

#endif // DRONEROBOT_H
