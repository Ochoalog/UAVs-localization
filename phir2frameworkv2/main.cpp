#include <pthread.h>
#include <iostream>
#include <iostream>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include "config.h"
#include "SomeKernels.h"
#include "densityheuristic.h"
#include "Robot.h"
#include "DroneRobot.h"
#include "GlutClass.h"
#include "SiftHeuristic.h"

using std::cout;
using std::endl;
using std::cerr;
using namespace std;

/*
Comando de exemplo, atualizado: (muitos defaults implícitos, como o uso da multiespectral e do NDVI comum)
-e /home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_13_06_2017.jpg -t /home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/traj_odom/170-387/traj.txt -s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -msc 1 -ndvit 1 -minsc 0,1 -maxsc 3 -msimp 0  -pdm 2 -fit 0 -inids 85 -iniexec 170 -fimds 498 -fiexec 387 -alfavismsk  0.5  -betaweight 0.5
-e /home/raziel/Documents/Datasets/Sequoia_Matrice100_05_02_2019/voo_100m/Mapa/UFRGS_VET_V3_17_12_2002.jpg -t /home/raziel/Documents/Datasets/Sequoia_Matrice100_05_02_2019/voo_100m -s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 20000 -ndvit 1 -minsc 0.5 -maxsc 5 -pdm 2 -inids 186 -iniexec 390 -fimds 719 -fiexec 405 -alfagaussmsk 0 -betaweight 0
*/

ConnectionMode connectionMode;
LogMode logMode;
string filename;
pthread_mutex_t* mutex;
bool quiet=false;
int numParticles = 0;
//float maxNdviWeight = 0;
//float minNdviWeight = 0;
bool multispectralCam=true;//Por default utilizo a multiespectral, não precisa informar o parâmetro.
int ndvi_type = 1; //Default: NDVI comum
int grvi_type = 0;
double minScale = 0;
double maxScale = 0;
int msImgPreProc = 0; //Default: without pre-processing
double gbSigma = 0;
double gbKernel = 0;
int bfD = 0;
double bfSigmaColor = 0;
double bfSigmaSpace = 0;
int mapPreProc = 1; //Default: quantization
int pixelDistMethod = 1;
int poolSelFit = 0;
int imgIniDs = 0;
int imgIniExec = 0;
int imgFimDs = 0;
int imgFimExec = 0;
float multiGaussianMask = 0;
float alphaVegetationMask = 0;
float betaHammingWeight = 0;//Utilizado no cálculo do peso dos pares de píxeis, baseado na máscara de vegetação NDVI.
bool compensateParticles = 0;
int currentImage = 0;//Inicializado em 0 (currentImage + iniexec + 1 = current image number)
//double escalaParaPixel = (0,366197183 * 45/0,188679245);//Dividir por esse valor para converter de escala para pixeis.
//double pixelParaEscala = (0,366197183 / 45*0,188679245);//Multiplicar por esse valor para converter de pixeis para escala.
bool imgMapaSalva = false;//Indica que a imagem da última versão do mapa já foi salva
double tmpExecPxDist = 0;//Conta o tempo total de execução das chamadas à distribuição dos pixeis.
int qtdChamadasPxDist = 0;//Quantidade de vezes que a distribuição de partículas foi chamada.
double tmpExecPxCalc = 0;//Conta o tempo total de execução das chamadas ao cálculo das posições pixeis.
int qtdChamadasPxCalc = 0;//Quantidade de vezes que o cálculo das posições dos píxeis foi chamado
double multExcalaZ = (45/0.188679245);//Multiplicador utilizado p converter de escala p pixeis
cv::Mat originalMap;//Mapa global original, sem nenhum processamento
bool corrPixelPosAngles;//Corrigir as posicoes dos pixeis com base na orientacao do drone.
int rollPitchAngRange;//Range em  que roll e pitch podem variar, na estimativa de partículas
double percentualPxVegImg = 0;

