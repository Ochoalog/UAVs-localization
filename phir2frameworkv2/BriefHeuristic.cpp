#include "BriefHeuristic.h"
#include <time.h>
#include "config.h"
#include <math.h>
#include <random>
#include <iostream>
#include <fstream>
#include <vector>

BriefHeuristic::BriefHeuristic(STRATEGY s, int id, int cd, double l):Heuristic(s,id,l,cd)
{
    this->totalPairs = 1000;
    this->lowThreshold = 0.5;
    this->multiplierThreshold = 4;
    this->margin = 10;
    this->width = 0;
    this->height = 0;
    this->sumNdviWeightsDrone = 0;

    /*
     * 0 - gray                     = 1 bit
     * 1 - Lab                      = 3 bits
     * 2 - Lab                      = 6 bits
     * 3 - Integer Difference       = Other :p
     * 4 - Lab wo L                 = 2 bits    //LAB without L
    */
    //type = 0; //ESTAMOS USANDO O LOWTHRESHOLD APENAS PRA NAO TER QUE MUDAR O CODIGO. ESSE "TYPE" AQUI
                         //REPRESENTA A QUANTIDADE DE BITS DA IMAGEM QUE ESTÁ SENDO USADA.

    //type = 4=>>> abBRIEF com a config utilizada no trabalho do Mathias.
    //type = 0 =>>> BRIEF default, em grayscale...

    type = BRIEF_HEURISTIC_TYPE;

cout<<"\ntype: "<< type;
}

void BriefHeuristic::printInfo(){
    cout << endl << "Brief parameters:\nPairs:\t\t" << totalPairs << "\nLow Threshold:\t" << lowThreshold << "\nMultiplier:\t" << multiplierThreshold << "\nMargin:\t\t" << margin << endl << endl;
}

//DRONE
void BriefHeuristic::updateDroneDescriptor(cv::Mat& drone, cv::Mat &redMap, cv::Mat &nirMap, cv::Mat &regMap, cv::Mat &greMap, cv::Mat &currentMapOriginal, Pose3d orientacao, Pose3d realPose)
{
    this->width = drone.cols;
    this->height = drone.rows;
    this->totPx = width * height;
    cv::Mat viMatrix;

    if(VIDouble)
        viMatrix = cv::Mat(drone.rows, drone.cols, CV_64F, 0.0);//VI double
    else
        viMatrix = cv::Mat(drone.rows, drone.cols, CV_32F, 0.0);//VI float


    if(pairs.size() < 1){
        for(int p=0; p<totalPairs; p++)
        {
            vector<cv::Point> pair(2);
            pairs.push_back(pair);

            vector<float> nrpair(2);
            pairVI.push_back(nrpair);

            float pw(1);
            pairWeight.push_back(pw);
        }
    }

    pixelDistribution(drone, nirMap, redMap, regMap, greMap, viMatrix);

    if(!quiet)
        drawPixelDistribution(currentMapOriginal);    

    vector<int> bin;    
    for(int c=0;c<pairs.size();c++)
    {
        vec3 color1(getValuefromPixel(pairs[c][0].x,pairs[c][0].y,&drone));
        vec3 color2(getValuefromPixel(pairs[c][1].x,pairs[c][1].y,&drone));

        //LAB WITH OUT L
        if(color1.r>color2.r) bin.push_back(1);
        else bin.push_back(0);
        if(color1.g>color2.g) bin.push_back(1);
        else bin.push_back(0);
    }

    droneDescriptor = bin;
    this->CalculateVIMask(redMap, nirMap, regMap, greMap, viMatrix);

    //if(!quiet)
    //{
    //    visualizaCameraPoseOriCorr(realPose, orientacao, false);
    //}
}

//http://answers.opencv.org/question/86801/normalize-image-0-255-for-display/
//https://www.youtube.com/watch?v=mr2LcT4jhCw
//Two-dimensional Gaussian function - https://en.wikipedia.org/wiki/Gaussian_function
void BriefHeuristic::gaussianMask(cv::Size &size, cv::Mat &output, int x0, int y0, float sigmaX, float sigmaY, float amplitude)
{
    cv::Mat temp = cv::Mat(size, CV_32F);
    float X = 0, Y = 0, value = 0;

    for (int y = 0; y < size.height; y++)
    {
        for (int x = 0; x < size.width; x++)
        {
            X = ((x - x0) * (x - x0)) / (2.0f * sigmaX * sigmaY);
            Y = ((y - y0) * (y - y0)) / (2.0f * sigmaX * sigmaY);
            value = amplitude * exp( -(X + Y));

            temp.at<float>(y, x) = value;
            X = 0; Y = 0; value = 0;
        }
    }
    cv::normalize(temp, temp, 0.0f, 1.0f, cv::NORM_MINMAX);
    output = temp;
}

void BriefHeuristic::gaussianMaskD(cv::Size &size, cv::Mat &output, int x0, int y0, double sigmaX, double sigmaY, double amplitude)
{
    cv::Mat temp = cv::Mat(size, CV_64F);
    double X = 0, Y = 0, value = 0;

    for (int y = 0; y < size.height; y++)
    {
        for (int x = 0; x < size.width; x++)
        {
            X = ((x - x0) * (x - x0)) / (2.0f * sigmaX * sigmaY);
            Y = ((y - y0) * (y - y0)) / (2.0f * sigmaX * sigmaY);
            value = amplitude * exp( -(X + Y));

            temp.at<double>(y, x) = value;
            X = 0; Y = 0; value = 0;
        }
    }
    cv::normalize(temp, temp, 0.0f, 1.0f, cv::NORM_MINMAX);
    output = temp;
}

