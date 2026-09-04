#ifndef VEGETATIONINDEX_H
#define VEGETATIONINDEX_H

#include<iostream>
#include<stdio.h>
#include<malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include<opencv2/opencv.hpp>
#include <math.h>
#include <time.h>
#include <fstream>
#include <sstream>

#define BLUE_CHANNEL 0
#define GREEN_CHANNEL 1
#define RED_CHANNEL 2

class VegetationIndex
{
public:
    //Public Attributes
    int radianceFix;//Define se serão calculadas a radiância e a reflectância.

    //Esses parâmetros de radiância, reflectância e a elevação solar, normalmente são obtidos na documentação do sartélite.
    //No caso da Parrot Sequoia esses valoes não serão necessários caso seja efetuado o processo de calibração da câmera.
    //Se esses dados forem necessários por algum outro motivo, estarão salvos nas tags das imagens em TIFF as quais a Sequoia salva.
    double radianceMultRed;
    double radianceAddRed;
    double radianceMultNir;
    double radianceAddNir;
    double radianceMultRedEdge;
    double radianceAddRedEdge;
    double radianceMultGreen;
    double radianceAddGreen;
    double reflectanceMultRed;
    double reflectanceAddRed;
    double reflectanceMultNir;
    double reflectanceAddNir;
    double reflectanceMultRedEdge;
    double reflectanceAddRedEdge;
    double reflectanceMultGreen;
    double reflectanceAddGreen;
    double thetaSe;             //Sun elevation/Solar zenith angle

    //Atributos referentes às imagens
    cv::Mat shortWaveMat;//NIR
    cv::Mat redVisibleMat;
    cv::Mat greenVisibleMat;
    cv::Mat redEdgeMat;
    int width = 0;
    int height = 0;
    double paramA = 0.4; //(0 - 1) The value of parameter “a ” represents the proportion of red reflectance, and the value of (1 –a ) represents the proportion of red-edge reflectance (Vegetation Indices Combining the Red and Red-Edge Spectral Information for Leaf Area Index Retrieval)

    //Public Methods
        VegetationIndex();
        void setColor(unsigned char *image, int pos, int r, int g, int b);
        void setReflectance(unsigned char *data, int const &band);
        void setRadiance(unsigned char *data, int const &band);
        double getReflectance(int pixel, int band);
        double getRadiance(int grayPixel, int band);

    //Virtual Methods
        /*Aplica índice de vegetação e salva a imagem já com as cores alteradas.*/
        virtual void setIndexSaveImg(unsigned char *shortWave, unsigned char *redVisible, unsigned char *redEdge, unsigned char *greenVisible, unsigned char *result) = 0;
        /*Substitui as cores dos pixels da imagem por uma repesentação vinculada ao índice em questão.*/
        virtual void paint(double const &min, double const &max, double *image, unsigned char *out) = 0;
        /*Calcular o índice para 1 pixel*/
        virtual float setIndex(double nir, double redEdge, double red, double green) = 0;
        virtual double setIndexD(double nir, double redEdge, double red, double green) = 0;
};

#endif // VEGETATIONINDEX_H
