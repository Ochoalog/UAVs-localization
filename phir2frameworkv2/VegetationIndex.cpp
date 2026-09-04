#include "VegetationIndex.h"

VegetationIndex::VegetationIndex()
{

}

void VegetationIndex::setColor(unsigned char *image, int pos, int r, int g, int b)
{
    image[pos + BLUE_CHANNEL] = b;
    image[pos + GREEN_CHANNEL] = g;
    image[pos + RED_CHANNEL] = r;
}

double VegetationIndex::getRadiance(int grayPixel, int band)
{
    if (grayPixel < 5)
        return 0;

    if (band == 3)
        return grayPixel * this->radianceMultRed + this->radianceAddRed;
    return grayPixel * this->radianceMultNir + this->radianceAddNir;
}

double VegetationIndex::getReflectance(int pixel, int band){
    if (pixel < 5)
        return 0;

    double theta = (this->thetaSe * M_PI) / 180;
    if (band == 3)
        return (this->reflectanceMultRed * pixel + this->reflectanceAddRed) / sin(theta);
    return (this->reflectanceMultNir * pixel + this->reflectanceAddNir) / sin(theta);
}

void VegetationIndex::setRadiance(unsigned char *data, int const &band)
{
    double value = 0.0;
    int pos;
    for (int row = 0; row < this->height; row++){
        for (int col = 0; col < this->width; col++){
            pos = row * this->width + col;
            value = getRadiance(data[pos], band);
            data[pos] = value;
        }
    }
}

void VegetationIndex::setReflectance(unsigned char *data, int const &band)
{
    double value = 0.0;
    int pos;
    for (int row = 0; row < this->height; row++){
        for (int col = 0; col < this->width; col++){
            pos = row * this->width + col;
            value = getReflectance((int)data[pos], band);
            data[pos] = (double)value;
        }
    }
}