//USAR O MÉTODO DA BISSEXÃO PARA MELHORAR O DESEMPENHO DA ROLETA. ADICIONAR VARIÁVEIS COMS OS LIMITES DE DOIS ELEMENTOS, A CADA PIXEL E SELECIONAR POR ELES.
void BriefHeuristic::pixelDistribution(cv::Mat& drone, cv::Mat &nirMap,cv::Mat &redMap, cv::Mat &regMap, cv::Mat &greMap, cv::Mat viMatrix)
{
    VegetationIndex *vegIdx = VegetatonIndexGen::InitVegetationIndex();
    int x = 0, y = 0, pixelPair = 0;
    cv::Point point;    
    double sigmaX = 0;
    double sigmaY = 0;
    clock_t tempo;
    tempo = clock();


    if(pixelDistMethod == 11)
    {
        //Original => Paper do BRIEF
        sigmaX = 1.0/25.0*pow(drone.cols,2);
        sigmaY = 1.0/25.0*pow(drone.rows,2);
        //Teste
        //sigmaX = (1.0/25.0)*pow(drone.cols/10,2);
        //sigmaY = (1.0/25.0)*pow(drone.rows/10,2);
    }
    else
    {
        sigmaX = drone.cols/5;
        sigmaY = drone.rows/5;
    }

    cv::RNG rng(time(NULL) * cv::getTickCount());
    cv::Mat gaussianMsk;
    cv::Size sz = cv::Size(drone.cols, drone.rows);
    //Gaussian Mask
    if(pixelDistMethod == 51 || pixelDistMethod == 7 || pixelDistMethod == 52 || pixelDistMethod == 2)
    {
        if(VIDouble)
        {
            gaussianMaskD(sz, gaussianMsk, drone.cols/2, drone.rows/2, sigmaX, sigmaY);//(double)Valores entre 0 e 1
            if(pixelDistMethod == 51)
                pixelDistMethod = 52;
        }
        else
        {
            gaussianMask(sz, gaussianMsk, drone.cols/2, drone.rows/2, sigmaX, sigmaY);//(float)Valores entre 0 e 1
        }
        //imshow("Gaussian", gaussianMsk);
        //imwrite("/home/phi/Documents/Images/output.jpg", gaussianMsk);
    }

   /* int rows = nirMap.rows;
    int cols = nirMap.cols;
    float fakeViMat[rows][cols];

    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            //Desenha uma máscara falsa
            if(j > cols/2-50 && j < cols/2+50)
                fakeViMat[i][j] = Utils::RandomFloat(-1, -0.9);
                //fakeViMat[i][j] = Utils::RandomFloat(-1, 0);
            else if(i > rows/2-50 && i < rows/2+50)
                fakeViMat[i][j] = Utils::RandomFloat(-1, -0.9);
                //fakeViMat[i][j] = Utils::RandomFloat(-1, 0);
            else
                fakeViMat[i][j] = Utils::RandomFloat(0.9, 1);
                //fakeViMat[i][j] = Utils::RandomFloat(0, 1);
            //==
        }
    }*/

    //cv::Mat fakePixelDistMask(rows, cols, CV_32FC1, fakeViMat);
    //imshow("pixelDistMask", fakePixelDistMask);

    //cvtColor(pixelDistMask, pixelDistMask, cv::COLOR_BGR2GRAY);
    //fakePixelDistMask.convertTo(fakePixelDistMask, CV_8UC3, 255.0);
    //imwrite("/home/phi/Documents/Images/fakePixelDistMask.jpg", fakePixelDistMask);


    switch(pixelDistMethod)
    {
    //Fitness proportionate selection
        case(2)://(OK) Roulette-wheel selection via stochastic acceptance (https://www.sciencedirect.com/science/article/pii/S0378437111009010)
        {
            int rows = nirMap.rows;
            int cols = nirMap.cols;
            std::vector< std::vector<double> > viMat(rows, std::vector<double>(cols));
            //float viMat[rows][cols];
            double sumScaled = 0;
            double random = 0;
            double maximumFitness = FLOAT_MIN;
            int totPxVeg = 0;
            cv::Mat gray(rows, cols, CV_8UC1);
            cv::Mat grayEqualized(rows, cols, CV_8UC1);
            cv::Mat grayEqualizedVis(rows, cols, CV_8UC1);
            double tempoTotalCalcVI = 0;
            clock_t tempoCalcVI;

            /* initialize random seed: */
            srand (time(NULL) * cv::getTickCount());
            tempoCalcVI = clock();
            int ndviCount = 0, ndviCount2 = 0;

            //Cálculo do vegetation index para toda a imagem
            #pragma omp parallel for num_threads(NUM_THREADS) shared(totPxVeg)
            for (int i = 0; i < rows; i++)//COMO CALCULO O VI AQUI, PASSAREI ESSE VALOR CALCULADO, E NÃO O RECALCULAREI NO PROCESSAMENTO DO DESCRITOR
            {
                for (int j = 0; j < cols; j++)
                {
                   viMat[i][j] = vegIdx->setIndexD(nirMap.at<uchar>(i, j),
                           regMap.at<uchar>(i, j),
                           redMap.at<uchar>(i, j),
                           greMap.at<uchar>(i, j));
                   if(viMat[i][j] >= 0.3)//Grandes chances de conter vegetação
                      #pragma omp atomic update
                      totPxVeg++;
                   viMatrix.at<float>(i,j) =  viMat[i][j];
                   viMat[i][j] = viMat[i][j] * -1;//Inverter a probabilidade, pois no default valores grandes indicam vegetação, mas quero fazer com que os valores grandes sejam referentes à probabilidade de não haver vegetação.

                   //Escala para um range positivo, já convertendo para o que seria esperado de uma imagem 0 - 255
                   gray.at<uchar>(i,j) = Utils::Normalize(viMat[i][j], 0, 255, -1, 1);

                   /*if(viMat[i][j] >= 0.6)
                       ndviCount++;
                   else if(viMat[i][j] < 0.3)
                       ndviCount2 ++;*/
                }
            }

            //cout<< "Maiores: " << ndviCount << "\tMenores " << ndviCount2 << endl;

            //Tempo de execução do Vegetation Index
            tempoTotalCalcVI = (clock() - tempoCalcVI) / (double)CLOCKS_PER_SEC;
            cout << "Tempo do calculo do VI: " << tempoTotalCalcVI << " s \n";
            //Cálculo do percentual de vegetação na imagem
            percentualPxVegImg = (totPxVeg/(totPx/100))/100;
            cout << "\nPercentual de vegetação: " << percentualPxVegImg << "\n";
            //Equalização da imagem para realçar as diferenças
            cv::equalizeHist(gray, grayEqualized);
            if(!quiet)
            {
                cv::namedWindow("grayWin");
                //cv::moveWindow("grayWin", 20, 55 + (0.25 * gray.rows));
                cv::resize(gray, gray, cv::Size(), 0.25, 0.25);
                cv::imshow("grayWin", gray);

                cv::namedWindow("grayEqualizedWin");
                //cv::moveWindow("grayEqualizedWin", 20, 85 + (0.5 * grayEqualizedVis.rows));
                cv::resize(grayEqualized, grayEqualizedVis, cv::Size(), 0.25, 0.25);
                imshow("grayEqualizedWin",grayEqualizedVis);
            }
            //Escalar a matriz de VIs e já aplicar o cálculo do alfa
            for (int i = 0; i < rows; i++)
            {
                for (int j = 0; j < cols; j++)
                {   //Escala de 0 a 255 para de 0 a 1.
                    viMat[i][j] = Utils::Normalize(grayEqualized.at<uchar>(i,j), 0, 1, 0, 255);
                    viMat[i][j] = (alphaVegetationMask * viMat[i][j]) + (1-alphaVegetationMask) * gaussianMsk.at<float>(i, j);
                    sumScaled += viMat[i][j];
                }
            }
            //Normalizar a matriz de VIs de modo que a soma totalize 1
            for (int i = 0; i < nirMap.rows; i++)
            {
                for (int j = 0; j < nirMap.cols; j++)
                {
                    viMat[i][j] = viMat[i][j]/sumScaled;

                    if(viMat[i][j] > maximumFitness)
                        maximumFitness = viMat[i][j];
                }
            }            
            //Cálculo da roleta  (PENSAR EM FORMA DE ZERAR O NDVI DO PIXEL JÁ SELECIONADO, PARA EVIAR Q SEJA SELECIOANDO NOVAMENTE, MAS ISSO NÃO PODE IMPACTAR NO CÁLCULO DO BETA, e tb decrementar esse valor do max)
            for(int pair = 0; pair < totalPairs; pair++)//Itera por todos os pares de pixeis
            {
                pixelPair = 0;
                while(pixelPair < 2)//Itera pelos pixeis do par
                {
                    //Select a pixel
                    x = floor(rand() % drone.cols);
                    y = floor(rand() % drone.rows);

                    random = ((double) rand() / (RAND_MAX));
                    //random = Utils::RandomFloat(0, maximumFitness); esse método não está legal

                    if(random < (viMat[y][x]/maximumFitness))
                    {
                        pairs[pair][pixelPair] = cv::Point(x, y);
                        pixelPair ++;                      
                    }
                }
            }
            break;
        }
        case(3)://(OK) Random Pool selection (Rejection sampling) - https://en.wikipedia.org/wiki/Rejection_sampling
        {//IMPLEMENTAR TESTE PARA EVITAR QUE O MESMO PIXEL SEJA SELECIONADO NOVAMENTE  
            float fitnessScore = 0;
            float secStep = 0;

            for(int i = 0; i < totalPairs; i++)//Itera por todos os pares de pixeis
            {
                pixelPair = 0;
                while(pixelPair < 2)//Itera pelos pixels do par
                {
                    //1st Step: select a pixel
                    if(pixelDistMethod == 31)
                    {
                        /* initialize random seed: */
                        cv::theRNG().state = time(NULL) * cv::getTickCount();

                        //Gaussian selection
                        x = (int)rng.gaussian(sigmaX)+drone.cols/2;
                        y = (int)rng.gaussian(sigmaY)+drone.rows/2;
                    }
                    else
                    {
                        /* initialize random seed: */
                        srand (time(NULL) * cv::getTickCount());

                        x = floor(rand() % drone.cols);
                        y = floor(rand() % drone.rows);
                    }

                    point = cv::Point(x, y);
                    if(pointIn(pairs[i][pixelPair], drone))//point - erro
                    {
                        //2st Step: accept/reject based on its  fitness (NDVI value - lower better)
                        secStep = Utils::RandomFloat(-1, 1);//Random number between -1 and 1, the NDVI range


                        fitnessScore = vegIdx->setIndex(nirMap.at<uchar>(y, x),
                                                        regMap.at<uchar>(y, x),
                                                        redMap.at<uchar>(y, x),
                                                        greMap.at<uchar>(y, x));

                       // fitnessScore = fakeViMat[y][x];//Máscara em cruz

                        //Se "fitnessScore(NDVI)" der -0.9, há grandes chances de não ser vegetação, então se "secStep" for maior, eu aceito.
                        //A condição está invertida, em relação a implementação default.

                        if(poolSelFit == 1)
                        {
                            if(fitnessScore > 0.3)
                                continue;
                        }
                        else if(poolSelFit == 2)
                        {
                            if(fitnessScore > 0.3 && fitnessScore < 0.65)//Teste, desconsidera grande parte da vegetação
                                continue;
                        }
                        else if(poolSelFit == 3)
                        {
                            if(fitnessScore < 0.3)//Teste, aceitar todos os pixels que não são vegetação
                                secStep = 1;
                        }
                        else if(poolSelFit == 4)
                        {
                            if(fitnessScore < 0.3)//Apenas vegetação
                                continue;
                        }
                        else if(poolSelFit == 5)
                        {
                            if(fitnessScore < 0.65)//Apenas vegetação saudável/densa
                                continue;
                        }
                        else if(poolSelFit == 6)
                        {
                            if(fitnessScore > 0.3 && fitnessScore < 0.65)//Teste, desconsidera grande parte da vegetação
                                continue;
                            else if(fitnessScore >= 0.1 && fitnessScore <= 0.2)//desconsiderar o solo
                                continue;
                        }
                        else if(poolSelFit == 7)
                        {
                            if(fitnessScore > 0.2 || fitnessScore < 0.1)//Apenas solo
                                continue;
                        }
                        if(secStep > fitnessScore)
                        {
                            pairs[i][pixelPair] = cv::Point(x, y);
                            pixelPair ++;
                        }
                    }
                }
            }

            //Print da lista, debug
            /*for(int i = 0; i < totalPairs; i++)
            {
                cout <<"Pair(" << i << ") " "0: X " << pairs[i][0].x << " Y " <<  pairs[i][0].y << " | " <<  "1: X " << pairs[i][1].x << " Y " <<  pairs[i][1].y << "\n";
            }*/
            break;
        }
        case(4):// Improved Random Pool selection (Rejection sampling) - NÃO ESTÁ OK
        {
            float viMat[nirMap.rows][nirMap.cols];
            float sumScaled = 0;
            float sumViValues = 0, sumNormalized = 0;

            //Cálculo do vegetation index para toda a imagem
            for (int x = 0; x < nirMap.rows; x++)
            {
                for (int y = 0; y < nirMap.cols; y++)
                {
                  /* viMat[x][y] = vegIdx->setIndex(nirMap.at<uchar>(y, x),
                           regMap.at<uchar>(y, x),
                           redMap.at<uchar>(y, x),
                           greMap.at<uchar>(y, x));
                   if(!isnan(viMat[x][y]))
                        sumViValues += viMat[x][y];*/

                   //Desenha uma máscara falsa
                   if(y > nirMap.cols/2-50 && y < nirMap.cols/2+50)
                       viMat[x][y] = Utils::RandomFloat(-1, -0.9);
                       //viMat[x][y] = Utils::RandomFloat(-1, 0);
                   else if(x > nirMap.rows/2-50 && x < nirMap.rows/2+50)
                       viMat[x][y] = Utils::RandomFloat(-1, -0.9);
                       //viMat[x][y] = Utils::RandomFloat(-1, 0);
                   else
                       viMat[x][y] = Utils::RandomFloat(0.9, 1);
                       //viMat[x][y] = Utils::RandomFloat(0, 1);
                   //==
                }
            }

           /* for (int x = 0; x < nirMap.rows; x++)
                for (int y = 0; y < nirMap.cols; y++)
                   cout << "viMat: " << viMat[x][y] << "\n";

            cout << "sumViValues: " << sumViValues << "\n";*/

            //Escaalr a matriz de VIs para 0 até 1
           /* for (int x = 0; x < nirMap.rows; x++)
                for (int y = 0; y < nirMap.cols; y++)
                {
                    if(!isnan(viMat[x][y]))
                    {
                        viMat[x][y] = Utils::Normalize(viMat[x][y], 0, 1, -1, 1);
                        if(!isnan(viMat[x][y]))
                            sumScaled += viMat[x][y];
                    }
                }
            cout << "sumScaled: " << sumScaled << "\n";*/

            //Normalizar a matriz de VIs de modo que a soma totalize 1
            for (int x = 0; x < nirMap.rows; x++)
                for (int y = 0; y < nirMap.cols; y++)
                {
                    if(!isnan(viMat[x][y]))
                    {                      
                        viMat[x][y] = viMat[x][y]/sumViValues;
                        sumNormalized += viMat[x][y];
                    }
                }
            cout << "sumNormalized: " << sumNormalized << "\n";


            bool proxPixel = false;
            double randomRealBetweenZeroAndOne = 0;

            for(int i = 0; i < totalPairs; i++)//Itera por todos os pares de pixeis
            {
                pixelPair = 0;
                while(pixelPair < 2)//Itera pelos pixels do par
                {
                    randomRealBetweenZeroAndOne = Utils::RandomFloat(0, 1);;

                    for (int x = 0; x < nirMap.rows; x++)
                    {
                        for (int y = 0; y < nirMap.cols; y++)
                        {
                            if(randomRealBetweenZeroAndOne > viMat[x][y])
                                randomRealBetweenZeroAndOne -= viMat[x][y];
                            else
                            {
                                pairs[i][pixelPair] = cv::Point(x, y);
                                pixelPair ++;
                                proxPixel = true;
                                break;
                            }
                        }
                        if(proxPixel)
                            break;
                    }
                }
            }

            break;
        }        
        case(5)://(OK) Roulette-wheel
        case(51)://(OK) Roulette-wheel + Gaussian (float)
              {
                  int rows = nirMap.rows;
                  int cols = nirMap.cols;
                  float viMat[rows][cols];
                  float sumNormalized = 0, sumScaled = 0;
                  float limite = 0, aux = 0;

                  /* initialize random seed: */
                  srand (time(NULL) * cv::getTickCount());

                  cv::Mat gray(rows, cols, CV_8UC1);
                  cv::Mat grayEqualized(rows, cols, CV_8UC1);
                  cv::Mat grayEqualizedVis(rows, cols, CV_8UC1);
                  int totPxVeg = 0;
                  double tempoTotalCalcVI = 0, tempoTotalCalcAlfaBeta = 0, tempoTotalNormalizacao = 0, tempoTotalRoleta = 0;
                  clock_t tempoCalcVI, tempoCalcAlfaBeta, tempoNormalizacao, tempoRoleta;

                  tempoCalcVI = clock();
                  //Cálculo do vegetation index para toda a imagem
                  #pragma omp parallel for num_threads(NUM_THREADS) shared(totPxVeg)
                  for (int i = 0; i < rows; i++)//COMO CALCULO O VI AQUI, PASSAREI ESSE VALOR CALCULADO, E NÃO O RECALCULAREI NO PROCESSAMENTO DO DESCRITOR
                  {
                      for (int j = 0; j < cols; j++)
                      {                          
                         viMat[i][j] = vegIdx->setIndex(nirMap.at<uchar>(i, j),
                                 regMap.at<uchar>(i, j),
                                 redMap.at<uchar>(i, j),
                                 greMap.at<uchar>(i, j));

                         if(viMat[i][j] >= 0.3)//Grandes chances de conter vegetação
                            #pragma omp atomic update
                            totPxVeg++;


                         viMatrix.at<float>(i,j) =  viMat[i][j];
                         viMat[i][j] = viMat[i][j] * -1;//Inverter a probabilidade, pois no default valores grandes indicam vegetação, mas quero fazer com que os valores grandes sejam referentes à probabilidade de não haver vegetação.

                          //Desenha uma máscara falsa (cruz)
                          /*if(j > cols/2-50 && j < cols/2+50)
                              viMat[i][j] = Utils::RandomFloat(-1, -0.9)*-1;
                              //viMat[i][j] = Utils::RandomFloat(-1, 0)*-1;
                          else if(i > rows/2-50 && i < rows/2+50)
                              viMat[i][j] = Utils::RandomFloat(-1, -0.9)*-1;
                              //viMat[i][j] = Utils::RandomFloat(-1, 0)*-1;
                          else
                              viMat[i][j] = Utils::RandomFloat(0.9, 1)*-1;
                              //viMat[i][j] = Utils::RandomFloat(0, 1)*-1;*/
                          //==


                         //Escala para um range positivo, já convertendo para o que seria esperado de uma imagem 0 - 255
                         gray.at<uchar>(i,j) = Utils::Normalize(viMat[i][j], 0, 255, -1, 1);   
                      }
                  }

                  tempoTotalCalcVI = (clock() - tempoCalcVI) / (double)CLOCKS_PER_SEC;
                  cout << "Tempo do calculo do VI: " << tempoTotalCalcVI << " s \n";

                  percentualPxVegImg = (totPxVeg/(totPx/100))/100;
                  cout << "\nPercentual de vegetação: " << percentualPxVegImg << "\n";
                  cv::equalizeHist(gray, grayEqualized);

                  if(!quiet)
                  {
                      /*cv::resize(gray, gray, cv::Size(), 0.25, 0.25);
                      applyColorMap(gray, gray, cv::COLORMAP_HOT);
                      imshow("gray",gray);*/
                      cv::namedWindow("grayWin");
                      //cv::moveWindow("grayWin", 20, 55 + (0.25 * gray.rows));
                      cv::resize(gray, gray, cv::Size(), 0.25, 0.25);
                      //applyColorMap(gray, gray, cv::COLORMAP_HOT);
                      cv::imshow("grayWin", gray);


                      /*cv::resize(grayEqualized, grayEqualizedVis, cv::Size(), 0.25, 0.25);
                      applyColorMap(grayEqualizedVis, grayEqualizedVis, cv::COLORMAP_HOT);
                      imshow("grayEqualizedVis",grayEqualizedVis);*/
                      cv::namedWindow("grayEqualizedWin");
                      //cv::moveWindow("grayEqualizedWin", 20, 85 + (0.5 * grayEqualizedVis.rows));
                      cv::resize(grayEqualized, grayEqualizedVis, cv::Size(), 0.25, 0.25);
                      //applyColorMap(grayEqualizedVis, grayEqualizedVis, cv::COLORMAP_HOT);
                      imshow("grayEqualizedWin",grayEqualizedVis);
                  }

                  tempoCalcAlfaBeta = clock();
                  for (int i = 0; i < rows; i++)
                  {
                      for (int j = 0; j < cols; j++)
                      {   //Escala de 0 a 255 para de 0 a 1.
                          viMat[i][j] = Utils::Normalize(grayEqualized.at<uchar>(i,j), 0, 1, 0, 255);
                          viMat[i][j] = (alphaVegetationMask * viMat[i][j]) + (1-alphaVegetationMask) * gaussianMsk.at<float>(i, j);
                          sumScaled += viMat[i][j];
                      }
                  }
                  tempoTotalCalcAlfaBeta = (clock() - tempoCalcAlfaBeta) / (double)CLOCKS_PER_SEC;
                  cout << "Tempo do calculo alfa beta: " << tempoTotalCalcAlfaBeta << " s \n";

                  tempoNormalizacao = clock();
                  //Normalizar a matriz de VIs de modo que a soma totalize 1
                  for (int i = 0; i < rows; i++)
                  {
                      for (int j = 0; j < cols; j++)
                      {
                          viMat[i][j] = viMat[i][j]/sumScaled;
                          sumNormalized += viMat[i][j];
                      }
                  }
                  tempoTotalNormalizacao = (clock() - tempoNormalizacao) / (double)CLOCKS_PER_SEC;
                  cout << "Tempo da Normalizacao: " << tempoTotalNormalizacao << " s \n";

                  tempoRoleta = clock();
                  //Roleta
                  #pragma omp parallel for num_threads(NUM_THREADS) private(aux,limite,pixelPair)
                  for(int pair = 0; pair < totalPairs; pair++)//Itera por todos os pares de pixeis
                  {
                      pixelPair = 0;
                      while(pixelPair < 2)//Itera pelos pixeis do par
                      {
                          aux = 0;
                          limite = Utils::RandomFloat(0, sumNormalized);

                          for (int iElem = 0; iElem < rows && limite > aux; iElem++)
                          {
                              for (int jElem = 0; jElem < cols && limite > aux; jElem++)
                              {
                                  aux += viMat[iElem][jElem];//Incrementando aqui eu não preciso dos loopings para calcular a porbabilidade individual de cada item

                                  if(limite <= aux)
                                  {
                                      if(jElem > 0)
                                          pairs[pair][pixelPair] = cv::Point(jElem-1, iElem);
                                      else//Linha anterior
                                          if(iElem > 0)
                                              pairs[pair][pixelPair] = cv::Point(jElem, iElem - 1);
                                          else
                                          {
                                              pairs[pair][pixelPair] = cv::Point(jElem, iElem);
                                              cout<< "Linha e coluna 0.";
                                          }

                                      pixelPair++;

                                      if(pixelPair == 1)
                                      {
                                          iElem = 0; jElem = 0;
                                          limite = Utils::RandomFloat(0, sumNormalized);
                                      }
                                  }

                                  if(pixelPair == 2)//Avança para o próximo par de pixeis
                                      break;
                              }
                              if(pixelPair == 2)//Avança para o próximo par de pixeis
                                  break;
                          }
                      }
                  }
                  tempoTotalRoleta = (clock() - tempoRoleta) / (double)CLOCKS_PER_SEC;
                  cout << "Tempo da Roleta: " << tempoTotalRoleta << " s \n";

                  if(!quiet)
                  {
                      //Reverter a normalização para melhorar a visualização
                      for (int i = 0; i < rows; i++)
                      {
                          for (int j = 0; j < cols; j++)
                          {
                              viMat[i][j] = viMat[i][j] * sumScaled;
                          }
                      }

                      //Exibição da máscara gerada pela distribuição
                      /*cv::Mat pixelDistMask(rows, cols, CV_32FC1, viMat);
                      cv::resize(pixelDistMask, pixelDistMask, cv::Size(), 0.25, 0.25);
                      imshow("pixelDistMask", pixelDistMask);*/

                      cv::namedWindow("pixelDistMaskWin");
                      //cv::moveWindow("pixelDistMaskWin", 20,20);
                      cv::Mat pixelDistMask(rows, cols, CV_32FC1, viMat);
                      cv::resize(pixelDistMask, pixelDistMask, cv::Size(), 0.25, 0.25);
                      cv::imshow("pixelDistMaskWin", pixelDistMask);

                      //pixelDistMask.convertTo(pixelDistMask, CV_8UC3, 255.0);
                      //imwrite("/home/phi/Documents/Images/pixelDistMask.jpg", pixelDistMask);
                  }
                  break;
              }

        case(52)://(OK) Roulette-wheel + Gaussian (double)
        {
            int rows = nirMap.rows;
            int cols = nirMap.cols;
            double viMat[rows][cols];
            double sumNormalized = 0, sumScaled = 0;
            double limite = 0, aux = 0;
            //float minimumFitness = FLOAT_MAX;
            //float widowingMinFitDec = 0;//Decremento da fitness de todos os elementos, com base na menor fitness.

            /* initialize random seed: */
            srand (time(NULL) * cv::getTickCount());

            cv::Mat grey(rows, cols, CV_8UC1);
            cv::Mat greyHis(rows, cols, CV_8UC1);
            cv::Mat greyHisVis(rows, cols, CV_8UC1);

            //Cálculo do vegetation index para toda a imagem
            for (int i = 0; i < rows; i++)//COMO CALCULO O VI AQUI, PASSAREI ESSE VALOR CALCULADO, E NÃO O RECALCULAREI NO PROCESSAMENTOD O DESCRITOR
            {
                for (int j = 0; j < cols; j++)
                {
                   /*viMat[i][j] = vegIdx->setIndexD(nirMap.at<uchar>(i, j),
                           regMap.at<uchar>(i, j),
                           redMap.at<uchar>(i, j),
                           greMap.at<uchar>(i, j));

                    viMatrix.at<double>(i,j) =  viMat[i][j];//Vi da imagem armazenado para a etapa dos pesos (está entre -1 e 1)
                    viMat[i][j] = viMat[i][j] * -1;//Inverter a probabilidade, pois no default valores grandes indicam vegetação, mas quero fazer com que os valores grandes sejam referentes à probabilidade de não haver vegetação.
                    */

                    viMatrix.at<double>(i,j) = vegIdx->setIndexD(nirMap.at<uchar>(i, j),
                            regMap.at<uchar>(i, j),
                            redMap.at<uchar>(i, j),
                            greMap.at<uchar>(i, j));

                     //viMatrix.at<double>(i,j) =  viMat[i][j];//Vi da imagem armazenado para a etapa dos pesos (está entre -1 e 1)
                     //viMat[i][j] = viMat[i][j] * -1;//Inverter a probabilidade, pois no default valores grandes indicam vegetação, mas quero fazer com que os valores grandes sejam referentes à probabilidade de não haver vegetação.



                    //Desenha uma máscara falsa (cruz)
                    /*if(j > cols/2-50 && j < cols/2+50)
                        viMat[i][j] = Utils::RandomFloat(-1, -0.9)*-1;
                        //viMat[i][j] = Utils::RandomFloat(-1, 0)*-1;
                    else if(i > rows/2-50 && i < rows/2+50)
                        viMat[i][j] = Utils::RandomFloat(-1, -0.9)*-1;
                        //viMat[i][j] = Utils::RandomFloat(-1, 0)*-1;
                    else
                        viMat[i][j] = Utils::RandomFloat(0.9, 1)*-1;
                        //viMat[i][j] = Utils::RandomFloat(0, 1)*-1;*/
                    //==                   

                   //Escalar a matriz de VIs para 0 , 1
                   //viMat[i][j] = Utils::Normalize(viMat[i][j], 0, 1, -1, 1);
                   //sumScaled += viMat[i][j];

                   /*if(pixelDistMethod == 51)
                   {
                       //Aplicar a máscara gaussiana
                       viMat[i][j] += gaussianMsk.at<float>(i, j) * multiGaussianMask;
                   }*/

                   //Minimum fitness -  usado no windowing
                   //if(viMat[i][j] < minimumFitness)
                   //    minimumFitness = viMat[i][j];

                   //grey.at<uchar>(i,j) = viMat[i][j]*255;

                   //Escala para um range positivo, já convertendo para o que seria esperado de uma imagem 0 - 255
                   //grey.at<uchar>(i,j) = Utils::Normalize(viMat[i][j], 0, 255, -1, 1);
                   grey.at<uchar>(i,j) = Utils::Normalize(viMatrix.at<double>(i,j), 0, 255, -1, 1);
                }
            }

            cv::equalizeHist(grey, greyHis);

            if(!quiet)
            {
                cv::resize(grey, grey, cv::Size(), 0.25, 0.25);
                applyColorMap(grey, grey, cv::COLORMAP_HOT);
                imshow("grey",grey);
                cv::resize(greyHis, greyHisVis, cv::Size(), 0.25, 0.25);
                applyColorMap(greyHisVis, greyHisVis, cv::COLORMAP_HOT);
                imshow("greyHisVis",greyHisVis);
            }

            //cv::normalize(greyHis, greyHis, 0, 1, cv::NORM_MINMAX, CV_8UC1);//Se o range do vi possuir negativo, tipo -1 até 1, a faixa de -1 é ignorada.
            //imshow("greyHisN",greyHis);

            for (int i = 0; i < rows; i++)
            {
                for (int j = 0; j < cols; j++)
                {
                    viMat[i][j] =  greyHis.at<uchar>(i,j);
                    //viMat[i][j] = Utils::Normalize(greyHis.at<uchar>(i,j), 0, 1, 0, 255);

                    if(pixelDistMethod == 51)
                    {
                        //Aplicar a máscara gaussiana
                        //viMat[i][j] +=  Utils::Normalize(gaussianMsk.at<float>(i, j) * multiGaussianMask, 0, 255, 0, 1);

                        //H(p) = aG(p) + (1-a)V(p)
                        //viMat[i][j] = (alphaVegetationMask * gaussianMsk.at<double>(i, j)) + (1-alphaVegetationMask) * viMat[i][j];
                        viMat[i][j] = (alphaVegetationMask * viMat[i][j]) + (1-alphaVegetationMask) * gaussianMsk.at<double>(i, j);
                    }
                    sumScaled += viMat[i][j];
                    //cout << "i: " << i << " j: " << " viMat[i][j]: " << viMat[i][j] << " sumScaled: " << sumScaled << "\n";
                }
            }

            //if(!quiet)
            //{
                //Print da VI Mat
                //cv::Mat OriginalViMat(rows, cols, CV_32F, viMat);
                //OriginalViMat.convertTo(OriginalViMat, CV_8UC3, 1);
                //imwrite("/home/phi/Documents/Images/OriginalViMat.jpg", OriginalViMat);
//                cv::Mat ResizedViMat;
//                cv::resize(OriginalViMat, ResizedViMat, cv::Size(), 0.25, 0.25);
//                imshow("OriginalViMat",OriginalViMat);
            //}


            //Windowing (subtraindo 90% do elemento min fitness,  de todos os elementos)
            /*widowingMinFitDec = minimumFitness * 0.9;
            for (int i = 0; i < rows; i++)
                for (int j = 0; j < cols; j++)
                    viMat[i][j] -= widowingMinFitDec;*/

            //Normalizar a matriz de VIs de modo que a soma totalize 1
            for (int i = 0; i < rows; i++)
            {
                for (int j = 0; j < cols; j++)
                {
                    viMat[i][j] = viMat[i][j]/sumScaled;
                    sumNormalized += viMat[i][j];
                }
            }

            for(int pair = 0; pair < totalPairs; pair++)//Itera por todos os pares de pixeis
            {
                pixelPair = 0;                
                while(pixelPair < 2)//Itera pelos pixeis do par
                {
                    aux = 0;
                    //limite = Utils::RandomFloat(0, sumNormalized);
                    limite = Utils::RandomDouble(0, sumNormalized);

                    for (int iElem = 0; iElem < rows && limite > aux; iElem++)
                    {
                        for (int jElem = 0; jElem < cols && limite > aux; jElem++)
                        {
                            aux += viMat[iElem][jElem];//Incrementando aqui eu não preciso dos loopings para calcular a porbabilidade individual de cada item

                            if(limite <= aux)
                            {
                                if(jElem > 0)
                                    pairs[pair][pixelPair] = cv::Point(jElem-1, iElem);
                                else//Linha anterior
                                    if(iElem > 0)
                                        pairs[pair][pixelPair] = cv::Point(jElem, iElem - 1);
                                    else
                                    {
                                        pairs[pair][pixelPair] = cv::Point(jElem, iElem);
                                        cout<< "Linha e coluna 0.";
                                    }

                                pixelPair ++;

                                if(pixelPair == 1)
                                {
                                    iElem = 0; jElem = 0;
                                    //limite = Utils::RandomFloat(0, sumNormalized);
                                    limite = Utils::RandomDouble(0, sumNormalized);
                                }
                            }

                            if(pixelPair == 2)//Avança para o próximo par de pixeis
                                break;
                        }
                        if(pixelPair == 2)//Avança para o próximo par de pixeis
                            break;                        
                    }                    
                }
            }            

            //Reverter a normalização para melhorar a visualização
            /*for (int i = 0; i < rows; i++)
            {
                for (int j = 0; j < cols; j++)
                {
                    viMat[i][j] = viMat[i][j] * sumScaled;
                }
            }

            //Exibição da máscara gerada pela distribuição
            cv::Mat pixelDistMask(rows, cols, CV_32FC1, viMat);
            imshow("pixelDistMask", pixelDistMask);
            pixelDistMask.convertTo(pixelDistMask, CV_8UC3, 255.0);
            imwrite("/home/phi/Documents/Images/pixelDistMask.jpg", pixelDistMask);*/
            break;
        }
        case(6)://(OK) Tournament
        case(61)://(OK) Tournament + Gaussian
        {
            int k = 3; //Tamanho do torneio (quantidade de indivíduos selecionados por vez)
            int x[k], y[k];
            cv::Point points[k];
            float fitnessScore[k];
            int idxMelhorIndv = 0;

            /* initialize random seed: */
            srand (time(NULL) * cv::getTickCount());

            for(int i = 0; i < totalPairs; i++)//Itera por todos os pares de pixeis
            {
                pixelPair = 0;
                while(pixelPair < 2)//Itera pelos pixels do par
                {
                    for(int individuo = 0; individuo < k; individuo++)
                    {
                        if(pixelDistMethod == 61)
                        {
                            /* initialize random seed: */
                            cv::theRNG().state = time(NULL) * cv::getTickCount();

                            //Gaussian Selection
                            x[individuo] = (int)rng.gaussian(sigmaX)+drone.cols/2;
                            y[individuo] = (int)rng.gaussian(sigmaY)+drone.rows/2;
                        }
                        else
                        {
                            /* initialize random seed: */
                            srand (time(NULL) * cv::getTickCount());

                            x[individuo] = floor(rand() % drone.cols);
                            y[individuo] = floor(rand() % drone.rows);
                        }

                        points[individuo] = cv::Point(x[individuo], y[individuo]);

                        if(pointIn(points[individuo], drone))
                        {
                            fitnessScore[individuo] = vegIdx->setIndex(nirMap.at<uchar>(y[individuo], x[individuo]),
                                                            regMap.at<uchar>(y[individuo], x[individuo]),
                                                            redMap.at<uchar>(y[individuo], x[individuo]),
                                                            greMap.at<uchar>(y[individuo], x[individuo]));
                            //fitnessScore[individuo] = fakeViMat[y[individuo]][x[individuo]]; // Usar a máscara em cruz
                            if(individuo > 0)
                            {
                                if(fitnessScore[individuo] < fitnessScore[individuo-1])
                                    idxMelhorIndv = individuo;
                                else
                                    idxMelhorIndv = individuo - 1;
                            }
                        }
                        else
                            individuo --;
                    }
                    pairs[i][pixelPair] = cv::Point(x[idxMelhorIndv], y[idxMelhorIndv]);
                    pixelPair ++;
                }
            }

            break;
        }

        case(7)://Random Pool Selection + Gaussian, usando o Roulette Wheel como base
        {
            int rows = nirMap.rows;
            int cols = nirMap.cols;
            float viMat[rows][cols];
            float secStep = 0;
            float fitnessScore = 0;

            /* initialize random seed: */
            srand (time(NULL) * cv::getTickCount());

            cv::Mat grey(rows, cols, CV_8UC1);
            cv::Mat greyHis(rows, cols, CV_8UC1);
            cv::Mat greyHisVis(rows, cols, CV_8UC1);

            //Cálculo do vegetation index para toda a imagem
            for (int i = 0; i < rows; i++)//COMO CALCULO O VI AQUI, PASSAREI ESSE VALOR CALCULADO, E NÃO O RECALCULAREI NO PROCESSAMENTOD O DESCRITOR
            {
                for (int j = 0; j < cols; j++)
                {
                   viMat[i][j] = vegIdx->setIndex(nirMap.at<uchar>(i, j),
                           regMap.at<uchar>(i, j),
                           redMap.at<uchar>(i, j),
                           greMap.at<uchar>(i, j));

                   viMat[i][j] = viMat[i][j] * -1;//Inverter a probabilidade, pois no default valores grandes indicam vegetação, mas quero fazer com que os valores grandes sejam referentes à probabilidade de não haver vegetação.

                   //Escala para um range positivo, já convertendo para o que seria esperado de uma imagem 0 - 255
                   grey.at<uchar>(i,j) = Utils::Normalize(viMat[i][j], 0, 255, -1, 1);
                }
            }
            cv::equalizeHist(grey, greyHis);

            if(!quiet)
            {
                cv::resize(grey, grey, cv::Size(), 0.25, 0.25);
                imshow("grey",grey);
                cv::resize(greyHis, greyHisVis, cv::Size(), 0.25, 0.25);
                imshow("greyHisVis",greyHisVis);
            }            

            for (int i = 0; i < rows; i++)
            {
                for (int j = 0; j < cols; j++)
                {
                    viMat[i][j] =  Utils::Normalize(greyHis.at<uchar>(i,j), 0, 1, 0, 255);//escala de 0 a 255 para 0 a 1.

                    //Aplicar a máscara gaussiana
                    //viMat[i][j] +=  gaussianMsk.at<float>(i, j) * multiGaussianMask;//A gaussianMsk está entre 0 e 1, adicionada ao vi que estava entre 0 e 1, resultará em um range entre 0 e 2.
                    //cout<< "\n viMat[" << i << "][" << j << "]: " << viMat[i][j];

                    //H(p) = aG(p) + (1-a)V(p)
                    //viMat[i][j] = (alphaVegetationMask * gaussianMsk.at<float>(i, j)) + (1-alphaVegetationMask) * viMat[i][j];
                    viMat[i][j] = (alphaVegetationMask * viMat[i][j]) + (1-alphaVegetationMask) * gaussianMsk.at<float>(i, j);
                }
            }

            for(int i = 0; i < totalPairs; i++)//Itera por todos os pares de pixeis
            {
                pixelPair = 0;
                while(pixelPair < 2)//Itera pelos pixeis do par
                {
                    //1st Step: select a pixel
                    /* initialize random seed: */
                    srand (time(NULL) * cv::getTickCount());

                    x = floor(rand() % drone.cols);
                    y = floor(rand() % drone.rows);


                    point = cv::Point(x, y);
                    if(pointIn(pairs[i][pixelPair], drone))//point - erro
                    {
                        //2st Step: accept/reject based on its  fitness (NDVI value - lower better)
                        secStep = Utils::RandomFloat(0, 2);

                        fitnessScore = viMat[y][x];

                        if(secStep < fitnessScore)
                        {
                            pairs[i][pixelPair] = cv::Point(x, y);
                            pixelPair ++;
                        }
                    }
                }
            }
            break;
        }

        case(1)://Gaussian
        case(11)://Old Gaussian
        default://(OK) Gaussian
        {
            for(int x=0;x<totalPairs;x++)
            {
                do
                {
                    pairs[x][0] = cv::Point((int)rng.gaussian(sigmaX)+drone.cols/2,
                                            (int)rng.gaussian(sigmaY)+drone.rows/2);
                }
                while(!pointIn(pairs[x][0], drone));

                do
                {              
                    pairs[x][1] = cv::Point((int)rng.gaussian(sigmaX)+drone.cols/2,
                                            (int)rng.gaussian(sigmaY)+drone.rows/2);
                }
                while(!pointIn(pairs[x][1], drone));
            }            
            break;
        }
    }

    tmpExecPxDist += (clock() - tempo) / (double)CLOCKS_PER_SEC;
    qtdChamadasPxDist++;

    cout << "Tempo médio de execução da distribuição dos pixeis: " << tmpExecPxDist/qtdChamadasPxDist << " s \n";    
}

