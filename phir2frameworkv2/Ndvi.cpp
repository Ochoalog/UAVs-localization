#include "Ndvi.h"
#include "config.h"
#include "Utils.h"

Ndvi::Ndvi(int _radianceFix)
{
    gravaLogNdvi = false;
    this->radianceFix = _radianceFix;
    this->ndviType = ndvi;
}

Ndvi::Ndvi(int _radianceFix, int _width, int _height)
{
    this->radianceFix = _radianceFix;
    this->width = _width;
    this->height = _height;
    this->ndviType = ndvi;
}

Ndvi::Ndvi(int _radianceFix, enum NdviType _type)
{
    gravaLogNdvi = false;
    this->radianceFix = _radianceFix;
    this->ndviType = _type;
}

Ndvi::Ndvi(int _radianceFix, int _type)
{
    gravaLogNdvi = false;
    this->radianceFix = _radianceFix;
    this->ndviType = _type;
}

float Ndvi::setIndex(double nir, double redEdge, double red, double green)
{
    switch(this->ndviType)
    {
        case(ndvi):
            return setIndexPixel(nir, red);
        case(re_ndvi):
            return setIndexPixelReNdvi(nir, redEdge);
        case(red_re_ndvi):
            return setIndexPixelRedReNdvi(nir, redEdge, red);
    }
}

double Ndvi::setIndexD(double nir, double redEdge, double red, double green)
{
    switch(this->ndviType)
    {
        case(ndvi):
            return setIndexPixelD(nir, red);
        case(re_ndvi):
            return setIndexPixelReNdviD(nir, redEdge);
        case(red_re_ndvi):
            return setIndexPixelRedReNdviD(nir, redEdge, red);
    }
}

float Ndvi::setIndexPixel(double shortWave, double redVisible)
{
    //return pow((shortWave - redVisible) / (shortWave + redVisible),2);
    return (shortWave - redVisible) / (shortWave + redVisible);
}

double Ndvi::setIndexPixelD(double shortWave, double redVisible)
{
    //return pow((shortWave - redVisible) / (shortWave + redVisible),2);
    return (shortWave - redVisible) / (shortWave + redVisible);
}


float Ndvi::setIndexPixelReNdvi(double shortWave, double redEdge)
{
    return (shortWave - redEdge) / (shortWave + redEdge);
}

double Ndvi::setIndexPixelReNdviD(double shortWave, double redEdge)
{
    return (shortWave - redEdge) / (shortWave + redEdge);
}

float Ndvi::setIndexPixelRedReNdvi(double shortWave, double redEdge, double redVisible)
{
    return (shortWave - (paramA * redVisible + (1 - paramA) * redEdge))/
           (shortWave + (paramA * redVisible + (1 - paramA) * redEdge));
}

double Ndvi::setIndexPixelRedReNdviD(double shortWave, double redEdge, double redVisible)
{
    return (shortWave - (paramA * redVisible + (1 - paramA) * redEdge))/
           (shortWave + (paramA * redVisible + (1 - paramA) * redEdge));
}

void Ndvi::setIndexSaveImg(unsigned char *shortWave, unsigned char *redVisible, unsigned char *redEdge, unsigned char *greenVisible, unsigned char *result)
{
    int posGray;
    double minValue = -1.0, maxValue = 1.0;
    double value = 0.0;
    int size = sizeof(double) * this->width * this->height;
    double *temp;
    double nir = 0, red = 0, reg = 0, gre = 0;
    int i, posResult;
    temp = (double*)malloc(size);

    /////LOG
    std::ofstream file;
    std::stringstream ss;
    if(this->gravaLogNdvi)
        file.open("../phir2framework/Logs_NDVI/ndvi.txt");

    for (int row = 0; row < this->height; row++){
        for (int col = 0; col < width; col++)
        {
            posGray = (row * this->width) + col;

            if(radianceFix)
            {
                nir = getReflectance(shortWave[posGray], 4);
                red = getReflectance(redVisible[posGray], 3);
            }
            else
            {
                nir = shortWave[posGray];
                red = redVisible[posGray];
                reg = redEdge[posGray];
                gre = greenVisible[posGray];
            }

            if (nir + red + reg != 0)
            {
                value = setIndex(nir, reg, red, gre);
                temp[posGray] = (double)value;
            }
            else
            {
                temp[posGray] = (double)redVisible[posGray];
            }
            if(this->gravaLogNdvi)
            {
                ss << " row (y):    "   << row
                   << " col (x):    "   << col
                   << " shortWave:  "   << (float)shortWave[posGray]
                   << " redVisible: "   << (float)redVisible[posGray]
                   << " ndvi:       "   << value << "\n";
            }
        }
    }

    if(this->gravaLogNdvi)
    {
        file << ss.str();
        file.close();
    }

    this->paint(-1, 1, temp, result);
    free(temp);
}


void Ndvi::paint(double const &min, double const &max, double *image, unsigned char *out)
{
    int posGray, posResult;
    double value = 0.0;
    for (int row = 0; row < height; row++){
        for (int col = 0; col < width; col++){
            posGray = (row*width)+col;
            posResult = ((row*width)+col)*3;

            value = image[posGray];

            setColor(out, posResult, Utils::Normalize(value, 0, 1, -1, 2)*255, 150,255 - 255*Utils::Normalize(value, 0, 1, -1, 2));


//            if (value > 0.941)
//                setColor(out, posResult, 0,102,0);

//            else if (value > 0.824 && value <= 0.941)
//                setColor(out, posResult, 0,136,0);

//            else if (value > 0.706 && value <= 0.824)
//                setColor(out, posResult, 0,187,0);

//            else if (value > 0.588 && value <= 0.706)
//                setColor(out, posResult, 0,255,0);

//            else if (value > 0.471 && value <= 0.588)
//                setColor(out, posResult, 204,255,0);

//            else if (value > 0.353 && value <= 0.471)
//                setColor(out, posResult, 255,255,0);

//            else if (value > 0.235 && value <= 0.353)
//                setColor(out, posResult, 255,204,0);

//            else if (value > 0.118 && value <= 0.235)
//                setColor(out, posResult, 255,136,0);

//            else if (value > 0.000 && value <= 0.118)
//                setColor(out, posResult, 255,0,0);

//            else if (value <= 0.000 && value > -0.118)
//                setColor(out, posResult, 238,0,0);

//            else if (value <= -0.118 && value > -0.235)
//                setColor(out, posResult, 221,0,0);

//            else if (value <= -0.235 && value > -0.353)
//                setColor(out, posResult, 204,0,0);

//            else if (value <= -0.353 && value > -0.471)
//                setColor(out, posResult, 187,0,0);

//            else if (value <= -0.471 && value > -0.588)
//                setColor(out, posResult, 170,0,0);

//            else if (value <= -0.588 && value > -0.706)
//                setColor(out, posResult, 153,0,0);

//            else if (value <= -0.706 && value > -0.824)
//                setColor(out, posResult, 136,0,0);

//            else if (value <= -0.824 && value > -0.941)
//                setColor(out, posResult, 119,0,0);

//            //else if (value <= -0.941 && value > -1)
//            else if (value <= -0.941)
//                setColor(out, posResult, 102,0,0);
        }
    }
}
