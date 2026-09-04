#ifndef NDVI_H
#define NDVI_H

#include "VegetationIndex.h"
/*
#define T_NDVI          1   //NDVI tradicional
#define T_RE_NDVI       2   //Red-edge NDVI
#define T_RED_RE_NDVI   3   //Red e Red-edge NDVI*/

class Ndvi : public VegetationIndex
{
public:
    //Public Attributes
    enum NdviType { ndvi=1, re_ndvi=2, red_re_ndvi=3 };
    int ndviType;
    bool gravaLogNdvi; //Controla a gravação do log utilizado para comparar os valores calculados para o NDVI.
    //Public Methods
        Ndvi(int _radianceFix);
        Ndvi(int _radianceFix, int _width, int _height);
        Ndvi(int _radianceFix, enum NdviType _type);
        Ndvi(int _radianceFix, int _type);
        /*Calcula Red Edge NDVI para um pixel*/        
        float setIndexPixelReNdvi(double shortWave, double redEdge);
        double setIndexPixelReNdviD(double shortWave, double redEdge);
        float setIndexPixelRedReNdvi(double shortWave, double redEdge, double redVisible);
        double setIndexPixelRedReNdviD(double shortWave, double redEdge, double redVisible);
        cv::Mat setIndexImage(cv::Mat &nirMap,cv::Mat &redMap, cv::Mat &regMap, cv::Mat &greMap);
    //Overridden Methods
        void setIndexSaveImg(unsigned char *shortWave, unsigned char *redVisible, unsigned char *redEdge, unsigned char *greenVisible, unsigned char *result);
        void paint(double const &min, double const &max, double *image, unsigned char *out);
        /*Calcula o NDVI para um pixel*/
        float setIndexPixel(double shortWave, double redVisible);
        double setIndexPixelD(double shortWave, double redVisible);
        float setIndex(double nir, double redEdge, double red, double green);
        double setIndexD(double nir, double redEdge, double red, double green);
};

#endif // NDVI_H