void* startRobotThread (void* ref)
{
    Robot* robot=(Robot*) ref;
    int k = 0;

    robot->initialize(connectionMode, logMode, filename, numParticles); //GO TO DRONEROBOT
    while(robot->isRunning()){
        cout<<"                                            K = "<<k++<<endl;
        if(k > 6)
            sleep(1);
        robot->run();
    }
    return NULL;
}

void* startGlutThread (void* ref)
{
    sleep(2);
    GlutClass* glut=GlutClass::getInstance();
    glut->setRobot((Robot*) ref);

    glut->initialize();

    glut->process();

    return NULL;
}

// Standard Input Error Message
bool errorMessage(int position=-1, std::string message="")
{
    // Detailed message
    if(message.size()!=0)
        cerr << message << endl;

    // Print character position if available
    if(position>0)
        cerr << "Failure at input element "<< position << endl;

    // Standard error message
    cerr << "Usage: PhiR2Framework -s <diff-intensity|diff-rgb|diff-cie1976|diff-cmc1984|diff-cie1994|diff-cie2000|diff-cie1994mix|diff-cie2000mix> <threshold> <gaussian|circular|inverted> <radius>" << endl;

    return false;
}

bool config(int argc, char* argv[], vector< heuristicType* > &heuristicTypes, std::string& mp, std::string& tp, std::string& op, int& start, int& finish)
{
    //data to collect
    cv::Mat image;
    cv::Mat map;
    std::string outputName;
    int p=1;

    quiet=false;    //show the graphical interface (false = show, true = no show)

    while(p<argc)
    {
        // print help and exit
        if(!strncmp(argv[p], "-h", 2) || !strncmp(argv[p], "-H", 2) || !strncmp(argv[p], "--help", 6))
        {
            cout << "Usage: PhiR2Framework -e <path/> -o <path/outputDir/> -s <density|ssd|entropy> <diff-intensity|diff-rgb|diff-cie1976|diff-cmc1984|diff-cie1994|diff-cie2000|diff-cie1994mix|diff-cie2000mix> <double> <gaussian|circular|inverted> <int>" << endl;
            exit(0);
        }
        else if(!strncmp(argv[p], "-quiet", 6))
        {
            quiet=true;
            p++;
        }
        // Check if this is the filename part -- step 1. The output configuration parameters.
        else if(!strncmp(argv[p], "-o", 2) || !strncmp(argv[p], "-O", 2))
        {
            // check if there is a file name
            if(argc>p+3)
            {
                op=argv[p+1];
                start=stoi(argv[p+2]);   //initial number of the output file
                finish=stoi(argv[p+3]);  //final number of the output file
                p+=4;
            }
            else
            {
                return errorMessage(p,"Failed to load output, missing argument");
            }
        }
        // Check if this is the filename part -- step 1. The globalmap path.
        else if(!strncmp(argv[p], "-e", 2) || !strncmp(argv[p], "-E", 2))
        {
            // check if there is a file name
            if(argc>p+1)
            {
                mp=argv[p+1];
                p+=2;
            }
            else
            {
                return errorMessage(p,"Failed to load map, missing argument");
            }
        }
        // Check the trajectory files.
        else if(!strncmp(argv[p], "-t", 2) || !strncmp(argv[p], "-T", 2))
        {
            // check if there is a file name
            if(argc>p+1)
            {
                tp=argv[p+1];
                p+=2;
            }
            else
            {
                return errorMessage(p,"Failed to load trajectory, missing argument");
            }
        }
        // Check the strategies files and parameters.
        else if(!strncmp(argv[p], "-s", 2) || !strncmp(argv[p], "-S", 2))
        {
            // initialize heuristic config
            heuristicType* ht = new heuristicType();

            // check if there is an appropriate strategy type
            if(argc>=p+3)
            {
                // store strategy
                std::string s(argv[p+1]);

                if(s.compare("SSD")==0 || s.compare("ssd")==0)
                    ht->strategy=SSD;
                else if(s.compare("DENSITY")==0 || s.compare("density")==0)
                    ht->strategy=DENSITY;
                else if(s.compare("SDENSITY")==0 || s.compare("sdensity")==0)
                    ht->strategy=SINGLE_COLOR_DENSITY;
                else if(s.compare("MEANSHIFT")==0 || s.compare("meanshift")==0)
                    ht->strategy=MEAN_SHIFT;
                else if(s.compare("SIFT")==0 || s.compare("sift")==0)
                    ht->strategy=SIFT_MCL;
                else if(s.compare("CREATE")==0 || s.compare("create")==0)
                    ht->strategy=CREATE_OBSERVATIONS;
                else if(s.compare("TEMPLATE")==0 || s.compare("template")==0)
                    ht->strategy=TEMPLATE_MATCHING;
                else if(s.compare("FEATURE")==0 || s.compare("feature")==0)
                    ht->strategy=FEATURE_MATCHING;
                else if(s.compare("COLOR")==0 || s.compare("color")==0)
                    ht->strategy=COLOR_ONLY;
                else if(s.compare("UNSCENTED")==0 || s.compare("unscented")==0)
                    ht->strategy=UNSCENTED_COLOR;
                else if(s.compare("ENTROPY")==0 || s.compare("entropy")==0)
                    ht->strategy=ENTROPY;
                else if(s.compare("MI")==0 || s.compare("mi")==0)
                    ht->strategy=MUTUAL_INFORMATION;
                else if(s.compare("BRIEF")==0 || s.compare("brief")==0)
                    ht->strategy=BRIEF;
                else
                    return errorMessage(p+1, "Invalid strategy: " + s);
            } else
                return errorMessage(p+1, "Insuficient strategy information:");

            // Validate and store color difference type
            std::string color_diff(argv[p+2]);
            if(color_diff.compare("diff-intensity")==0 || color_diff.compare("DIFF-INTENSITY")==0)
                ht->colorDifference = INTENSITYC;
            if (color_diff.compare("diff-rgb")==0 || color_diff.compare("DIFF-RGB")==0)
                ht->colorDifference = RGBNORMA;
            if(color_diff.compare("diff-cie1976")==0   || color_diff.compare("DIFF-CIE1976")==0)
                ht->colorDifference = CIELAB1976;
            if(color_diff.compare("diff-cmc1984")==0   || color_diff.compare("DIFF-CMC1984")==0)
                ht->colorDifference = CMCLAB1984;
            if(color_diff.compare("diff-cie1994")==0   || color_diff.compare("DIFF-CIE1994")==0)
                ht->colorDifference = CIELAB1994;
            if(color_diff.compare("diff-cie2000")==0   || color_diff.compare("DIFF-CIE2000")==0)
                ht->colorDifference = CIELAB2000;
            if(color_diff.compare("diff-cie1994mix")==0|| color_diff.compare("DIFF-CIE1994MIX")==0)
                ht->colorDifference = CIELAB1994MIX;
            if(color_diff.compare("diff-cie2000mix")==0|| color_diff.compare("DIFF-CIE2000MIX")==0)
                ht->colorDifference = CIELAB2000MIX;

            //Check for correct initialization
            if(ht->colorDifference == -1)
                return errorMessage(p+2, "Invalid color difference: " + color_diff);

            // Check if this is the threshold for color difference is valid
            try {
                ht->threshold = atof(argv[p+3]);
            }
            catch (const std::invalid_argument& ia) {
                cerr << "Invalid argument: " << ia.what() << '\n';
                return errorMessage(p+3, "Color threshold is a double, example: -t 2.3\n");
            }

            // Catching silly color threshold
            if(ht->threshold<0)
                return errorMessage(p+3, "Color limiar cannot be negative: " + to_string(ht->threshold));

            //increasing the string position
            p+=4;

            // Check if we must check for kernel data
            if(ht->strategy == DENSITY ||
               ht->strategy == SINGLE_COLOR_DENSITY ||
               ht->strategy == MEAN_SHIFT ||
               ht->strategy == SIFT_MCL ||
               ht->strategy == ENTROPY)
            {
                if(argc>=p+1) {

                    // store kernel type
                    std::string kernel(argv[p]);
                    if(kernel.compare("Gaussian")==0 || kernel.compare("gaussian")==0)
                        ht->kernelType=KGAUSSIAN;
                    else if(kernel.compare("Inverted")==0 || kernel.compare("inverted")==0)
                        ht->kernelType=KANTIELIP;
                    else if(kernel.compare("Circular")==0 || kernel.compare("circular")==0)
                        ht->kernelType=KCIRCULAR;
                    else
                        return errorMessage(p, "Invalid kernel type: " + kernel);

                    // check if there is an appropriate kernel radius
                    try {
                        ht->radius = atof(argv[p+1]);
                    }
                    catch (const std::invalid_argument& ia) {
                        cerr << "Invalid radius: " << ia.what() << endl;
                        return errorMessage(p+1, ", it must be an int.");
                    }
                    // Catching silly radius error
                    if(ht->radius<=0)
                        return errorMessage(p+1, "Invalid radius: " + to_string(ht->radius) + ",it must greater than 0.");

                    // Move to next argument
                    p+=2;


                }
                else // Density with missing arguments
                    return errorMessage(p, "Lacking arguments: density requires a kernel type and radius.");
            }

            if(ht->strategy==SINGLE_COLOR_DENSITY)
            {
                if(argc>=p) {
                    std::string color(argv[p]);
                    if(color.compare("WHITE")==0 || color.compare("white")==0)
                        ht->color=WHITE;
                    else if(color.compare("BLACK")==0 || color.compare("black")==0)
                        ht->color=BLACK;
                    else if(color.compare("RED")==0 || color.compare("red")==0)
                        ht->color=RED;
                    else if(color.compare("BLUE")==0 || color.compare("blue")==0)
                        ht->color=BLUE;
                    else if(color.compare("GREEN")==0 || color.compare("green")==0)
                        ht->color=GREEN;
                    else
                        return errorMessage(p, "Invalid color name: " + color);
                }
                else // Density with missing arguments
                    return errorMessage(p, "Lacking arguments: single_color_density requires a color value.");
            }
            // Store heuristic
            heuristicTypes.push_back(ht);
        }
        else if(!strncmp(argv[p], "-bp", 3) || !strncmp(argv[p], "-BP", 3))
        {
            heuristicType* ht = heuristicTypes[heuristicTypes.size()-1]; //The last heuristic added
            try {
                ht->numberPairs = atoi(argv[p+1]);
            }
            catch (const std::invalid_argument& ia) {
                cerr << "Invalid argument: " << ia.what() << '\n';
                return errorMessage(p+1, "Number of pairs is a integer, example: -bp 1000\n");
            }
            p++;
        }
        else if(!strncmp(argv[p], "-blt", 4) || !strncmp(argv[p], "-BLT", 4))
        {
            heuristicType* ht = heuristicTypes[heuristicTypes.size()-1];//The last heuristic added
            try {
                ht->lowThreshold = atof(argv[p+1]);
            }
            catch (const std::invalid_argument& ia) {
                cerr << "Invalid argument: " << ia.what() << '\n';
                return errorMessage(p+1, "Low threshold is a float between 0 and 0.99, example: -blt 0.55\n");
            }
            if (ht->lowThreshold >= 1 || ht->lowThreshold < 0){
                return errorMessage(p+1, "Low threshold is a float between 0 and 0.99, example: -blt 0.55\n");
            }
            p++;
        }
        else if(!strncmp(argv[p], "-bmt", 4) || !strncmp(argv[p], "-BMT", 4))
        {
            heuristicType* ht = heuristicTypes[heuristicTypes.size()-1];//The last heuristic added
            try {
                ht->multiplierThreshold = atof(argv[p+1]);
            }
            catch (const std::invalid_argument& ia) {
                cerr << "Invalid argument: " << ia.what() << '\n';
                return errorMessage(p+1, "Multiplier threshold is a float, example: -bmt 10.5\n");
            }
            p++;
        }
        else if(!strncmp(argv[p], "-bm", 3) || !strncmp(argv[p], "-BM", 3))
        {
            heuristicType* ht = heuristicTypes[heuristicTypes.size()-1];
            try {
                ht->margin = atoi(argv[p+1]);
            }
            catch (const std::invalid_argument& ia) {
                cerr << "Invalid argument: " << ia.what() << '\n';
                return errorMessage(p+1, "Margin is a integer, example: -bmt 10\n");
            }
            p++;
        }
        else if(!strncmp(argv[p], "-unp", 4) || !strncmp(argv[p], "-UNP", 4))
        {
            heuristicType* ht = heuristicTypes[heuristicTypes.size()-1];//The last heuristic added
            try {
                ht->unscentedNumberPoints  = atoi(argv[p+1]);
            }
            catch (const std::invalid_argument& ia) {
                cerr << "Invalid argument: " << ia.what() << '\n';
                return errorMessage(p+1, "Number of Points is a integer, example: -np 100\n");
            }
            p++;
        }
        else if(!strncmp(argv[p], "-par", 4) || !strncmp(argv[p], "-PAR", 4))
        {
            try
            {
                numParticles =  atoi(argv[p+1]);
            }
            catch (const std::invalid_argument& ia)
            {
                cerr << "Invalid argument: " << ia.what() << '\n';
                return errorMessage(p+1, "Number of particles is a integer, example: -par 50000\n");
            }
            p++;
        }
        /*else if(!strncmp(argv[p], "-mndvw", 6) || !strncmp(argv[p], "-MNDVW", 6))
        {
            try
            {
                maxNdviWeight =  atof(argv[p+1]);
            }
            catch (const std::invalid_argument& ia)
            {
                cerr << "Invalid argument: " << ia.what() << '\n';
                return errorMessage(p+1, "Max NDVI Weight is a float, example: -mndvw 2.3\n");
            }
            p++;
        }*/
        /*else if(!strncmp(argv[p], "-mindviw", 8) || !strncmp(argv[p], "-MINDVIW", 8))
        {
            try
            {
                minNdviWeight =  atof(argv[p+1]);
            }
            catch (const std::invalid_argument& ia)
            {
                cerr << "Invalid argument: " << ia.what() << '\n';
                return errorMessage(p+1, "Min NDVI Weight is a float, example: -mindviw 1.2\n");
            }
            p++;
        }*/
        else if(!strncmp(argv[p], "-msc", 4) || !strncmp(argv[p], "-MMSC", 4))
        {
            try
            {
                multispectralCam =  atoi(argv[p+1]);
            }
            catch (const std::invalid_argument& ia)
            {
                cerr << "Invalid argument: " << ia.what() << '\n';
                return errorMessage(p+1, "The use of a multispectal camera is a boolean, example: -msc 1\n");
            }
            p++;
        }
        else if(!strncmp(argv[p], "-ndvit", 6) || !strncmp(argv[p], "-NDVIT", 6))
        {
            try
            {
                ndvi_type =  atoi(argv[p+1]);
            }
            catch (const std::invalid_argument& ia)
            {
                cerr << "Invalid argument: " << ia.what() << '\n';
                return errorMessage(p+1, "NDVI Type is a integer, example: -ndvit 1\n");
            }
            p++;
        }
        else if(!strncmp(argv[p], "-grvit", 6) || !strncmp(argv[p], "-GRVIT", 6))
        {
            try
            {
                grvi_type =  atoi(argv[p+1]);
            }
            catch (const std::invalid_argument& ia)
            {
                cerr << "Invalid argument: " << ia.what() << '\n';
                return errorMessage(p+1, "GRVI Type is a integer, example: -grvit 1\n");
            }
            p++;
        }
        else if(!strncmp(argv[p], "-minsc", 6) || !strncmp(argv[p], "-MINSC", 6))
        {
            try
            {
                minScale =  atof(argv[p+1]);
            }
            catch (const std::invalid_argument& ia)
            {
                cerr << "Invalid argument: " << ia.what() << '\n';
                return errorMessage(p+1, "Min Scale  Type is a double, example: -minsc 1.9\n");
            }
            p++;
        }
        else if(!strncmp(argv[p], "-maxsc", 6) || !strncmp(argv[p], "-MAXSC", 6))
        {
            try
            {
                maxScale =  atof(argv[p+1]);
            }
            catch (const std::invalid_argument& ia)
            {
                cerr << "Invalid argument: " << ia.what() << '\n';
                return errorMessage(p+1, "Max Scale  Type is a double, example: -maxsc 1.9\n");
            }
            p++;
        }
        else if(!strncmp(argv[p], "-msimp", 6) || !strncmp(argv[p], "-MSIMP", 6))
        {
            try
            {
                msImgPreProc =  atoi(argv[p+1]);
            }
            catch (const std::invalid_argument& ia)
            {
                cerr << "Invalid argument: " << ia.what() << '\n';
                return errorMessage(p+1, "Multispectral image pre-processing is a int:\n "
                                         "0: without pre-processing\n "
                                         "1: gaussian filter\n "
                                         "2: bilateral filtering.\n");
            }
            p++;
        }
        else if(!strncmp(argv[p], "-gbs", 6) || !strncmp(argv[p], "-GBS", 6))
        {
            try
            {
                gbSigma =  atof(argv[p+1]);
            }
            catch (const std::invalid_argument& ia)
            {
                cerr << "Invalid argument: " << ia.what() << '\n';
                return errorMessage(p+1, "Gaussian blur sigma.\n");
            }
            p++;
        }
        else if(!strncmp(argv[p], "-gbk", 6) || !strncmp(argv[p], "-GBK", 6))
        {
            try
            {
                gbKernel =  atof(argv[p+1]);
            }
            catch (const std::invalid_argument& ia)
            {
                cerr << "Invalid argument: " << ia.what() << '\n';
                return errorMessage(p+1, "Gaussian blur kernel size.\n");
            }
            p++;
        }
        else if(!strncmp(argv[p], "-bfd", 6) || !strncmp(argv[p], "-BFD", 6))
        {
            try
            {
                bfD =  atoi(argv[p+1]);
            }
            catch (const std::invalid_argument& ia)
            {
                cerr << "Invalid argument: " << ia.what() << '\n';
                return errorMessage(p+1, "Bilateral Filter D\n");
            }
            p++;
        }
        else if(!strncmp(argv[p], "-bfsc", 6) || !strncmp(argv[p], "-BFSC", 6))
        {
            try
            {
                bfSigmaColor =  atof(argv[p+1]);
            }
            catch (const std::invalid_argument& ia)
            {
                cerr << "Invalid argument: " << ia.what() << '\n';
                return errorMessage(p+1, "Bilateral Filter sigma color\n");
            }
            p++;
        }
        else if(!strncmp(argv[p], "-bfss", 6) || !strncmp(argv[p], "-BFSS", 6))
        {
            try
            {
                bfSigmaSpace =  atof(argv[p+1]);
            }
            catch (const std::invalid_argument& ia)
            {
                cerr << "Invalid argument: " << ia.what() << '\n';
                return errorMessage(p+1, "Bilateral Filter sigma space\n");
            }
            p++;
        }
        else if(!strncmp(argv[p], "-mappp", 6) || !strncmp(argv[p], "-MAPPP", 6))
        {
            try
            {
                mapPreProc =  atoi(argv[p+1]);
            }
            catch (const std::invalid_argument& ia)
            {
                cerr << "Invalid argument: " << ia.what() << '\n';
                return errorMessage(p+1, "Local and global map pre-processing is a int:\n "
                                         "1: quantization\n "
                                         "2: bilateral filtering.\n");
            }
            p++;
        }
        else if(!strncmp(argv[p], "-pdm", 4) || !strncmp(argv[p], "-PDM", 4))
        {
            try
            {
                pixelDistMethod =  atoi(argv[p+1]);
            }
            catch (const std::invalid_argument& ia)
            {
                cerr << "Invalid argument: " << ia.what() << '\n';
                return errorMessage(p+1, "Pixel Distribution Method is a int:\n "
                                         "1:  Gaussian\n"
                                         "11: Old Gaussian\n"
                                         "2:  Roulette-wheel selection via stochastic acceptance.\n "
                                         "3:  Random Pool Selection (Rejection sampling).\n"
                                         "31: Random Pool Selection (Rejection sampling) + Gaussian.\n"
                                         "4:  Improved Random Pool selection (Rejection sampling)\n"
                                         "5:  Roulette-wheel/n"
                                         "51: Roulette-wheel + Gaussian/n"
                                         "6:  Tournament/n"
                                         "61: Tournament + Gaussian/n");
            }
            p++;
        }
        else if(!strncmp(argv[p], "-fit", 4) || !strncmp(argv[p], "-FIT", 4))
        {
            try
            {
                poolSelFit = atoi(argv[p+1]);
            }
            catch (const std::invalid_argument& ia)
            {
                cerr << "Invalid argument: " << ia.what() << '\n';
                return errorMessage(p+1, "Pool Selection Fitness is a int:\n "
                                         "0: funcionamento defaut\n "
                                         "1: desconsiderar toda a vegetação\n "
                                         "2: desconsiderar a vegetação comum, trabalhando apenas com a mais densa.\n "
                                         "3: Aceitar todos os pixeis que não são vegetação");
            }
            p++;
        }        
        else if(!strncmp(argv[p], "-inids", 6) || !strncmp(argv[p], "-INIDS", 6))
        {
            try
            {
                imgIniDs = atoi(argv[p+1]);
            }
            catch (const std::invalid_argument& ia)
            {
                cerr << "Invalid argument: " << ia.what() << '\n';
                return errorMessage(p+1, "Imagem inicial do dataset.");
            }
            p++;
        }
        else if(!strncmp(argv[p], "-iniexec", 8) || !strncmp(argv[p], "-INIEXEC", 8))
        {
            try
            {
                imgIniExec = atoi(argv[p+1]);
            }
            catch (const std::invalid_argument& ia)
            {
                cerr << "Invalid argument: " << ia.what() << '\n';
                return errorMessage(p+1, "Imagem inicial da execução.");
            }
            p++;
        }
        else if(!strncmp(argv[p], "-fimds", 6) || !strncmp(argv[p], "-FIMDS", 6))
        {
            try
            {
                imgFimDs = atoi(argv[p+1]);
            }
            catch (const std::invalid_argument& ia)
            {
                cerr << "Invalid argument: " << ia.what() << '\n';
                return errorMessage(p+1, "Imagem final do dataset.");
            }
            p++;
        }
        else if(!strncmp(argv[p], "-fiexec", 7) || !strncmp(argv[p], "-FIEXEC", 7))
        {
            try
            {
                imgFimExec = atoi(argv[p+1]);
            }
            catch (const std::invalid_argument& ia)
            {
                cerr << "Invalid argument: " << ia.what() << '\n';
                return errorMessage(p+1, "Imagem final da execução.");
            }
            p++;
        }
        else if(!strncmp(argv[p], "-multgaussmsk", 13) || !strncmp(argv[p], "-MULTGAUSSMSK", 13))
        {
            try
            {
                multiGaussianMask = atof(argv[p+1]);
            }
            catch (const std::invalid_argument& ia)
            {
                cerr << "Invalid argument: " << ia.what() << '\n';
                return errorMessage(p+1, "Multiplicador da máscara gaussiana utilizada em conjunto com a roleta pelo VI. Ex: com 0.5, a máscara gaussiana só exercerá 50% de influência...");
            }
            p++;
        }
        else if(!strncmp(argv[p], "-alfavismsk", 11) || !strncmp(argv[p], "-ALFAVIMSK", 11))
        {
            try
            {
                alphaVegetationMask = atof(argv[p+1]);
            }
            catch (const std::invalid_argument& ia)
            {
                cerr << "Invalid argument: " << ia.what() << '\n';
                return errorMessage(p+1, "Alfa da máscara de vegetação utilizada em conjunto com a roleta pelo VI. Ex: com 0.5, a máscara gaussiana só exercerá 50% de influência...");
            }
            p++;
        }
        else if(!strncmp(argv[p], "-betaweight", 11) || !strncmp(argv[p], "-BETAWEIGHT", 11))
        {
            try
            {
                betaHammingWeight = atof(argv[p+1]);
            }
            catch (const std::invalid_argument& ia)
            {
                cerr << "Invalid argument: " << ia.what() << '\n';
                return errorMessage(p+1, "Utilizado no cálculo do peso dos pares de píxeis, baseado na máscara de vegetação NDVI.");
            }
            p++;
        }
        else if(!strncmp(argv[p], "-cmppar", 7) || !strncmp(argv[p], "-CMPPAR", 7))
        {
            try
            {
                compensateParticles = atoi(argv[p+1]);
            }
            catch (const std::invalid_argument& ia)
            {
                cerr << "Invalid argument: " << ia.what() << '\n';
                return errorMessage(p+1, "Compensar as partículas com base na orientação do drone. 1 ativa e 0 ou não mencionar, desativa");
            }
            p++;
        }
        else if(!strncmp(argv[p], "-corrpxpos", 10) || !strncmp(argv[p], "-CORRPXPOS", 10))
        {
            try
            {
                corrPixelPosAngles =  atoi(argv[p+1]);
            }
            catch (const std::invalid_argument& ia)
            {
                cerr << "Invalid argument: " << ia.what() << '\n';
                return errorMessage(p+1, "The correction on the pixel positions based on the drone angles is a boolean, example: -corrpxpos 1\n");
            }
            p++;
        }
        else if(!strncmp(argv[p], "-rpang", 6) || !strncmp(argv[p], "-RPANG", 6))
        {
            try
            {
                rollPitchAngRange =  atoi(argv[p+1]);
            }
            catch (const std::invalid_argument& ia)
            {
                cerr << "Invalid argument: " << ia.what() << '\n';
                return errorMessage(p+1, "Troll and pitch ranges is a integer, example: -rpang 5\n");
            }
            p++;
        }
        else
            p++;
    }
    //errorMessage(0, "Input error.");
    return true;
}