void BriefHeuristic::drawPixelDistribution(cv::Mat& localMap)
{
     cv::Mat map = localMap.clone();

    for(int x=0;x<totalPairs;x++)
    {
        cv::rectangle( map, cv::Point2f( pairs[x][0].x-1, pairs[x][0].y-1 ), cv::Point2f( pairs[x][0].x+1, pairs[x][0].y+1 ), cv::Scalar( 0, 0, 255 ) );
        cv::rectangle( map, cv::Point2f( pairs[x][1].x-1, pairs[x][1].y-1 ), cv::Point2f( pairs[x][1].x+1, pairs[x][1].y+1 ), cv::Scalar( 0, 0, 255 ) );

        cv::rectangle( map, cv::Point2f( pairs[x][0].x-2, pairs[x][0].y-2 ), cv::Point2f( pairs[x][0].x+2, pairs[x][0].y+2 ), cv::Scalar( 0, 0, 255 ) );
        cv::rectangle( map, cv::Point2f( pairs[x][1].x-2, pairs[x][1].y-2 ), cv::Point2f( pairs[x][1].x+2, pairs[x][1].y+2 ), cv::Scalar( 0, 0, 255 ) );

        cv::rectangle( map, cv::Point2f( pairs[x][0].x-3, pairs[x][0].y-3 ), cv::Point2f( pairs[x][0].x+3, pairs[x][0].y+3 ), cv::Scalar( 0, 0, 255 ) );
        cv::rectangle( map, cv::Point2f( pairs[x][1].x-3, pairs[x][1].y-3 ), cv::Point2f( pairs[x][1].x+3, pairs[x][1].y+3 ), cv::Scalar( 0, 0, 255 ) );
    }


    //imwrite("/home/phi/Documents/Images/vegetationMask.jpg", map);
    /*cv::resize(map, map, cv::Size(), 0.4, 0.4);
    imshow("Pixel Distribution (MSabBRIEF)", map);*/
    cv::namedWindow("PixelDistributionWin");
    //cv::moveWindow("PixelDistributionWin", 1070, 20);
    cv::resize(map, map, cv::Size(), 0.4, 0.4);
    imshow("PixelDistributionWin", map);
}

