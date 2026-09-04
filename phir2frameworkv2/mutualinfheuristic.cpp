#include "mutualinfheuristic.h"

MutualInfHeuristic::MutualInfHeuristic(STRATEGY s, int id, int cd, double l):Heuristic(s,id,l,cd)
{
    bins_images = 256;

}

void MutualInfHeuristic::updateDroneDescriptor(cv::Mat &drone){
    //HISTOGRAM gray-scale
//    droneDescriptor = computeHistogram(256/25, drone);
    vector<double> histograms;
    float total = 0;
    //CLEANING

    for(int i = 0; i < bins_images; i++)
        histograms.push_back(0);

    cv::cvtColor(drone, drone, CV_BGR2GRAY);

    //COMPUTING THE HISTOGRAM
    for (int i = 0; i < drone.rows; i++){
        for (int j = 0; j < drone.cols; j++){
            int pixel = (int)drone.at<uchar>(j,i);

//            float r = pixel.b * 0.114;//B
//            float g = pixel.g * 0.587;//G
//            float b = pixel.r * 0.299;//R

//            int sum1 = r + g + b;

            histograms[pixel] += 1;
            total++;
        }
    }
    //NORMALIZATION
    for(int i = 0; i < bins_images; i++){
        histograms[i] /= total;
    }
    droneDescriptor = histograms;

}
/*
cv::Mat Utils::getRotatedROIFromImage(Pose3d p, cv::Size2f s, cv::Mat& largeMap)
cv::RotatedRect rRect = cv::RotatedRect(cv::Point2f(p.x,p.y), s, RAD2DEG(p.yaw)+90.0);
 */

double MutualInfHeuristic::calculateValue2(Pose3d p, cv::Mat &drone, cv::Mat *map){
    double MI = 1;
    cv::Mat globalmap = map->clone();

    bool *goodParticle = new bool;
    *goodParticle = true;
    vector<double> particleDescriptor = computeHistogram(p, drone, map, goodParticle);

    if(!goodParticle) return 2;

    vector< vector<double> > jointHist = computeJointHistogram(p, drone, map);

    double entropy_drone = entropy(droneDescriptor);
    double entropy_partic = entropy(particleDescriptor);
    double joint_entropy = jointEntropy(jointHist);

    MI = (entropy_drone + entropy_partic) / joint_entropy;

    return 2.0 - MI;
}

vector<double> MutualInfHeuristic::computeHistogram(Pose3d p, cv::Mat &drone, cv::Mat *globalmap, bool *good){
    vector<double> histograms;
    float total = 0;
    int intensity;

    //CLEANING
    for(int i = 0; i < bins_images; i++)
        histograms.push_back(0);

    double scale = p.z;
    float theta = p.yaw*(+1)-1.5508;
    int xPad = drone.cols/2 - 0.5;
    int yPad = drone.rows/2 - 0.5;


//    double elapsedTime;
//    struct timeval tstart, tend;

//         Start counting the elapsed time of this iteration
//    gettimeofday(&tstart, NULL);

    //COMPUTING THE HISTOGRAM
    for (int i = 0; i < drone.rows; i++){
        for (int j = 0; j < drone.cols; j++){
            cv::Point point1 = cv::Point(((i-xPad)*scale*cos(theta)+(j-yPad)*scale*sin(theta))+p.y,
                                            ((i-xPad)*scale*-sin(theta)+(j-yPad)*scale*cos(theta))+p.x);

            if(!(point1.x < 0 || point1.x >= globalmap->cols || point1.y < 0 || point1.y >= globalmap->rows)){

                cv::Vec3b color1 = globalmap->at<cv::Vec3b>(point1.y, point1.x);

                histograms[(int)color1[0]] += 1;
                total++;
            }
            else
            {
                *good = false;
                return histograms;
            }
        }
    }
//    gettimeofday(&tend, NULL);

//    elapsedTime = ((double)tend.tv_sec - (double)tstart.tv_sec) + ((double)tend.tv_usec - (double)tstart.tv_usec)/1000000.0;
//    std::cout<<elapsedTime<<std::endl;

    //NORMALIZATION
    for(int i = 0; i < bins_images; i++){
        histograms[i] /= total;
    }
    return histograms;
}

vector<vector<double> > MutualInfHeuristic::computeJointHistogram(Pose3d p, cv::Mat &drone, cv::Mat *globalmap){
    vector< vector<double> > jointHistogram;
    float total = 0;
    //CLEANING

    for(int i = 0; i < bins_images; i++){
        vector<double> temp;
        for(int j = 0; j < bins_images; j++){
            temp.push_back(0);
        }
        jointHistogram.push_back(temp);
    }

    double scale = p.z;
    float theta = p.yaw*(+1)-1.5508;
    int xPad = drone.cols/2 - 0.5;
    int yPad = drone.rows/2 - 0.5;

    for(int i = 0; i < drone.rows; i++){
        for(int j = 0; j < drone.cols; j++){
            cv::Point point1 = cv::Point(((i-xPad)*scale*cos(theta)+(j-yPad)*scale*sin(theta))+p.y,
                                            ((i-xPad)*scale*-sin(theta)+(j-yPad)*scale*cos(theta))+p.x);

            if(!(point1.x < 0 || point1.x >= globalmap->cols || point1.y < 0 || point1.y >= globalmap->rows)){

                int pixel_drone = (int)drone.at<uchar>(i,j);

                cv::Vec3b color1 = globalmap->at<cv::Vec3b>(point1.y, point1.x);

                jointHistogram[pixel_drone][(int)color1[0]] += 1;
                total++;
            }
        }
    }

    //NORMALIZATION
    for(int i = 0; i < bins_images; i++)
        for(int j = 0; j < bins_images; j++)
            jointHistogram[i][j] /= total;
    return jointHistogram;
}

double MutualInfHeuristic::entropy(vector<double> histogram){
    double entropy = 0;
    for(int i = 0; i < bins_images; i++){
//        cout<<"Hist[i]:"<<histogram[i]<<" log:"<<log2(histogram[i])<<endl;
        if(histogram[i] != 0)
            entropy -= histogram[i]*log2(histogram[i]);
    }
    return entropy;
}

double MutualInfHeuristic::jointEntropy(vector<vector<double> > jointHistogram){
    double entropy = 0;
    for(int i = 0; i < bins_images; i++){
        for(int j = 0; j < bins_images; j++){
            if(jointHistogram[i][j] != 0)
                entropy -= jointHistogram[i][j]*log2(jointHistogram[i][j]);
        }
    }
    return entropy;
}

double MutualInfHeuristic::calculateValue(int x, int y, cv::Mat *image, cv::Mat* map){
    return 0;
}

bool MutualInfHeuristic::boundingRectIsIn(cv::RotatedRect rRect, cv::Mat map){
    if(rRect.boundingRect().x < 0 ||
       rRect.boundingRect().y < 0 ||
       rRect.boundingRect().x + rRect.boundingRect().width > map.cols ||
       rRect.boundingRect().y + rRect.boundingRect().height > map.rows){
        return false;
    }else{
        return true;
    }
}