// -e /home/mathias/Documents/Datasets/ufrgs_google_longterm/adjusted/median_new -t /home/mathias/Documents/Datasets/ufrgs_dronao_28ago/trajectories/gt_traj4.txt -s BRIEF diff-rgb 15 -bp 300 -blt 0.5 -bmt 4 -bm 10 -s unscented diff-rgb -unp 10
int main(int argc, char* argv[])
{
//    vector<valueCV> points;
//    for ( unsigned i = 0 ; i < 10 ; ++i )
//    {
//        points.push_back(valueCV(cv::Point2i(i,i),i));
//    }

//    Rtree rt(points);
//    rt.findPointsInsideCircle(cv::Point2i(5,5),2);
//    exit(0);

    // Global variables


    connectionMode = SIMULATION;    //Robot.h
    logMode = NONE;                 //Utils.h
    filename = "";

    // load config from command line
    std::string mapPath, trajPath, outputPath;
    int start=-1;
    int finish=-1;

    vector< heuristicType* > heuristicTypes;
    if(!config(argc, argv, heuristicTypes, mapPath, trajPath, outputPath, start, finish))
        exit(1);

    Robot* r;
    // r = new Robot();
    // r = new PioneerRobot();
    std::cout<<"CONFIGURATED"<<std::endl;
    r = new DroneRobot(mapPath,trajPath,heuristicTypes,quiet,outputPath,start,finish);

    if(quiet){
        startRobotThread((void*)r);        
    }else{
        pthread_t robotThread, glutThread;
        mutex = new pthread_mutex_t;
        pthread_mutex_unlock(mutex);

        pthread_create(&(robotThread),NULL,startRobotThread,(void*)r);
        pthread_create(&(glutThread),NULL,startGlutThread,(void*)r);

        pthread_join(robotThread, 0);
        pthread_join(glutThread, 0);
    }

    return 0;
}