//DRONE
void BriefHeuristic::updateDroneDescriptor(cv::Mat& drone)
{
    width = drone.cols;
    height = drone.rows;

    if(pairs.size() < 1){
        for(int p=0; p<totalPairs; p++)
        {
            vector<cv::Point> pair(2);
            pairs.push_back(pair);
        }
    }

    cv::RNG rng;
    for(int x=0;x<totalPairs;x++)
    {
        do{
            pairs[x][0] = cv::Point((int)rng.gaussian(1.0/25.0*pow(drone.cols,2))+drone.cols/2, (int)rng.gaussian(1.0/25.0*pow(drone.rows,2))+drone.rows/2);
        }while(!pointIn(pairs[x][0], drone));
        do{
            pairs[x][1] = cv::Point((int)rng.gaussian(1.0/25.0*pow(drone.cols,2))+drone.cols/2, (int)rng.gaussian(1.0/25.0*pow(drone.rows,2))+drone.rows/2);
        }while(!pointIn(pairs[x][1], drone));
    }

    vector<int> bin;
    for(int c=0;c<pairs.size();c++)
    {
        vec3 color1(getValuefromPixel(pairs[c][0].x,pairs[c][0].y,&drone));
        vec3 color2(getValuefromPixel(pairs[c][1].x,pairs[c][1].y,&drone));

        //Lab with 3 bits
        if(type == 1)
        {
            if(color1.r>color2.r) bin.push_back(1);
            else bin.push_back(0);
            if(color1.g>color2.g) bin.push_back(1);
            else bin.push_back(0);
            if(color1.b>color2.b) bin.push_back(1);
            else bin.push_back(0);
        }

        //Lab with 6 bits
        else if(type == 2)
        {
            if(color1.r > color2.r){
                bin.push_back(1);
                bin.push_back(0);
            }else if(color1.r < color2.r){
                bin.push_back(0);
                bin.push_back(1);
            }else{
                bin.push_back(0);
                bin.push_back(0);
            }
            if(color1.g > color2.g){
                bin.push_back(1);
                bin.push_back(0);
            }else if(color1.g < color2.g){
                bin.push_back(0);
                bin.push_back(1);
            }else{
                bin.push_back(0);
                bin.push_back(0);
            }
            if(color1.b > color2.b){
                bin.push_back(1);
                bin.push_back(0);
            }else if(color1.b < color2.b){
                bin.push_back(0);
                bin.push_back(1);
            }else{
                bin.push_back(0);
                bin.push_back(0);
            }
        }

        //GRAY
        else if(type == 0)
        {
            float r = color1.b * 0.114;//B
            float g = color1.g * 0.587;//G
            float b = color1.r * 0.299;//R

            int sum1 = r + g + b;

            r = color2.b * 0.114;//B
            g = color2.g * 0.587;//G
            b = color2.r * 0.299;//R

            int sum2 = r + g + b;

            if(sum1 > sum2) bin.push_back(1);
            else bin.push_back(0);
        }

        else if(type == 3)
        {
            int diff = (color1.r-color2.r);
            bin.push_back(diff);
            diff = (color1.g-color2.g);
            bin.push_back(diff);
            diff = (color1.b-color2.b);
            bin.push_back(diff);
        }
        //LAB WITH OUT L
        else if(type == 4)
        {
            if(color1.r>color2.r) bin.push_back(1);
            else bin.push_back(0);
            if(color1.g>color2.g) bin.push_back(1);
            else bin.push_back(0);
        }
    }
    droneDescriptor = bin;
}

