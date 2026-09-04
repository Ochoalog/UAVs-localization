#ifndef CONFIG_H
#define CONFIG_H
#include <strings.h>
#include <opencv2/core/core.hpp>

//Geral
extern int pixelDistMethod; //1 Gaussian, 2 Fitness proportionate selection, 3 Random Pool Selection (Rejection sampling)
extern int msImgPreProc; //1 filtro gaussiano, 2 filtro bilateral, 0 sem filtro
extern int mapPreProc; //1 quantization, 2 bilateral filtering
//extern float maxNdviWeight;
//extern float minNdviWeight;
extern int ndvi_type;
extern int grvi_type;
extern double minScale;
extern double maxScale;
extern bool multispectralCam;
extern double gbSigma;//Gaussian Blur - sigma
extern double gbKernel;//Gaussian Blur - kernel size
extern int bfD;//Biltaeral Filter - Diameter of each pixel neighborhood that is used during filtering. If it is non-positive, it is computed from sigmaSpace.
extern double bfSigmaColor;//Biltaeral Filter - sigma color
extern double bfSigmaSpace;//Biltaeral Filter - sigma space
extern int poolSelFit;//0: funcionamento defaut, 2: desconsiderar toda a vegetação, 2: desconsiderar a vegetação comum, trbalhando apenas com a mais densa
extern float multiGaussianMask;//Multiplicador da máscara gaussiana utilizada em conjunto com a roleta pelo VI. Ex: com 0.5, a máscara gaussiana só exercerá 50% de influência...
extern bool quiet;//Exibir ou não a inteface gráfica
extern bool compensateParticles;//Compensar as partículas com base na orientação do drone.
extern int currentImage;//Inicializado em 0 (currentImage + iniexec + 1 = current image number)
//extern double escalaParaPixel;//Dividir por esse valor para converter de escala para pixeis.
//extern double pixelParaEscala;//Multiplicar por esse valor para converter de pixeis para escala.
extern bool imgMapaSalva;//Indica que a imagem da última versão do mapa já foi salva
extern double tmpExecPxDist;//Conta o tempo total de execução das chamadas à distribuição dos pixeis.
extern int qtdChamadasPxDist;//Quantidade de vezes que a distribuição de pixeis foi chamada.
extern double tmpExecPxCalc;//Conta o tempo total de execução das chamadas ao cálculo das posições pixeis.
extern int qtdChamadasPxCalc;//Quantidade de vezes que o cálculo das posições dos píxeis foi chamado
extern double multExcalaZ;//Multiplicador utilizado p converter de escala p pixeis
extern cv::Mat originalMap;//Mapa global original, sem nenhum processamento
extern bool corrPixelPosAngles;//Corrigir as posicoes dos pixeis com base na orientacao do drone.
extern int rollPitchAngRange;//Range em  que roll e pitch podem variar, na estimativa de partículas
//extern float alphaGaussianMask;//Utilizado para definir a relevância da máscara gaussiana, em relação à máscara de vegetação.
extern float alphaVegetationMask;//Utilizado para definir a relevância da máscara de vegetação, em relação à máscara de vegetação.
extern float betaHammingWeight;//Utilizado no cálculo do peso dos pares de píxeis, baseado na máscara de vegetação NDVI.


bool const experimentosRollPitchEstimation = false;
bool const RAW_ODOM           = false;    //Odometria utilizando os dados do ground truth (vinculado ao flag "flPrimeiraLeituraGTRawOdom")
int  const NUM_THREADS        = 8;
bool const fixedRandomSeed    = false;
bool const VIDouble = false;//Todos os cálculos envolvendo a VI, como NDVI, e distribuição de píxeis serão efetuados utilizando double, ao invés de floas.
//NÃO ATIVAR ESSE FLAG DE DOUBLE, COLOCAR MANUALMENTE O DOUBLE APENAS AN DISTRIBUIÇÃO DE PIXEIS

// BRIEF HEURISTIC ====================
/* 0 - gray                     = 1 bit   (Utilizar nos datasets em grayscale)
 * 1 - Lab                      = 3 bits
 * 2 - Lab                      = 6 bits
 * 3 - Integer Difference       = Other :p
 * 4 - Lab wo L                 = 2 bits    //LAB without L
*/
int const BRIEF_HEURISTIC_TYPE = 4; //ESTAMOS USANDO O LOWTHRESHOLD APENAS PRA NAO TER QUE MUDAR O CODIGO. ESSE "TYPE" AQUI
                     //REPRESENTA A QUANTIDADE DE BITS DA IMAGEM QUE ESTÁ SENDO USADA.
//type = 4=>>> abBRIEF com a config utilizada no trabalho do Mathias.
//type = 0 =>>> BRIEF default, em grayscale...
//==========================

//Odometria =================
int const ODOM_TYPE = 1; //1 = SURF, 2 = SIFT, 3 = ORB, 4 = ECC, 5 = ICP, 6 = Correlative SM e 7 = template matching
    //Pré processamento das imagens
    bool const USE_SOBEL = false; //Usar Sobel para detectar linhas, antes de processar a odometria.
    bool const USE_NDVI = false;  //Usar a imagem em NDVI para ajudar a difereniar regiões com vegetação aparentemente homogênea (apenas odometria)   
//==========================
//                              |VET 100M   |UFRGS traj 1   |VET 50M
extern int imgIniDs;        //  |186        |1              |179
extern int imgIniExec;      //  |390        |1              |418
extern int imgFimDs;        //  |719        |147            |754
extern int imgFimExec;      //  |613        |147            |754

extern double percentualPxVegImg;

//imfim 613
#endif // CONFIG_H
