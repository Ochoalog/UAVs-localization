#ifndef GRVI_H
#define GRVI_H

#include "VegetationIndex.h"

//#define T_GRVI          1   //GRVI tradicional

class Grvi : public VegetationIndex
{
public:    
    //Public Attributes
    enum GrviType { grvi=1 };
    int grviType;
    //Public Methods
        Grvi();
        Grvi(enum GrviType _type);
        Grvi(int _type);
    //Overridden Methods
        void setIndexSaveImg(unsigned char *shortWave, unsigned char *redVisible, unsigned char *redEdge, unsigned char *greenVisible, unsigned char *result);
        void paint(double const &min, double const &max, double *image, unsigned char *out);
        float setIndexPixel(double green, double red);
        double setIndexPixelD(double green, double red);
        float setIndex(double nir, double redEdge, double red, double green);
        double setIndexD(double nir, double redEdge, double red, double green);
        cv::Mat setIndexImage(cv::Mat &nirMap,cv::Mat &redMap, cv::Mat &regMap, cv::Mat &greMap);
};

#endif // GRVI_H