//Método responsável por calcular a máscara de pesos do descritor do drone,
//com base em um índice de vegetação
void BriefHeuristic::CalculateVIMask(cv::Mat &redMap, cv::Mat &nirMap, cv::Mat regMap, cv::Mat greMap, cv::Mat viMatrix)
{
    //Ndvi *ndvi = new Ndvi(0, ndvi_type);
    VegetationIndex *vegIdx = VegetatonIndexGen::InitVegetationIndex();
    sumNdviWeightsDrone = 0;

    for(int i= 0; i < totalPairs; i++)
    {
        if(pixelDistMethod != 51 && pixelDistMethod != 52 && pixelDistMethod != 2)//Nas roletas (51, 52 e 2), o NDVI já é calculado para toda a imagem
        {
            //Calcular o VI para o par em questão
            //Px 1
            pairVI[i][0] = vegIdx->setIndex(nirMap.at<uchar>(pairs[i][0].y, pairs[i][0].x),
                    regMap.at<uchar>(pairs[i][0].y, pairs[i][0].x),
                    redMap.at<uchar>(pairs[i][0].y, pairs[i][0].x),
                    greMap.at<uchar>(pairs[i][0].y, pairs[i][0].x));

            //Px 2
            pairVI[i][1] = vegIdx->setIndex(nirMap.at<uchar>(pairs[i][1].y, pairs[i][1].x),
                    regMap.at<uchar>(pairs[i][1].y, pairs[i][1].x),
                    redMap.at<uchar>(pairs[i][1].y, pairs[i][1].x),
                    greMap.at<uchar>(pairs[i][1].y, pairs[i][1].x));
        }
        else
        {
            if(VIDouble)
            {
                pairVI[i][0] = viMatrix.at<double>(pairs[i][0].y,pairs[i][0].x);
                pairVI[i][1] = viMatrix.at<double>(pairs[i][1].y,pairs[i][1].x);
            }
            else
            {
                pairVI[i][0] = viMatrix.at<float>(pairs[i][0].y,pairs[i][0].x);
                pairVI[i][1] = viMatrix.at<float>(pairs[i][1].y,pairs[i][1].x);
            }
        }

        ///////////////////////////////////////////////////
        //Peso do par de pixeis, após o cálculo do NDVI (o valor está normalizado entre -1 e 1)
        //novoX = X - min(X)/max(X) - min(X)
        //pairWeight[i] = ((pairVI[i][0] + pairVI[i][1]) +2) /(2 + 2); escalando a soma dos NDVI para para 0 à 1

        //Escalar para outro range (MIN_NDVI_WEIGHT - MAX_NDVI_WEIGHT)
        //range = maxNdviWeight - minNdviWeight
        //novoX = (x * range) + x;
        //pairWeight[i] = (pairWeight[i] * (maxNdviWeight - minNdviWeight)) + pairWeight[i];

        //sumNdviWeightsDrone += pairWeight[i];
        ///////////////////////////////////////////////////

        ///////////////////////////////////////////////////
        //Adotar uma nova formula para a atribuição dos pesos dos pares de píxeis
        //Escalar a soma do NDVI dos pixeis do par, de -2 à 2, para 0 à 1
        //pairWeight[i] = Utils::Normalize((pairVI[i][0] + pairVI[i][1]), 0, 1, -2, 2);

        //(NDVI * weight) + 1
        //((NDVIpx1 + NDVIpx2) * weight) + 1
        //pairWeight[i] = (pairWeight[i] * betaHammingWeight) + 1;

        //sumNdviWeightsDrone += pairWeight[i];
        ///////////////////////////////////////////////////


        //Forma mais compreensível e com limites
        //Escalar a soma do NDVI dos pixeis do par, de -2 à 2, para 0 à 1
        pairWeight[i] = Utils::Normalize((pairVI[i][0] + pairVI[i][1])*-1, 0, 1, -2, 2);//Multiplico a soma do NDVI por -1 para fazer  com que a grande concentração de vegetação tenda a 2, ao invés de -2. Assim ao escalar para 0-1, muita vegetação ficará próximo de 0.

        //(NDVI * weight) + 0.01
        //((NDVIpx1 + NDVIpx2) * weight) + 0.01
        pairWeight[i] = (pairWeight[i] * betaHammingWeight) + 0.01;
        //0.01 é o valor mínimo que um par de pixeis pode assumir (máximo de vegetação)
        //enquanto o valor máximo que um pixel pode assumir é 1 (nenhuma vegetação)
        //Sendo que beta varia de 0 à 0.99
        sumNdviWeightsDrone += pairWeight[i];
    }
}

//MAPA
//Com câmera multiespectral
double BriefHeuristic::calculateValue2(Pose3d p, cv::Mat *map, cv::Mat &redMap, cv::Mat &nirMap, cv::Mat &regMap, cv::Mat &greMap, cv::Mat &currentMapOriginal, Pose3d orientacao)
{
    vector<int> bin;
    clock_t tempo;
    tempo = clock();

    int bitsPerPair;
    switch (type)
    {
        case 0:
            bitsPerPair = 1;
            break;
        case 1:
            bitsPerPair = 3;
            break;
        case 2:
            bitsPerPair = 6;
            break;
        case 4:
            bitsPerPair = 2;
            break;
    }

    double scale = p.z;
    float theta = p.yaw*(-1)-1.5708;//-1.5708 = -90 graus
    int xPad = width/2 - 0.5;
    int yPad = height/2 - 0.5;

    double dir[4];//Direcao inicial (Apontando p baixo)[x y z 0], e localizado na origem
    double cosPitch;
    double sinPitch;
    double cosYaw;
    double sinYaw;
    double cosRoll;
    double sinRoll;
    double particleYaw = p.yaw*(-1)-1.5708;//-1.5708 = -90 graus
    if(corrPixelPosAngles)
    {
        dir[0] = {0};
        dir[1] = 0;
        dir[2] = (-scale /0.366197183) * (multExcalaZ);
        dir[3] = 0;
        //Orientações do GT
        //cosPitch = cos(orientacao.pitch); rad
        //sinPitch = sin(orientacao.pitch); rad
        //cosYaw   = cos(orientacao.yawInvertido); rad
        //sinYaw   = sin(orientacao.yawInvertido); rad
        //cosRoll  = cos(orientacao.roll); rad
        //sinRoll  = sin(orientacao.roll); rad
        //Orientações estimadas
        cosPitch = cos(p.pitch);
        sinPitch = sin(p.pitch);
        cosYaw   = cos(particleYaw);
        sinYaw   = sin(particleYaw);
        cosRoll  = cos(p.roll);
        sinRoll  = sin(p.roll);
    }
    float cosTheta = cos(theta);
    float sinTheta = sin(theta);

    for(int c=0; c<pairs.size(); c++)
    {
        //Projeção dos pares de pontos do descritor do drone, no "patch" do mapa.
        //Rotação e escala
        //Pixel 1 (projeta o ponto do par no chão - seria o que a partícula vê com roll e pitch nulos, apenas com o yaw)
        cv::Point point1 = cv::Point(((pairs[c][0].x-xPad)*scale*cosTheta+(pairs[c][0].y-yPad)*scale*sinTheta)+p.x,
                                    ((pairs[c][0].x-xPad)*scale*-sinTheta+(pairs[c][0].y-yPad)*scale*cosTheta)+p.y);

        if(corrPixelPosAngles)
        {
            //Rotacionar em roll, pitch e yaw (mover esse ponto que a partícula estaria vendo, com base na orientação do drone)
            point1.x += dir[0]*(cosPitch*cosYaw) + dir[1]*(sinRoll*sinPitch*cosYaw-cosRoll*sinYaw) + dir[2]*(cosRoll*sinPitch*cosYaw+sinRoll*sinYaw);
            point1.y += dir[0]*(cosPitch*sinYaw) + dir[1]*(sinRoll*sinPitch*sinYaw+cosRoll*cosYaw) + dir[2]*(cosRoll*sinPitch*sinYaw-sinRoll*cosYaw);
            //z = ((dir[0]*-sinPitch + dir[1]*(sinRoll*cosPitch) + dir[2]*(cosRoll*cosPitch))) * 0,366197183 / 45*0,188679245;
        }

        if(point1.x < 0 || point1.x >= map->cols || point1.y < 0 || point1.y >= map->rows)
        {
            bin.push_back(HEURISTIC_UNDEFINED_INT);
            bin.push_back(HEURISTIC_UNDEFINED_INT);
            bin.push_back(HEURISTIC_UNDEFINED_INT);
        }
        else
        {
            //Pixel 2
            cv::Point point2 = cv::Point(((pairs[c][1].x-xPad)*scale*cosTheta+(pairs[c][1].y-yPad)*scale*sinTheta)+p.x,
                    ((pairs[c][1].x-xPad)*scale*-sinTheta+(pairs[c][1].y-yPad)*scale*cosTheta)+p.y);

            if(corrPixelPosAngles)
            {
                //Rotacionar em roll, pitch e yaw
                point2.x += dir[0]*(cosPitch*cosYaw) + dir[1]*(sinRoll*sinPitch*cosYaw-cosRoll*sinYaw) + dir[2]*(cosRoll*sinPitch*cosYaw+sinRoll*sinYaw);
                point2.y += dir[0]*(cosPitch*sinYaw) + dir[1]*(sinRoll*sinPitch*sinYaw+cosRoll*cosYaw) + dir[2]*(cosRoll*sinPitch*sinYaw-sinRoll*cosYaw);
                //já foi calulado, o mesmo para as os 2 pixeis   //scale += ((dir[0]*-sinPitch + dir[1]*(sinRoll*cosPitch) + dir[2]*(cosRoll*cosPitch))) * 0,366197183 / 45*0,188679245;
            }

            if(point2.x < 0 || point2.x >= map->cols || point2.y < 0 || point2.y >= map->rows){
                bin.push_back(HEURISTIC_UNDEFINED_INT);
                bin.push_back(HEURISTIC_UNDEFINED_INT);
                bin.push_back(HEURISTIC_UNDEFINED_INT);
            }
            else
            {
                cv::Vec3b color1 = map->at<cv::Vec3b>(point1.y, point1.x);
                cv::Vec3b color2 = map->at<cv::Vec3b>(point2.y, point2.x);

                //Lab
                if(type == 1)
                {
                    if(color1[0]>color2[0]) bin.push_back(1);
                    else bin.push_back(0);
                    if(color1[1]>color2[1]) bin.push_back(1);
                    else bin.push_back(0);
                    if(color1[2]>color2[2]) bin.push_back(1);
                    else bin.push_back(0);
                }

                else if(type == 2)
                {
                    if(color1[0] > color2[0]){
                        bin.push_back(1);
                        bin.push_back(0);
                    }else if(color1[0] < color2[0]){
                        bin.push_back(0);
                        bin.push_back(1);
                    }else{
                        bin.push_back(0);
                        bin.push_back(0);
                    }
                    if(color1[1] > color2[1]){
                        bin.push_back(1);
                        bin.push_back(0);
                    }else if(color1[1] < color2[1]){
                        bin.push_back(0);
                        bin.push_back(1);
                    }else{
                        bin.push_back(0);
                        bin.push_back(0);
                    }
                    if(color1[2] > color2[2]){
                        bin.push_back(1);
                        bin.push_back(0);
                    }else if(color1[2] < color2[2]){
                        bin.push_back(0);
                        bin.push_back(1);
                    }else{
                        bin.push_back(0);
                        bin.push_back(0);
                    }
                }

                //GRAY
                else if(type == 0)
                {
                    float r = color1.val[0] * 0.114;//B
                    float g = color1.val[1] * 0.587;//G
                    float b = color1.val[2] * 0.299;//R

                    int sum1 = r + g + b;

                    r = color2.val[0] * 0.114;//B
                    g = color2.val[1] * 0.587;//G
                    b = color2.val[2] * 0.299;//R

                    int sum2 = r + g + b;

                    if(sum1 > sum2) bin.push_back(1);
                    else bin.push_back(0);
                }

                else if(type == 3)
                {
                    int diff = (color1[0]-color2[0]);
                    bin.push_back(diff);
                    diff = (color1[1]-color2[1]);
                    bin.push_back(diff);
                    diff = (color1[2]-color2[2]);
                    bin.push_back(diff);
                }
                //LAB WITH OUT L
                else if(type == 4)
                {
                    if(color1[0]>color2[0]) bin.push_back(1);
                    else bin.push_back(0);
                    if(color1[1]>color2[1]) bin.push_back(1);
                    else bin.push_back(0);
                }
            }
        }
    }

    //visualizaCameraPoseOriCorr(p, orientacao, true);

    //BRIEF cálculo da diferença entre o descritor do drone e o descritor do "patch"
    double result = 0;
    double sumNdviWeightsPatch = 0;
    int pairCount = 0;
    int droneImageUndefined = 0;

    int diff = 0;

    for(int i = 0; i < droneDescriptor.size(); i++)
    {
        if(droneDescriptor[i] == HEURISTIC_UNDEFINED_INT
                || bin[i] == HEURISTIC_UNDEFINED_INT
                || bin[i] != droneDescriptor[i])
        {
            diff++;
            sumNdviWeightsPatch += pairWeight[pairCount];
        }
        if(droneDescriptor[i] == HEURISTIC_UNDEFINED_INT)
        {
            droneImageUndefined++;
            sumNdviWeightsPatch -= pairWeight[pairCount];
        }
        if(i % 2 != 0)
            pairCount++;
    }

    diff =

    diff -= droneImageUndefined;

    sumNdviWeightsPatch = (this->sumNdviWeightsDrone * bitsPerPair) - sumNdviWeightsPatch;

    result = 1.0 - ((float)sumNdviWeightsPatch / ((float)this->sumNdviWeightsDrone * (float)bitsPerPair));

    tmpExecPxCalc += (clock() - tempo) / (double)CLOCKS_PER_SEC;
    qtdChamadasPxCalc++;
    return result;
}

