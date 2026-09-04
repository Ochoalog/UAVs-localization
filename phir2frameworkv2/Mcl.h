#ifndef MCL_H
#define MCL_H

class MCL;

#include <GL/glew.h>
#include "Robot.h"
#include "MapGrid.h"
#include "GlutClass.h"
#include "vec3.h"
#include <GL/glut.h>
#include <opencv2/core/core.hpp>
#include "densityheuristic.h"
#include "colorheuristic.h"
#include "mutualinfheuristic.h"
#include "miheuristic.h"
#include "BriefHeuristic.h"
#include <omp.h>
#include <limits>

typedef struct{
    Pose3d p;
    double w; //WEIGHT
    double s; //SCALE
} MCLparticle;

class MCL
{
    public:
        MCL(vector<Heuristic*>& hVector, vector<MapGrid *> &cMaps, vector<cv::Mat> &gMaps, Pose3d &initial, string &lName, int &numParticles, vector<Pose3d>droneAngles, vector<Pose3d>CorrectedGT);
        ~MCL();

        bool run(Pose3d &u, bool is_u_reliable, cv::Mat &z, double time, Pose3d &real, cv::Mat &redMap, cv::Mat &nirMap, cv::Mat &regMap, cv::Mat &greMap, cv::Mat &currentMapOriginal);
        bool initialRun(Pose3d &u, bool is_u_reliable, cv::Mat &z, double time, Pose3d &real, double lastTotalElapsed, cv::Mat &redMap, cv::Mat &nirMap, cv::Mat &regMap, cv::Mat &greMap, cv::Mat &currentMapOriginal, Pose3d orientacao);
        void writeErrorLogFile3d(double trueX, double trueY, double trueTh);
        void writeLogFile3d();
        void draw(int x_aux, int y_aux, int halfWindowSize);
        void restart(Pose3d &initial, string &lName);
        void setNumParticles(int _numParticles);

        // Required to draw
        vector<MCLparticle> particles;
        vector<cv::Mat>& globalMaps;
        GLuint imageTex;

    private:
        fstream particleLog;
        int numParticles;
        int resamplingThreshold;
        double maxRange;
        double neff;
        double minscale;
        double maxscale;
        float sumW;
        Pose3d lastOdometry;
        STRATEGY locTechnique;
        Pose3d realPose;
        Pose3d odomPose;
        vector<Pose3d> realPath;
        vector<Pose3d> odomPath;
        vector<Pose3d> meamParticlePath; //Média das partículas
        vector<Pose3d> varParticlePath; //Variância das partículas
        vector<Pose3d>droneAngles;
        vector<Pose3d>CorrectedGT;

        vector<Heuristic*>& heuristics;
        vector<MapGrid*>& cachedMaps;
        vector<double> heuristicValues;
        vector<double> heuristicGradients;
        vector<cv::Mat> frameColorConverted;
        cv::Mat binaryFrameMask;

        bool removeDuplicates;
        bool starting;

        Timer timer;
        vector<float> ztable;
        vector<vector<bool> > bins;

        void sampling(Pose3d &u, bool reliable);
        MCLparticle samplingKLD(Pose3d &u, bool reliable, MCLparticle part);
        void weighting(cv::Mat& z_robot, Pose3d &u, cv::Mat &redMap, cv::Mat &nirMap, cv::Mat &regMap, cv::Mat &greMap, cv::Mat &currentMapOriginal);
        void prepareWeighting(cv::Mat &z);
        void prepareWeighting(cv::Mat &z, cv::Mat &redMap, cv::Mat &nirMap, cv::Mat &regMap, cv::Mat &greMap, cv::Mat &currentMapOriginal, Pose3d orientacao);
        void resampling();
        MCLparticle resamplingKLD();

        //void weightingSSD(cv::Mat& z_robot);
        //void weightingDensity(vector<int>& densities, Pose &u, vector<double> &gradients);
        //void weightingColor();

        void discardInvalidDeltaAngles(Pose3d &u, vector<double> &gradients);
        double computeNeff();
        double computeError(double trueX, double trueY,double particleX, double particleY);
        double computeError3d(double trueX, double trueY,double trueZ,double particleX, double particleY,double particleZ);
        double computeAngleError(double trueTh, double particleTh);
        double sumAngles(double a, double b);
        void createColorVersions(cv::Mat& imageRGB);
        void kldSampling(Pose3d &u, bool is_u_reliable, cv::Mat &z, double time, Pose3d& real, cv::Mat &redMap, cv::Mat &nirMap, cv::Mat &regMap, cv::Mat &greMap, cv::Mat &currentMapOriginal);
        void normalizingParticles();
        void buildTableZ();
        void setingEmptyBins();
        bool isEmpty(MCLparticle part);
        float getingZValue(float confidence);
        void updateOdomPath(Pose3d &u);
        bool verifyParticlePosition(MCLparticle part);
        //Compensa a posiçao de todas as partículas, baseado nos ângulos (roll, pitch e yaw) do drone naquele momento(IMU).
        void compensateCloudPosition();
        //Compensa a posiçao de uma partícula, baseado nos ângulos (roll, pitch e yaw) do drone naquele momento(IMU).
        void compensateParticlePosition(MCLparticle part);
};

#endif // MCL_H