//Sem câmera multiespectral
double BriefHeuristic::calculateValue2(Pose3d p, cv::Mat *map)
{
    vector<int> bin;

    int bitsPerPair;
    switch (type)
    {
        case 0:
            bitsPerPair = 1;
            break;
        case 1:
            bitsPerPair = 3;
            break;
        case 2:
            bitsPerPair = 6;
            break;
        case 4:
            bitsPerPair = 2;
            break;
//    case 3:
//        bitsPerPair = 3;
//        break;
//    case 4:
//        bitsPerPair = 4;
//        break;
//    case 5:
//        bitsPerPair = 6;
//        break;
//    case 6:
//        bitsPerPair = 3;
//        break;
//    case 7:
//        bitsPerPair = 4;
//        break;
//    case 8:
//        bitsPerPair = 6;
//        break;
//    case 9:
//        bitsPerPair = 6;
//        break;
//    case 10:
//        bitsPerPair = 7;
//        break;
//    case 11:
//        bitsPerPair = 3;
//        break;
    }
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@333
    double scale = p.z;
    float theta = p.yaw*(-1)-1.5708;
    int xPad = width/2 - 0.5;
    int yPad = height/2 - 0.5;

    for(int c=0;c<pairs.size();c++)
    {
        //Projeção dos pares de pontos do descritor do drone, no "patch" do mapa.
        //Rotação e escala
        //Pixel 1
        cv::Point point1 = cv::Point(((pairs[c][0].x-xPad)*scale*cos(theta)+(pairs[c][0].y-yPad)*scale*sin(theta))+p.x,
                                    ((pairs[c][0].x-xPad)*scale*-sin(theta)+(pairs[c][0].y-yPad)*scale*cos(theta))+p.y);

        if(point1.x < 0 || point1.x >= map->cols || point1.y < 0 || point1.y >= map->rows)
        {
            bin.push_back(HEURISTIC_UNDEFINED_INT);
            bin.push_back(HEURISTIC_UNDEFINED_INT);
            bin.push_back(HEURISTIC_UNDEFINED_INT);
        }
        else
        {
            //Pixel 2
            cv::Point point2 = cv::Point(((pairs[c][1].x-xPad)*scale*cos(theta)+(pairs[c][1].y-yPad)*scale*sin(theta))+p.x,
                    ((pairs[c][1].x-xPad)*scale*-sin(theta)+(pairs[c][1].y-yPad)*scale*cos(theta))+p.y);

            if(point2.x < 0 || point2.x >= map->cols || point2.y < 0 || point2.y >= map->rows){
                bin.push_back(HEURISTIC_UNDEFINED_INT);
                bin.push_back(HEURISTIC_UNDEFINED_INT);
                bin.push_back(HEURISTIC_UNDEFINED_INT);
            }
            else{
                cv::Vec3b color1 = map->at<cv::Vec3b>(point1.y, point1.x);
                cv::Vec3b color2 = map->at<cv::Vec3b>(point2.y, point2.x);

                //Lab
                if(type == 1)
                {
                    if(color1[0]>color2[0]) bin.push_back(1);
                    else bin.push_back(0);
                    if(color1[1]>color2[1]) bin.push_back(1);
                    else bin.push_back(0);
                    if(color1[2]>color2[2]) bin.push_back(1);
                    else bin.push_back(0);
                }


                else if(type == 2)
                {
                    if(color1[0] > color2[0])
                    {
                        bin.push_back(1);
                        bin.push_back(0);
                    }else if(color1[0] < color2[0])
                    {
                        bin.push_back(0);
                        bin.push_back(1);
                    }else
                    {
                        bin.push_back(0);
                        bin.push_back(0);
                    }
                    if(color1[1] > color2[1])
                    {
                        bin.push_back(1);
                        bin.push_back(0);
                    }
                    else if(color1[1] < color2[1])
                    {
                        bin.push_back(0);
                        bin.push_back(1);
                    }else
                    {
                        bin.push_back(0);
                        bin.push_back(0);
                    }
                    if(color1[2] > color2[2])
                    {
                        bin.push_back(1);
                        bin.push_back(0);
                    }
                    else if(color1[2] < color2[2])
                    {
                        bin.push_back(0);
                        bin.push_back(1);
                    }
                    else
                    {
                        bin.push_back(0);
                        bin.push_back(0);
                    }
                }

                //GRAY
                else if(type == 0)
                {
                    float r = color1.val[0] * 0.114;//B
                    float g = color1.val[1] * 0.587;//G
                    float b = color1.val[2] * 0.299;//R

                    int sum1 = r + g + b;

                    r = color2.val[0] * 0.114;//B
                    g = color2.val[1] * 0.587;//G
                    b = color2.val[2] * 0.299;//R

                    int sum2 = r + g + b;

                    if(sum1 > sum2) bin.push_back(1);
                    else bin.push_back(0);
                }

                else if(type == 3)
                {
                    int diff = (color1[0]-color2[0]);
                    bin.push_back(diff);
                    diff = (color1[1]-color2[1]);
                    bin.push_back(diff);
                    diff = (color1[2]-color2[2]);
                    bin.push_back(diff);
                }
                //LAB WITH OUT L
                else if(type == 4)
                {
                    if(color1[0]>color2[0]) bin.push_back(1);
                    else bin.push_back(0);
                    if(color1[1]>color2[1]) bin.push_back(1);
                    else bin.push_back(0);
//                    if(color1[2]>color2[2]) bin.push_back(1);
//                    else bin.push_back(0);
        //            cout<<"R - "<<color1[0]<<endl;
        //            cout<<"G - "<<color1[1]<<endl;
        //            cout<<"B - "<<color1[2]<<endl;
                }

//                //RGB without Equals
//                if(type == 3 || type == 4 || type == 5 || type == 8){
//                    if(color1[0]>color2[0]) bin.push_back(1);
//                    else if(color1[0]<color2[0]) bin.push_back(0);
//                    else bin.push_back(HEURISTIC_UNDEFINED_INT);
//                    if(color1[1]>color2[1]) bin.push_back(1);
//                    else if(color1[0]<color2[0]) bin.push_back(0);
//                    else bin.push_back(HEURISTIC_UNDEFINED_INT);
//                    if(color1[2]>color2[2]) bin.push_back(1);
//                    else if(color1[0]<color2[0]) bin.push_back(0);
//                    else bin.push_back(HEURISTIC_UNDEFINED_INT);
//                }

//                //RGB normalized
//                if(type == 6 || type == 7 || type == 8 || type == 9 || type == 10){
//                    float sum = color1[0] + color1[1] + color1[2];
//                    float c1r = color1.val[0]/sum;
//                    float c1g = color1.val[1]/sum;
//                    float c1b = color1.val[2]/sum;

//                    sum = color2[0] + color2[1] + color2[2];
//                    float c2r = color2.val[0]/sum;
//                    float c2g = color2.val[1]/sum;
//                    float c2b = color2.val[2]/sum;

//                    if(c1r>c2r) bin.push_back(1);
//                    else bin.push_back(0);
//                    if(c1g>c2g) bin.push_back(1);
//                    else bin.push_back(0);
//                    if(c1b>c2b) bin.push_back(1);
//                    else bin.push_back(0);
//                }

//                //Lab
//                if(type == 11){
//                    cv::Mat rgb(1, 2, CV_8UC3);
//                    rgb.at<cv::Vec3b>(0, 0) = color1;
//                    rgb.at<cv::Vec3b>(0, 1) = color2;
//                    cv::Mat lab;
//                    cv::cvtColor(rgb, lab, CV_RGB2Lab);

//                    if (lab.at<cv::Vec3b>(0,0).val[0] > lab.at<cv::Vec3b>(0,1).val[0]) bin.push_back(1);
//                    else bin.push_back(0);

//                    if (lab.at<cv::Vec3b>(0,0).val[1] > lab.at<cv::Vec3b>(0,1).val[1]) bin.push_back(1);
//                    else bin.push_back(0);

//                    if (lab.at<cv::Vec3b>(0,0).val[2] > lab.at<cv::Vec3b>(0,1).val[2]) bin.push_back(1);
//                    else bin.push_back(0);
//                }

            }
        }
    }
    //abBRIEF cálculo da diferença entre o descritor do drone e o descritor do "patch"
    if(type == 3)
    {
        int diff = 0;
        double prob = 1;
        float saturation = 25;
        float varProb = 10;
        for(int i= 0; i < droneDescriptor.size(); i++)
        {
            if(droneDescriptor[i] == HEURISTIC_UNDEFINED_INT || bin[i] == HEURISTIC_UNDEFINED_INT)
                diff+=saturation;
            else
            {
                int diff = abs(droneDescriptor[i]-bin[i]);
                if(diff>saturation) diff=saturation;
            }

            double auxD = /*(1.0/(sqrt(varProb*2.0*M_PI))**/exp(-0.5*(pow(diff,2.0)/varProb));
            //if(auxD<0.9) auxD = 0.9;
            prob *= auxD;
        }

        return prob;
        //return 1.0-(diff/(totalPairs*saturation)); comentei p não afz sentido 2 returns assim...
    }
    //BRIEF cálculo da diferença entre o descritor do drone e o descritor do "patch"
    else
    {
        int diff = 0;
        int droneImageUndefined = 0;
        double result = 0;
        for(int i= 0; i < droneDescriptor.size(); i++){
            if(droneDescriptor[i] == HEURISTIC_UNDEFINED_INT || bin[i] == HEURISTIC_UNDEFINED_INT || bin[i] != droneDescriptor[i]) diff++;
            if(droneDescriptor[i] == HEURISTIC_UNDEFINED_INT) droneImageUndefined++;
        }

        diff -= droneImageUndefined;
        if(color_difference != INTENSITYC)
            result = 1.0 - ((float)((totalPairs*bitsPerPair)-diff)/(float)(totalPairs*bitsPerPair));
            //return 1.0 - ((float)((totalPairs*bitsPerPair)-diff)/(float)(totalPairs*bitsPerPair));
        else
            result = 1.0 - ((float)((totalPairs)-diff)/(float)(totalPairs));
            //return 1.0 - ((float)((totalPairs)-diff)/(float)(totalPairs));
        return result;
    }
}

cv::Point BriefHeuristic::transform(cv::Point pt, cv::Mat rot, cv::Point trans, int max_x, int max_y)
{
    cv::Mat res(1,2,CV_64F);

    res.at<double>(0,0)=pt.x;
    res.at<double>(0,1)=pt.y;

    cv::Mat dst = res*rot;

    cv::Point point = cv::Point(dst.at<double>(0,0)+trans.x,dst.at<double>(0,1)+trans.y);

    if(point.x < 0) point.x = 0;
    else if(point.x > max_x) point.x = max_x;
    if(point.y < 0) point.y = 0;
    else if(point.y > max_y) point.y = max_y;

    return point;
}

double BriefHeuristic::calculateValue(int x, int y, cv::Mat *image, cv::Mat* map){
    return 0;
}

bool BriefHeuristic::pointIn(cv::Point points, cv::Mat image){
    if(points.x < image.cols && points.x >= 0 && points.y < image.rows && points.y >= 0)
        return true;
    return false;
}

void BriefHeuristic::visualizaCameraPoseOriCorr(Pose3d pose, Pose3d orientacao, bool invYaw)//Pose da partícula ou drone, e a orientação utilizada para calcular a nova imagem.
{
    cv::Mat patchOriginal = cv::Mat::zeros(cv::Size(width, height), CV_8UC3);
    cv::Mat patchDeslocado = cv::Mat::zeros(cv::Size(width, height), CV_8UC3);
    cv::Mat patchDeslocadoOK = cv::Mat::zeros(cv::Size(width, height), CV_8UC3);
    float theta = pose.yaw*(-1)-1.5708;//-1.5708 = -90 graus
    int xPad = width/2 - 0.5;
    int yPad = height/2 - 0.5;
    double cosPitch = cos(orientacao.pitch);
    double sinPitch = sin(orientacao.pitch);
    double cosYaw = 0;
    double sinYaw = 0;
    double cosRoll = cos(orientacao.roll);
    double sinRoll = sin(orientacao.roll);
    float cosTheta = cos(theta);
    float sinTheta = sin(theta);
    int xCorr = 0;
    int yCorr = 0;
    int zCorr = 0;
    float xCorrOri = 0;
    float yCorrOri = 0;
    float zCorrOri = 0;
    int xOri = 0;
    int yOri = 0;
    double dirExib[4] = {0, 0, (-pose.z /0.366197183) * (multExcalaZ), 0};

    //Invertendo o yaw, a projeçãos erá inversa
    if(invYaw)
    {
        cosYaw = cos(orientacao.yawInvertido);
        sinYaw = sin(orientacao.yawInvertido);
    }
    else
    {
        cosYaw = cos(orientacao.yaw);
        sinYaw = sin(orientacao.yaw);
    }

    //Extrair o patch original, antes de deslocar os pares de pixeis
    //Utils::MatType(*map);
    for(int i=0; i<height; i++)
    {
        for(int j=0; j<width; j++)
        {
            xOri = ((j-xPad)*pose.z*cosTheta+(i-yPad)*pose.z*sinTheta)+pose.x;
            yOri = ((j-xPad)*pose.z*-sinTheta+(i-yPad)*pose.z*cosTheta)+pose.y;

            if(xOri < 4800 && yOri < 4800 && xOri > 0 && yOri > 0)
                patchOriginal.at<cv::Vec3b>(i,j) = originalMap.at<cv::Vec3b>(yOri, xOri);
        }
    }

    resize(patchOriginal,patchOriginal,cv::Size(0,0),0.25,0.25);
    imshow("patchOriginal (GT)", patchOriginal);

    //Patch corrigido com roll e pitch
    /*xCorrOri = pose.x    + (dirExib[0]*(cosPitch*cosYaw)       + dirExib[1]*(sinRoll*sinPitch*cosYaw-cosRoll*sinYaw)       + dirExib[2]*(cosRoll*sinPitch*cosYaw+sinRoll*sinYaw));
    yCorrOri = pose.y    + (dirExib[0]*(cosPitch*sinYaw)       + dirExib[1]*(sinRoll*sinPitch*sinYaw+cosRoll*cosYaw)       + dirExib[2]*(cosRoll*sinPitch*sinYaw-sinRoll*cosYaw));
    zCorrOri = pose.z    + (dirExib[0]*-sin(orientacao.pitch)  + dirExib[1]*(sin(orientacao.roll)*cos(orientacao.pitch))   + dirExib[2]*(cos(orientacao.roll)*cos(orientacao.pitch))) * 0,366197183 / 45*0,188679245;

    //Utils::MatType(*map);
    for(int i=0; i<height; i++)
    {
        for(int j=0; j<width; j++)
        {
            xCorr = ((j-xPad)*pose.z*cosTheta+(i-yPad)*pose.z*sinTheta)+xCorrOri;
            yCorr = ((j-xPad)*pose.z*-sinTheta+(i-yPad)*pose.z*cosTheta)+yCorrOri;

            if(xCorr < 4800 && yCorr < 4800 && xCorr > 0 && yCorr > 0)
                patchDeslocado.at<cv::Vec3b>(i,j) = originalMap.at<cv::Vec3b>(yCorr, xCorr);
        }
    }
    resize(patchDeslocado,patchDeslocado,cv::Size(0,0),0.25,0.25);
    imshow("patchDeslocado (GT corrigido apenas px centro)", patchDeslocado);*/

    for(int i=0; i<height; i++)
    {
        for(int j=0; j<width; j++)
        {
            //Projeta no mapa
            xOri = ((j-xPad)*pose.z*cosTheta+(i-yPad)*pose.z*sinTheta)+pose.x;
            yOri = ((j-xPad)*pose.z*-sinTheta+(i-yPad)*pose.z*cosTheta)+pose.y;

            //Rotaciona
            xCorrOri = xOri    + (dirExib[0]*(cosPitch*cosYaw)       + dirExib[1]*(sinRoll*sinPitch*cosYaw-cosRoll*sinYaw)       + dirExib[2]*(cosRoll*sinPitch*cosYaw+sinRoll*sinYaw));
            yCorrOri = yOri    + (dirExib[0]*(cosPitch*sinYaw)       + dirExib[1]*(sinRoll*sinPitch*sinYaw+cosRoll*cosYaw)       + dirExib[2]*(cosRoll*sinPitch*sinYaw-sinRoll*cosYaw));
            zCorrOri = pose.z    + (dirExib[0]*-sin(orientacao.pitch)  + dirExib[1]*(sin(orientacao.roll)*cos(orientacao.pitch))   + dirExib[2]*(cos(orientacao.roll)*cos(orientacao.pitch))) * 0,366197183 / 45*0,188679245;


            if(xCorrOri < 4800 && yCorrOri < 4800 && xCorrOri > 0 && yCorrOri > 0)
                patchDeslocadoOK.at<cv::Vec3b>(i,j) = originalMap.at<cv::Vec3b>(yCorrOri, xCorrOri);
        }
    }

    resize(patchDeslocadoOK,patchDeslocadoOK,cv::Size(0,0),0.25,0.25);
    imshow("patchDeslocado (GT corrigido)", patchDeslocadoOK);
}

/*Método que recebe a pose referente ao centro do patch, e estrai o patch sendo visto pelo drone*/
void BriefHeuristic::visualizaCameraPose(Pose3d pose, Pose3d orientacao, bool GT)//Pose da partícula ou drone, e a orientação utilizada para calcular a nova imagem.
{
    cv::Mat patchOriginal = cv::Mat::zeros(cv::Size(width, height), CV_8UC3);
    cv::Mat patchDeslocado = cv::Mat::zeros(cv::Size(width, height), CV_8UC3);
    cv::Mat patchDeslocadoOK = cv::Mat::zeros(cv::Size(width, height), CV_8UC3);
    float theta = pose.yaw*(-1)-1.5708;//-1.5708 = -90 graus
    int xPad = width/2 - 0.5;
    int yPad = height/2 - 0.5;
    float cosTheta = cos(theta);
    float sinTheta = sin(theta);
    int xOri = 0;
    int yOri = 0;
    float xCorrOri = 0;
    float yCorrOri = 0;
    float zCorrOri = 0;
    double dirExib[4] = {0, 0, (-pose.z /0.366197183) * (multExcalaZ), 0};
    double cosPitch=0;
    double sinPitch=0;
    double cosYaw=0;
    double sinYaw=0;
    double cosRoll=0;
    double sinRoll=0;    

//    cosPitch = cos(orientacao.pitch);
//    sinPitch = sin(orientacao.pitch);
//    cosYaw = cos(orientacao.yaw);
//    sinYaw = sin(orientacao.yaw);
//    cosRoll = cos(orientacao.roll);
//    sinRoll = sin(orientacao.roll);

    //testes
    cosPitch = cos(DEG2RAD(-35)); //rad
    sinPitch = sin(DEG2RAD(-35)); //rad
    cosYaw   = cos(orientacao.yaw+DEG2RAD(0)); //rad
    sinYaw   = sin(orientacao.yaw+DEG2RAD(0)); //rad
    cosRoll  = cos(DEG2RAD(0)); //rad
    sinRoll  = sin(DEG2RAD(0)); //rad

    //Nenhuma inclinação, olhando diretamente p baixo (GT Corrigido)
    int x0 = 0, y0 = 0, x1 = 0, y1 = 0, x2 = 0, y2 = 0, x3 = 0, y3 = 0;
    //0: esquerda cima, 1: direita cima, 2: esquerda baixo e 3: dirwita baixo.

    if(!GT)
    {
        //Extrair o patch original
        for(int i=0; i<height; i++)
        {
            for(int j=0; j<width; j++)
            {
                xOri = ((j-xPad)*pose.z*cosTheta+(i-yPad)*pose.z*sinTheta)+pose.x;
                yOri = ((j-xPad)*pose.z*-sinTheta+(i-yPad)*pose.z*cosTheta)+pose.y;

                if(i == 0 && j == 0)
                {
                    x0 = xOri;
                    y0 = yOri;
                }
                else if(i == 0 && j == width-1)
                {
                    x1 = xOri;
                    y1 = yOri;
                }
                else if(i == height-1 && j == 0)
                {
                    x2 = xOri;
                    y2 = yOri;
                }
                else if(i == height-1 && j == width-1)
                {
                    x3 = xOri;
                    y3 = yOri;
                }

                //Projection
                //xOri = (xOri * FocalLength)/pose.z;
                //yOri = (yOri * FocalLength)/pose.z;                

                if(xOri < 4800 && yOri < 4800 && xOri > 0 && yOri > 0)
                    patchOriginal.at<cv::Vec3b>(i,j) = originalMap.at<cv::Vec3b>(yOri, xOri);
            }
        }
        cout << "\n\ !GT nWidth: " << width << " Height: " << height << " X0: " << x0 << " Y0: " << y0 << " X1: " << x1 << " Y1: " << y1 <<
                " X2: " << x2 << " Y2: " << y2 << " X3: " << x3 << " Y3: " << y3 << "\n\n";

        if(!patchDeslocadoOK.empty())
        {
            //cv::imwrite("imgs/" + std::to_string(currentImage)+"_GTcorrigidoReto.jpg", patchOriginal);
            //perspectiveProjection(pose.z, patchOriginal, orientacao.roll, orientacao.pitch, orientacao.yaw, width, height);
            resize(patchOriginal, patchOriginal, cv::Size(0,0),0.25,0.25);
            imshow("patchOriginal (GT corrigido) linha reta para o chão", patchOriginal);
        }
    }
    else//GT original, olhando para a direção do GT corrigido. Algo semelhante ao mapa local que a câmera está capturando, se manteno parecido quando há grande inclinação.
    {


        for(int i=0; i<height; i++)
        {
            for(int j=0; j<width; j++)
            {
                //Projeta no mapa
                xOri = ((j-xPad)*pose.z*cosTheta+(i-yPad)*pose.z*sinTheta)+pose.x;
                yOri = ((j-xPad)*pose.z*-sinTheta+(i-yPad)*pose.z*cosTheta)+pose.y;

                //Rotaciona
                xCorrOri = xOri    + (dirExib[0]*(cosPitch*cosYaw)       + dirExib[1]*(sinRoll*sinPitch*cosYaw-cosRoll*sinYaw)       + dirExib[2]*(cosRoll*sinPitch*cosYaw+sinRoll*sinYaw));
                yCorrOri = yOri    + (dirExib[0]*(cosPitch*sinYaw)       + dirExib[1]*(sinRoll*sinPitch*sinYaw+cosRoll*cosYaw)       + dirExib[2]*(cosRoll*sinPitch*sinYaw-sinRoll*cosYaw));
                zCorrOri = pose.z    + (dirExib[0]*-sin(orientacao.pitch)  + dirExib[1]*(sin(orientacao.roll)*cos(orientacao.pitch))   + dirExib[2]*(cos(orientacao.roll)*cos(orientacao.pitch))) * 0,366197183 / 45*0,188679245;

                if(xCorrOri < 4800 && yCorrOri < 4800 && xCorrOri > 0 && yCorrOri > 0)
                    patchDeslocadoOK.at<cv::Vec3b>(i,j) = originalMap.at<cv::Vec3b>(yCorrOri, xCorrOri);
            }
        }

        if(!patchDeslocadoOK.empty())
        {
            //cv::imwrite("imgs/" + std::to_string(currentImage)+"_GToriginalInclinado.jpg", patchDeslocadoOK);
            resize(patchDeslocadoOK,patchDeslocadoOK,cv::Size(0,0),0.25,0.25);
            imshow("patchDeslocado (GT original) inclinado", patchDeslocadoOK);
        }
    }
}

//perspectiveProjection(pose.z, patchOriginal, orientacao.roll, orientacao.pitch, orientacao.yaw, width, height )
void BriefHeuristic::perspectiveProjection(double z, cv::Mat source, double roll, double pitch, double yaw, int width, int height)
{
    /*Projection Matrix
    A = [f 0 0 0
         0 f 0 0
         0 0 1 0]
    */

    //Parrot Sequoia
    //https://support.micasense.com/hc/en-us/articles/218543928-What-is-the-field-of-view-for-Sequoia-s-lenses-
    //    RGB Imager (16 MP):
    //    - Focal Length: 4.9 mm
    //    - HFOV: 63.9°
    //    - VFOV: 50.1°
    //    - DFOV: 73.5°


    //    Monochrome Imagers:
    //    - Focal Length: 4.0 mm
    //    - HFOV: 61.9°
    //    - VFOV: 48.5°
    //    - DFOV: 73.7°
    float FocalLength = 3.979999908;   // mm - along X and Y axis (multispectral)
    //-

    cv::Mat transfo;
    cv::Mat destination;
    double f, dist;
    //dist = (-z /0.366197183) * (multExcalaZ);
    dist = z;
    f = FocalLength;
//    double alpha, beta, gamma;
//    alpha = ((double)alpha_ - 90.)*PI/180;
//    beta = ((double)beta_ - 90.)*PI/180;
//    gamma = ((double)gamma_ - 90.)*PI/180;
    //f = (double) f_;
    //dist = (double) dist_;

    cv::Size taille = cv::Size(width, height);
    //double w = (double)taille.width, h = (double)taille.height;

    // Projection 2D -> 3D matrix
    cv::Mat A1 = (cv::Mat_<double>(4,3) <<
        1, 0, -width/2,
        0, 1, -height/2,
        0, 0,    0,
        0, 0,    1);
    /*
    %Rotação em yaw (z fixo)
    rotYaw = [  cos(yaw) 	sin(yaw) 	0   0
                -sin(yaw)	cos(yaw) 	0   0
                 0          0           1   0
                 0          0           0   1];

    %Rotação em pitch (y fixo)
    rotPitch = [cos(pitch)	0	-sin(pitch) 0
                0           1	0           0
                sin(pitch)  0	cos(pitch)  0
                0           0   0           1];

    %Rotação em roll (x fixo)
    rotRoll = [ 1	0           0           0
                0	cos(roll)	sin(roll)   0
                0	-sin(roll)	cos(roll)   0
                0   0           0           1];  */


    // Rotation matrices around the X,Y,Z axis (ROLL)
    cv::Mat RX = (cv::Mat_<double>(4, 4) <<
        1,          0,           0, 0,
        0, cos(roll), -sin(roll), 0,
        0, sin(roll),  cos(roll), 0,
        0,          0,           0, 1);

    //PITCH
    cv::Mat RY = (cv::Mat_<double>(4, 4) <<
        cos(pitch), 0, -sin(pitch), 0,
                0, 1,          0, 0,
        sin(pitch), 0,  cos(pitch), 0,
                0, 0,          0, 1);

    //YAW
    cv::Mat RZ = (cv::Mat_<double>(4, 4) <<
        cos(yaw), -sin(yaw), 0, 0,
        sin(yaw),  cos(yaw), 0, 0,
        0,          0,           1, 0,
        0,          0,           0, 1);

    // Composed rotation matrix with (RX,RY,RZ)
    cv::Mat R = RX * RY * RZ;

    // Translation matrix on the Z axis change dist will change the height
    cv::Mat T = (cv::Mat_<double>(4, 4) <<
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, dist,
        0, 0, 0, 1);

    // Camera Intrisecs matrix 3D -> 2D
    cv::Mat A2 = (cv::Mat_<double>(3,4) <<
        f, 0, width/2, 0,
        0, f, height/2, 0,
        0, 0,   1, 0);

    // Final and overall transformation matrix
    transfo = A2 * (T * (R * A1));

    for(int i = 0; i < transfo.rows; i++)
    {
        for(int j = 0; j < transfo.cols; j++)
        {
            cout<<transfo.row(i).col(j)<<" | ";
        }
        cout<<"\n";
    }


    // Apply matrix transformation
    warpPerspective(source, destination, transfo, taille, cv::INTER_CUBIC | cv::WARP_INVERSE_MAP);
    resize(destination, destination, cv::Size(0,0),0.25,0.25);
    imshow("perspective", destination);
}


//Cálculo do abBRIEF para uma partícula (usado apenas em alguns testes para a comparação entre o descritor de diferentes ângulos)
/*
roolSign: 0 para nenhuma inclinaçãoc (GT olhando p baixo), positivo para um dos lados e negativo para o outro.
pichSign: 0 para nenhuma inclinaçãoc (GT olhando p baixo), positivo para frente e negativo p trás, ou o contrário, não lembro...
yawSign: mesma coisa, só que para o giro do drone, o loocal para onde o pico está apontando horizontalmene.
GT aponta para baixo, senão será o GT corrigido, o qual apresenta uma orientação diferente, com inclinação e tal...
Enviando 0 para roolSign e pichSign, será considerada a execução natural (GT).
Esse método existe pois preciso provar que o abBRIEF de uma partícula olhando para baixo e de suas vizinhas olhando para a mesma posição, possuem valores que não são estatísticamente significantes.
*/
double BriefHeuristic::abBriefParicle(Pose3d dronePose, cv::Mat *map, cv::Mat &redMap, cv::Mat &nirMap, cv::Mat &regMap, cv::Mat &greMap, cv::Mat &currentMapOriginal, Pose3d orientacao, bool GT)
{
    vector<int> bin;

    int bitsPerPair;
    switch (type)
    {
        case 0:
            bitsPerPair = 1;
            break;
        case 1:
            bitsPerPair = 3;
            break;
        case 2:
            bitsPerPair = 6;
            break;
        case 4:
            bitsPerPair = 2;
            break;
    }

    double scale = dronePose.z;
    float theta = dronePose.yaw*(-1)-1.5708;//-1.5708 = -90 graus
    int xPad = width/2 - 0.5;
    int yPad = height/2 - 0.5;

    double dir[4];//Direcao inicial (Apontando p baixo)[x y z 0], e localizado na origem
    double cosPitch;
    double sinPitch;
    double cosYaw;
    double sinYaw;
    double cosRoll;
    double sinRoll;
    double particleYaw = dronePose.yaw*(-1)-1.5708;//-1.5708 = -90 graus
    if(GT)
    {
        dir[0] = {0};
        dir[1] = 0;
        dir[2] = (-scale /0.366197183) * (multExcalaZ);
        dir[3] = 0;
        //Orientações do GT
        cosPitch = cos(orientacao.pitch); //rad
        sinPitch = sin(orientacao.pitch); //rad
        cosYaw   = cos(orientacao.yaw); //rad
        sinYaw   = sin(orientacao.yaw); //rad
        cosRoll  = cos(orientacao.roll); //rad
        sinRoll  = sin(orientacao.roll); //rad

    }
    float cosTheta = cos(theta);
    float sinTheta = sin(theta);

    for(int c=0; c<pairs.size(); c++)
    {
        //Projeção dos pares de pontos do descritor do drone, no "patch" do mapa.
        //Rotação e escala
        //Pixel 1 (projeta o ponto do par no chão - seria o que a partícula vê com roll e pitch nulos, apenas com o yaw)
        cv::Point point1 = cv::Point(((pairs[c][0].x-xPad)*scale*cosTheta+(pairs[c][0].y-yPad)*scale*sinTheta)+dronePose.x,
                                    ((pairs[c][0].x-xPad)*scale*-sinTheta+(pairs[c][0].y-yPad)*scale*cosTheta)+dronePose.y);

        if(GT)
        {
            //Rotacionar em roll, pitch e yaw (mover esse ponto que a partícula estaria vendo, com base na orientação do drone)
            point1.x += dir[0]*(cosPitch*cosYaw) + dir[1]*(sinRoll*sinPitch*cosYaw-cosRoll*sinYaw) + dir[2]*(cosRoll*sinPitch*cosYaw+sinRoll*sinYaw);
            point1.y += dir[0]*(cosPitch*sinYaw) + dir[1]*(sinRoll*sinPitch*sinYaw+cosRoll*cosYaw) + dir[2]*(cosRoll*sinPitch*sinYaw-sinRoll*cosYaw);
            //z = ((dir[0]*-sinPitch + dir[1]*(sinRoll*cosPitch) + dir[2]*(cosRoll*cosPitch))) * 0,366197183 / 45*0,188679245;
        }

        if(point1.x < 0 || point1.x >= map->cols || point1.y < 0 || point1.y >= map->rows)
        {
            bin.push_back(HEURISTIC_UNDEFINED_INT);
            bin.push_back(HEURISTIC_UNDEFINED_INT);
            bin.push_back(HEURISTIC_UNDEFINED_INT);
        }
        else
        {
            //Pixel 2
            cv::Point point2 = cv::Point(((pairs[c][1].x-xPad)*scale*cosTheta+(pairs[c][1].y-yPad)*scale*sinTheta)+dronePose.x,
                    ((pairs[c][1].x-xPad)*scale*-sinTheta+(pairs[c][1].y-yPad)*scale*cosTheta)+dronePose.y);

            if(GT)
            {
                //Rotacionar em roll, pitch e yaw
                point2.x += dir[0]*(cosPitch*cosYaw) + dir[1]*(sinRoll*sinPitch*cosYaw-cosRoll*sinYaw) + dir[2]*(cosRoll*sinPitch*cosYaw+sinRoll*sinYaw);
                point2.y += dir[0]*(cosPitch*sinYaw) + dir[1]*(sinRoll*sinPitch*sinYaw+cosRoll*cosYaw) + dir[2]*(cosRoll*sinPitch*sinYaw-sinRoll*cosYaw);
                //já foi calulado, o mesmo para as os 2 pixeis   //scale += ((dir[0]*-sinPitch + dir[1]*(sinRoll*cosPitch) + dir[2]*(cosRoll*cosPitch))) * 0,366197183 / 45*0,188679245;
            }

            if(point2.x < 0 || point2.x >= map->cols || point2.y < 0 || point2.y >= map->rows){
                bin.push_back(HEURISTIC_UNDEFINED_INT);
                bin.push_back(HEURISTIC_UNDEFINED_INT);
                bin.push_back(HEURISTIC_UNDEFINED_INT);
            }
            else
            {
                cv::Vec3b color1 = map->at<cv::Vec3b>(point1.y, point1.x);
                cv::Vec3b color2 = map->at<cv::Vec3b>(point2.y, point2.x);

                //Lab
                if(type == 1)
                {
                    if(color1[0]>color2[0]) bin.push_back(1);
                    else bin.push_back(0);
                    if(color1[1]>color2[1]) bin.push_back(1);
                    else bin.push_back(0);
                    if(color1[2]>color2[2]) bin.push_back(1);
                    else bin.push_back(0);
                }

                else if(type == 2)
                {
                    if(color1[0] > color2[0]){
                        bin.push_back(1);
                        bin.push_back(0);
                    }else if(color1[0] < color2[0]){
                        bin.push_back(0);
                        bin.push_back(1);
                    }else{
                        bin.push_back(0);
                        bin.push_back(0);
                    }
                    if(color1[1] > color2[1]){
                        bin.push_back(1);
                        bin.push_back(0);
                    }else if(color1[1] < color2[1]){
                        bin.push_back(0);
                        bin.push_back(1);
                    }else{
                        bin.push_back(0);
                        bin.push_back(0);
                    }
                    if(color1[2] > color2[2]){
                        bin.push_back(1);
                        bin.push_back(0);
                    }else if(color1[2] < color2[2]){
                        bin.push_back(0);
                        bin.push_back(1);
                    }else{
                        bin.push_back(0);
                        bin.push_back(0);
                    }
                }

                //GRAY
                else if(type == 0)
                {
                    float r = color1.val[0] * 0.114;//B
                    float g = color1.val[1] * 0.587;//G
                    float b = color1.val[2] * 0.299;//R

                    int sum1 = r + g + b;

                    r = color2.val[0] * 0.114;//B
                    g = color2.val[1] * 0.587;//G
                    b = color2.val[2] * 0.299;//R

                    int sum2 = r + g + b;

                    if(sum1 > sum2) bin.push_back(1);
                    else bin.push_back(0);
                }

                else if(type == 3)
                {
                    int diff = (color1[0]-color2[0]);
                    bin.push_back(diff);
                    diff = (color1[1]-color2[1]);
                    bin.push_back(diff);
                    diff = (color1[2]-color2[2]);
                    bin.push_back(diff);
                }
                //LAB WITH OUT L
                else if(type == 4)
                {
                    if(color1[0]>color2[0]) bin.push_back(1);
                    else bin.push_back(0);
                    if(color1[1]>color2[1]) bin.push_back(1);
                    else bin.push_back(0);
                }
            }
        }
    }

    if(!quiet)
        visualizaCameraPose(dronePose, orientacao, GT);

    //BRIEF cálculo da diferença entre o descritor do drone e o descritor do "patch"
    double result = 0;
    double sumNdviWeightsPatch = 0;
    int pairCount = 0;
    int droneImageUndefined = 0;

    int diff = 0;

    for(int i = 0; i < droneDescriptor.size(); i++)
    {
        if(droneDescriptor[i] == HEURISTIC_UNDEFINED_INT
                || bin[i] == HEURISTIC_UNDEFINED_INT
                || bin[i] != droneDescriptor[i])
        {
            diff++;
            sumNdviWeightsPatch += pairWeight[pairCount];
        }
        if(droneDescriptor[i] == HEURISTIC_UNDEFINED_INT)
        {
            droneImageUndefined++;
            sumNdviWeightsPatch -= pairWeight[pairCount];
        }
        if(i % 2 != 0)
            pairCount++;
    }
    //sumNdviWeightsPatch é o somatório dos pesos do NDVI quando não ocorre match com o mapa local, ou seja, a diferença
    //(this->sumNdviWeightsDrone * bitsPerPair) é o peso máximo a ser obtido, equivalente à um match perfeito
    diff -= droneImageUndefined;

    sumNdviWeightsPatch = (this->sumNdviWeightsDrone * bitsPerPair) - sumNdviWeightsPatch;

    result = 1.0 - ((float)sumNdviWeightsPatch / ((float)this->sumNdviWeightsDrone * (float)bitsPerPair));

    return result;
}

/*
Esse método existe pois preciso provar que o abBRIEF de uma partícula olhando para baixo e de suas vizinhas olhando para a mesma posição, possuem valores que não são estatísticamente significantes.
Porém ele apenas chama  método do cálculo, para cada partícula, e calcula a probabilidade final.
*/
void BriefHeuristic::abBriefParicleProb(Pose3d GTpose, Pose3d correctedGTpose, cv::Mat *map, cv::Mat &redMap, cv::Mat &nirMap, cv::Mat &regMap, cv::Mat &greMap, cv::Mat &currentMapOriginal, Pose3d orientacao)
{
    double GTbrief = 0;
    double correctedGTbrief = 0;
    double GTbriefProb = 1.0;
    double correctedGTbriefProb = 1.0;
    double varBrief = pow(.15, 2.0);

    //Calcular o brief para o GT
    GTbrief = this->abBriefParicle(GTpose, map, redMap, nirMap, regMap, greMap, currentMapOriginal, orientacao, true);
    //Calcular o brief para uma partícula no GT corrigido
    correctedGTbrief = this->abBriefParicle(correctedGTpose, map, redMap, nirMap, regMap, greMap, currentMapOriginal, orientacao, false);//preciso alterar para passa o GT corrigido aqui

    cout<<"\ncurrentImage: "<< currentImage << " Image: "<<currentImage + imgIniExec + 1<<"  GTbrief: "<< GTbrief << " correctedGTbrief: " << correctedGTbrief;

    //abBRIEFLog<<"\ncurrentImage: "<< currentImage << " Image: "<<currentImage + imgIniExec + 1<<"  GTbrief: "<< GTbrief << " correctedGTbrief: " << correctedGTbrief;

    ////ofstream briefLog;

    //if(!briefLog.is_open())
        ////briefLog.open ("abBRIEF_values_GT_CorrectedGT.txt", std::ios_base::app);
    //if(currentImage == 1)
    //{
        //briefLog<<"Nova execucao========================================================================================================";
        //file.clear();
    //}
    ////briefLog << "\nImage: "<<currentImage + imgIniExec + 1<<"  GTbrief: "<< GTbrief << " correctedGTbrief: " << correctedGTbrief;
    //if(currentImage == (imgFimExec - imgIniExec-1))
    ////if(briefLog.is_open())
        ////briefLog.close();

    if(currentImage == 1)
        abBRIEFLog<<"currentImage| Image| GTbrief| correctedGTbrief| GTbriefProb| correctedGTbriefProb\n";

    GTbriefProb *= (1.0/(sqrt(varBrief*2.0*M_PI))*exp(-0.5*(pow(GTbrief,2.0)/varBrief)));
    correctedGTbriefProb *= (1.0/(sqrt(varBrief*2.0*M_PI))*exp(-0.5*(pow(correctedGTbrief,2.0)/varBrief)));

    abBRIEFLog << currentImage + imgIniExec + 1 << "        " << GTbrief << "       " << correctedGTbrief << "      " << GTbriefProb << "       " << correctedGTbriefProb << "\n";

    if(currentImage == (imgFimExec - imgIniExec+1))
    {
        ofstream briefLog;
        float rnd = Utils::RandomFloat(1, 10);
        briefLog.open("abBRIEF_values_GT_CorrectedGT" + std::to_string(rnd) + ".txt", std::ios_base::app);
        briefLog << abBRIEFLog.rdbuf();
        briefLog.close();
    }
}
