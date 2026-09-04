#include <random>
#include <chrono>
#include <limits>
#include "Mcl.h"
#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <set>
#include <GL/glut.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/video.hpp>
#include "DroneRobot.h"
#include "ColorCPU.h"
#include "RadiusVolumeTransferFunctions.h"
#include "opencv2/highgui/highgui.hpp"
#include <math.h>
#include "config.h"


//////////////////////
// Métodos Públicos //
//////////////////////

/// // TRAJ4 -s color diff-intensity 36  -s density diff-intensity 35.640406 circular 10 -s density diff-cie2000 13.889990 inverted 10
/// // TRAJ2 -s color diff-cie2000 16    -s density diff-intensity 35.640406 circular 10 -s density diff-cie2000 13.889990 inverted 10


MCL::MCL(vector<Heuristic*> &hVector, vector<MapGrid *> &cMaps, vector<cv::Mat> &gMaps, Pose3d &initial, string &lName, int &nParticles, vector<Pose3d>droneAngles, vector<Pose3d>CorrectedGT):
    heuristics(hVector),
    heuristicValues(heuristics.size(),0.0),
    heuristicGradients(heuristics.size(),0.0),
    frameColorConverted(3),
    binaryFrameMask(),
    cachedMaps(cMaps),
    globalMaps(gMaps)
{
    this->sumW = 0;
    this->droneAngles = droneAngles;
    this->CorrectedGT = CorrectedGT;
    numParticles = nParticles;
    cout << "PARTICLES:" << numParticles<< endl;
    resamplingThreshold = numParticles/8;
    lastOdometry.x=0.0;
    lastOdometry.y=0.0;
    lastOdometry.yaw=0.0;
    starting=false;
    particles.resize(numParticles);
    realPose = initial;
    odomPose = initial;
    minscale = minScale;
    maxscale = maxScale;
//    odomPose.theta = 0;

    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();

    if(fixedRandomSeed)
        seed = 445473462;

    std::default_random_engine generator (seed);

//    std::uniform_real_distribution<double> randomX(0.25*globalMaps[0].cols,0.75*globalMaps[0].cols);
//    std::uniform_real_distribution<double> randomY(0.25*globalMaps[0].cols,0.75*globalMaps[0].cols);
    std::uniform_real_distribution<double> randomX(0.0,globalMaps[0].cols);
    std::uniform_real_distribution<double> randomY(0.0,globalMaps[0].rows);
    std::uniform_real_distribution<double> randomTh(-M_PI,M_PI);
    std::uniform_real_distribution<double> randomS(minscale,maxscale);//SCALE FACTOR
    std::uniform_real_distribution<double> randomRollPitch(-45*(M_PI/180), 45*(M_PI/180));

    //cout << "Seed: " << seed << "\n";
    // generate initial set
    for(int i=0; i<particles.size(); i++)
    {
        bool valid = false;
        do{            
            // sample particle pose
            particles[i].p.x = randomX(generator);
            particles[i].p.y = randomY(generator);
            particles[i].p.roll = randomRollPitch(generator);
            particles[i].p.pitch = randomRollPitch(generator);
            particles[i].p.yaw = randomTh(generator);
            particles[i].p.z = randomS(generator); //SCALE FACTOR

//            particles[i].p.x = globalMaps[0].cols/2;
//            particles[i].p.y = globalMaps[0].rows/2;
//            particles[i].p.theta = 0.0;

//            particles[i].p=initial;

            // check if particle is valid (known and not obstacle)
//            if(realMap->isKnown((int)particles[i].p.x,(int)particles[i].p.y) &&
//               !realMap->isObstacle((int)particles[i].p.x,(int)particles[i].p.y))
                valid = true;

        }while(!valid);

//        cout << "Particle (" << i << ") " << RAD2DEG(particles[i].p.theta) << endl;

        //if(i < 10)
        //    cout << "X: " << particles[i].p.x << " Y: " << particles[i].p.y << " Yaw: " << particles[i].p.yaw << " Z: " << particles[i].p.z << "\n";
    }

    /******************** Prepare log file *****************************/
    if(lName.empty()){
        time_t t = time(0);
        struct tm *now = localtime(&t);
        stringstream logName;
        logName << "../phir2framework/Logs/mcl-" << -100+now->tm_year
                        << setfill('0') << setw(2) << 1+now->tm_mon
                        << setfill('0') << setw(2) << now->tm_mday << '-'
                        << setfill('0') << setw(2) << now->tm_hour
                        << setfill('0') << setw(2) << now->tm_min
                        << setfill('0') << setw(2) << now->tm_sec << ".txt";
        cout << logName.str() << endl; cout.flush();
        particleLog.open(logName.str().c_str(), std::fstream::out);
    }else{
        particleLog.open(lName.c_str(), std::fstream::out);
    }
//    particleLog << "trueX trueY meanPX meanPY closestx cloesty closestTh closestw meanParticleErrorOk meanParticleStdev meanErrorOk stdevError trueTh meanAngle angleStdev angleError stdevAngleError NEFF elapsedTime\n";

    // Check if must remove duplicates in resampling
    removeDuplicates=false;
    for(int h=0; h<heuristics.size(); ++h)
        if(heuristics[h]->getType() == MEAN_SHIFT){
            removeDuplicates=true;
            break;
        }

    buildTableZ();
}

void MCL::restart(Pose3d &initial, string &lName)
{
    lastOdometry.x=0.0;
    lastOdometry.y=0.0;
    lastOdometry.yaw=0.0;

    realPose = initial;
    odomPose = initial;
//    odomPose.theta = 0;

    std::default_random_engine generator;
//    std::uniform_real_distribution<double> randomX(0.25*globalMaps[0].cols,0.75*globalMaps[0].cols);
//    std::uniform_real_distribution<double> randomY(0.25*globalMaps[0].cols,0.75*globalMaps[0].cols);
    std::uniform_real_distribution<double> randomX(0.0,globalMaps[0].cols);
    std::uniform_real_distribution<double> randomY(0.0,globalMaps[0].rows);
    std::uniform_real_distribution<double> randomTh(-M_PI,M_PI);
    std::uniform_real_distribution<double> randomS(minscale, maxscale); //SCALE FACTOR
    std::uniform_real_distribution<double> randomRollPitch(-rollPitchAngRange*(M_PI/180), rollPitchAngRange*(M_PI/180));

    // generate initial set
    for(int i=0; i<particles.size(); i++){

        bool valid = false;
        do{

            // sample particle pose
            particles[i].p.x = randomX(generator);
            particles[i].p.y = randomY(generator);
            particles[i].p.roll = randomRollPitch(generator);
            particles[i].p.pitch = randomRollPitch(generator);
            particles[i].p.yaw = randomTh(generator);
            particles[i].s = randomS(generator); //SCALE FACTOR

//            particles[i].p.x = globalMaps[0].cols/2;
//            particles[i].p.y = globalMaps[0].rows/2;
//            particles[i].p.theta = 0.0;

//            particles[i].p=initial;

            // check if particle is valid (known and not obstacle)
//            if(realMap->isKnown((int)particles[i].p.x,(int)particles[i].p.y) &&
//               !realMap->isObstacle((int)particles[i].p.x,(int)particles[i].p.y))
                valid = true;

        }while(!valid);

//        cout << "Particle (" << i << ") " << RAD2DEG(particles[i].p.theta) << endl;
    }

    realPath.clear();
    odomPath.clear();

    if(particleLog.is_open())
        particleLog.close();

    /******************** Prepare log file *****************************/
    if(lName.empty()){
        time_t t = time(0);
        struct tm *now = localtime(&t);
        stringstream logName;
        logName << "../phir2framework/Logs/mcl-" << -100+now->tm_year
                        << setfill('0') << setw(2) << 1+now->tm_mon
                        << setfill('0') << setw(2) << now->tm_mday << '-'
                        << setfill('0') << setw(2) << now->tm_hour
                        << setfill('0') << setw(2) << now->tm_min
                        << setfill('0') << setw(2) << now->tm_sec << ".txt";
        cout << logName.str() << endl; cout.flush();
        particleLog.open(logName.str().c_str(), std::fstream::out);
    }else{
        particleLog.open(lName.c_str(), std::fstream::out);
    }
//    particleLog << "trueX trueY meanPX meanPY closestx cloesty closestTh closestw meanParticleError meanParticleStdev meanError stdevError trueTh meanAngle angleStdev angleError stdevAngleError NEFF elapsedTime\n";
      particleLog << "meanPX meanPY meanPZ varPX varPY varPZ meanAngle angleStdev NEFF elapsedTime\n";
}

MCL::~MCL()
{
    particleLog.close();
}

void MCL::draw(int x_aux, int y_aux, int halfWindowSize)
{
    int h = globalMaps[0].rows;
    int w = globalMaps[0].cols;
    double x = 0;
    double y = 0;
    double th = 0;

    // Atualiza a região da janela
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    if(w > h){
        glOrtho (0 + x_aux + halfWindowSize,
                 w + x_aux - halfWindowSize,
                 h+(w-h)/2 + y_aux - halfWindowSize,
                 -(w-h)/2 + y_aux + halfWindowSize,
                 -1, 50);
    }else{
        glOrtho (-(h-w)/2 + x_aux + halfWindowSize,
                 w+(h-w)/2 + x_aux - halfWindowSize,
                 h + y_aux - halfWindowSize,
                 0 + y_aux + halfWindowSize,
                 -1, 50);
    }
    glMatrixMode (GL_MODELVIEW);

    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear (GL_COLOR_BUFFER_BIT);

    glColor3f(1.0f, 1.0f, 1.0f);

    glEnable(GL_TEXTURE_2D);
    // Draw map
    // Note: Window co-ordinates origin is top left, texture co-ordinate origin is bottom left.
    glBindTexture(GL_TEXTURE_2D, imageTex);
    glBegin(GL_QUADS);
        glTexCoord2f(1, 1);
        glVertex2f(0,  h);
        glTexCoord2f(0, 1);
        glVertex2f( w,  h);
        glTexCoord2f(0, 0);
        glVertex2f(w, 0);
        glTexCoord2f(1, 0);
        glVertex2f(0, 0);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

    // Draw particle cloud path (mean)
    if(meamParticlePath.size() > 1)
    {
        int lineSize = 0;
            for(unsigned int i = 0; i<meamParticlePath.size()-1; i++)
            {
                //Largura da linha proprocional ao erro
                //lineSize = (int)(varParticlePath[i].x + varParticlePath[i].y)/2;
                //lineSize = 3;
                //glLineWidth(lineSize);

                glLineWidth(3);
                glBegin( GL_LINE_STRIP ); // point
                {
                    glColor4f(1.0,0.0,1.0, 0.5);
                    glVertex2f(meamParticlePath[i].x, meamParticlePath[i].y);
                    glVertex2f(meamParticlePath[i+1].x, meamParticlePath[i+1].y);
                    glEnd();
                }
            }
    }

    // Draw real path
    if(realPath.size() > 1){
        glLineWidth(3);
        glBegin( GL_LINE_STRIP );
        {
            glColor3f(0.0,1.0,0.0);
            for(unsigned int i=0;i<realPath.size()-1; i++){
                glVertex2f(realPath[i].x, realPath[i].y);
                glVertex2f(realPath[i+1].x, realPath[i+1].y);
            }
        }
        glEnd();
        glLineWidth(1);
    }

    // Draw corrected real path
    if(realPath.size() > 1){
        glLineWidth(3);
        glBegin( GL_LINE_STRIP );
        {
            glColor3f(0.0,1.0,1.0);
            for(unsigned int i=0;i<realPath.size()-1; i++){
                glVertex2f(CorrectedGT[i].x, CorrectedGT[i].y);
                glVertex2f(CorrectedGT[i+1].x, CorrectedGT[i+1].y);
            }
        }
        glEnd();
        glLineWidth(1);
    }

    // Draw odom path
    if(odomPath.size() > 1){
        glLineWidth(3);
        glBegin( GL_LINE_STRIP );
        {
            glColor3f(1.0,1.0,0.0);
            for(unsigned int i=0;i<odomPath.size()-1; i++){
                glVertex2f(odomPath[i].x, odomPath[i].y);
                glVertex2f(odomPath[i+1].x, odomPath[i+1].y);
            }
        }
        glEnd();
        glLineWidth(1);
    }

    // Draw corrected real pose
    if(CorrectedGT.size() > 0)
    {
        x = CorrectedGT[meamParticlePath.size()-1].x;
        y = CorrectedGT[meamParticlePath.size()-1].y;
        th = CorrectedGT[meamParticlePath.size()-1].z;
        glColor3f(0.0,1.0,1.0);
        glPointSize(12);
        glBegin( GL_POINTS ); // point
        {
            glVertex2f(x, y);
        }
        glEnd();
        glColor3f(0.0, 0.0, 0.0);
        glLineWidth(2);
        glBegin( GL_LINES ); // direction
        {
            glVertex2f(x, y);
            glVertex2f(x+cos(th)*150, y+sin(th)*150);
        }
        glEnd();
        glLineWidth(1);
    }

    // Draw real pose
    x=realPose.x;
    y=realPose.y;
    th=realPose.yaw;
    glColor3f(0.0,1.0,0.0);
    glPointSize(12);
    glBegin( GL_POINTS ); // point
    {
        glVertex2f(x, y);
    }
    glEnd();
    glColor3f(0.0, 0.0, 0.0);
    glLineWidth(2);
    glBegin( GL_LINES ); // direction
    {
        glVertex2f(x, y);
        glVertex2f(x+cos(th)*150, y+sin(th)*150);
    }
    glEnd();
    glLineWidth(1);

    // Draw odom pose
    x=odomPose.x;
    y=odomPose.y;
    th=odomPose.yaw;
    glColor3f(1.0,1.0,0.0);
    glPointSize(12);
    glBegin( GL_POINTS ); // point
    {
        glVertex2f(x, y);
    }
    glEnd();
    glColor3f(0.0, 0.0, 0.0);
    glLineWidth(2);
    glBegin( GL_LINES ); // direction
    {
        glVertex2f(x, y);
        glVertex2f(x+cos(th)*150, y+sin(th)*150);
    }
    glEnd();
    glLineWidth(1);

    // Draw particle cloud (mean)
    if(meamParticlePath.size() > 0)
    {
        x=meamParticlePath[meamParticlePath.size()-1].x;
        y=meamParticlePath[meamParticlePath.size()-1].y;
        th=meamParticlePath[meamParticlePath.size()-1].z;
        glColor3f(1.0,0.0,1.0);
        glPointSize(12);
        glBegin( GL_POINTS ); // point
        {
            glVertex2f(x, y);
        }
        glEnd();
        glColor3f(0.0, 0.0, 0.0);
        glLineWidth(2);
        glBegin( GL_LINES ); // direction
        {
            glVertex2f(x, y);
            glVertex2f(x+cos(th)*150, y+sin(th)*150);
        }
        glEnd();
        glLineWidth(1);
    }

    if(starting){
        glutSwapBuffers();
        glutPostRedisplay();
        return;
    }

    double highestParticleWeight = 0;
    double lowestParticleWeight = DBL_MAX;
    for(int p=0;p<particles.size();p++)
    {
        if(particles[p].w > highestParticleWeight)
            highestParticleWeight = particles[p].w;
        else if(particles[p].w  < lowestParticleWeight)
            lowestParticleWeight = particles[p].w;
    }


    // Draw particles
    //float maxWeig = 0;
    if(sumW > 0)//Só atualizar o desenho das partículas quando forem calculados novos pesos
    {
        for(int p=0;p<particles.size();p++)
        {
            double parWeightDrawing = Utils::Normalize(particles[p].w, 0, 1, lowestParticleWeight, highestParticleWeight);
            double x=particles[p].p.x;
            double y=particles[p].p.y;
            double th=particles[p].p.yaw;

            // Draw point
            //Alterar para variar entre vermelhoe  magenta, conforme a altitude da partícula(qnd eu possuir esa info)
    //        if(particles[p].w == 0.0)
                //glColor3f(1.0,0.0,0.0);
                //glColor4f(1.0,0.0,0.0,0.3);//with alpha

                //glColor4f(1.0, 0.0, 0.0, 1 - parWeightDrawing);
                glColor4f(parWeightDrawing, 1 - parWeightDrawing, 0.0, 1.0);
                //float tst = particles[p].w * sumW;
                //if(tst > maxWeig) maxWeig = tst;
                //if(tst > 0)
                //    glColor3f(particles[p].w * sumW, 1 - (particles[p].w * sumW), 0.5);

    //        else
    //            glColor3f(1.0,1.0,0.0);

            glPointSize(6);
            glBegin( GL_POINTS );
            {
                glVertex2f(x, y);
            }
            glEnd();

            // Draw direction
            //glColor3f(0.0, 0.0, 1.0);
            glColor4f(0.0, 0.0, 1.0, 1.0);

            glLineWidth(2);
            glBegin( GL_LINES );
            {
                glVertex2f(x, y);
                glVertex2f(x+cos(th)*50, y+sin(th)*50);
            }
            glEnd();
            glLineWidth(1);
        }
        //sumW = 0;//Reinicializar, pois só uso essa variável para pintar as partículas
    }

    // Desenha robô
//    double xRobot = robot_->truePose_.x/robot_->scale;
//    double yRobot = robot_->truePose_.y/robot_->scale;
//    double angRobot = -robot_->truePose_.theta;

//    double xRobot = robot_->trueLocRX;//w/2;
//    double yRobot = h-robot_->trueLocRY; //h/2
//    double angRobot = -robot_->trueLocTh*180.0/M_PI;

//    glTranslatef(xRobot,yRobot,0.0);
//    drawRobot(angRobot, robot_->scale);
//    glTranslatef(-xRobot,-yRobot,0.0);
//    //printRealPos(xRobot, yRobot, 0.7, 0.0, 0.0);

//    if(robot_->slam->saveImage)
//        screenshot();


    if(currentImage == (imgFimExec - imgIniExec) && !imgMapaSalva)//Salvar as últiams versões do mapa, desde que a última imagem da rota foi carregada.
    {
        //Salvar a versao final do mapa (a imagem ficou bem menor do que deveria, mas salvou e manteve as proporções)
        //vector<int> compression_params;
        float rnd = Utils::RandomFloat(0, 1000);
        std::ostringstream ssini, ssFim;
        string imgName;
        //compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
        //compression_params.push_back(9);//For PNG, it can be the compression level ( CV_IMWRITE_PNG_COMPRESSION ) from 0 to 9. A higher value means a smaller size and longer compression time. Default value is 3.

        ssini << imgIniExec;
        std::string strIniExec(ssini.str());
        ssini << imgFimExec;

        std::string strFimExec(ssFim.str());
        ssFim << rnd;
        std::string strRnd(ssFim.str());//ADICIONAR  A GRAVAÇÃO DO MAPA AO DIRETÓRIO DOS EXPERTIMENTOS

        imgName = "/home/phi/images/tstsvMap/Mapa_Final_" + strIniExec + "_" + strFimExec + "_" + strRnd + ".png";

        //Converter da exiibção da GLUT para uma matriz do opencv
        //https://stackoverflow.com/questions/9097756/converting-data-from-glreadpixels-to-opencvmat/9098883
        //cv::Mat img(4800, 4800, CV_8UC3);
        //cv::Mat flipped(4800, 4800, CV_8UC3);

        //use fast 4-byte alignment (default anyway) if possible

        //glReadPixels(0, 0, img.cols, img.rows, GL_BGR, GL_UNSIGNED_BYTE, img.data);
        //cv::flip(img, flipped, 0);

        //imwrite(imgName, flipped, compression_params);

        GlutClass* glut=GlutClass::getInstance();
        glut->screenshot(imgName);
        imgMapaSalva = true;
    }
    glutSwapBuffers();
    glutPostRedisplay(); 
}


bool MCL::initialRun(Pose3d &u, bool is_u_reliable, cv::Mat &z, double time, Pose3d &real, double lastTotalElapsed, cv::Mat &redMap, cv::Mat &nirMap, cv::Mat &regMap, cv::Mat &greMap, cv::Mat &currentMapOriginal, Pose3d orientacao)
{
    // Create different versions of the input image
    createColorVersions(z);

    realPose = real;
    realPath.push_back(realPose);

//    cout << "Starting MCL" << endl;

    double wElapsedTime, totalElapsedTime=0.0;
    struct timeval tstart, tend, tstartW, tendW;

    // Start counting the elapsed time of this iteration
    gettimeofday(&tstart, NULL);

    sampling(u,is_u_reliable);

    gettimeofday(&tstartW, NULL);

    if(multispectralCam)
        prepareWeighting(z, redMap, nirMap, regMap, greMap, currentMapOriginal, orientacao);
    else
        prepareWeighting(z);

    weighting(z, u, redMap, nirMap, regMap, greMap, currentMapOriginal);
    gettimeofday(&tendW, NULL);

    double sumWeights = 0.0;
    for(int i=0; i<particles.size(); i++)
        sumWeights += particles[i].w;


    if(sumWeights!=0)
        resampling();

    lastOdometry = u;

    // Stop counting the elapsed time of this SLAM iteration
    gettimeofday(&tend, NULL);

    // Compute and print the elapsed time
    if (tstart.tv_usec > tend.tv_usec) {
        tend.tv_usec += 1000000;
        tend.tv_sec--;
    }
    if (tstartW.tv_usec > tendW.tv_usec) {
        tendW.tv_usec += 1000000;
        tendW.tv_sec--;
    }

    wElapsedTime = ((double)tendW.tv_sec - (double)tstartW.tv_sec) + ((double)tendW.tv_usec - (double)tstartW.tv_usec)/1000000.0;
    totalElapsedTime = ((double)tend.tv_sec - (double)tstart.tv_sec) + ((double)tend.tv_usec - (double)tstart.tv_usec)/1000000.0;
    cout << "PARTICLES:" << numParticles
         << " total elapsed time MCL: " << totalElapsedTime << " weighting elapsed time MCL: " << wElapsedTime
         << " ratio:" << wElapsedTime/totalElapsedTime << endl;

//    particleLog  <<  "elapsed time MCL: " << elapsedTime << endl;
    float time_limit = 0.33333333;
    if(wElapsedTime > time_limit ){
        numParticles = (time_limit-lastTotalElapsed)/(totalElapsedTime-lastTotalElapsed)*(numParticles/2) + numParticles/2;

        particles.resize(numParticles);
        cout << "FINAL NUMPARTICLES " << numParticles << endl;

        particleLog << "# NumParticles " << numParticles << endl;
        particleLog << "# trueX trueY meanPX3333 meanPY closestx cloesty closestTh closestw meanParticleErrorOk meanParticleStdev meanErrorOk stdevError trueTh meanAngle angleStdev angleError stdevAngleError NEFF elapsedTime\n";

        std::default_random_engine generator;
    //    std::uniform_real_distribution<double> randomX(0.25*globalMaps[0].cols,0.75*globalMaps[0].cols);
    //    std::uniform_real_distribution<double> randomY(0.25*globalMaps[0].cols,0.75*globalMaps[0].cols);
        std::uniform_real_distribution<double> randomX(0.0,globalMaps[0].cols);
        std::uniform_real_distribution<double> randomY(0.0,globalMaps[0].rows);
        std::uniform_real_distribution<double> randomTh(-M_PI,M_PI);
        std::uniform_real_distribution<double> randomS(minscale, maxscale); //SCALE FACTOR
        std::uniform_real_distribution<double> randomRollPitch(-rollPitchAngRange*(M_PI/180), rollPitchAngRange*(M_PI/180));

        // generate initial set
        for(int i=0; i<particles.size(); i++){

            bool valid = false;
            do{

                // sample particle pose
                particles[i].p.x = randomX(generator);
                particles[i].p.y = randomY(generator);
                particles[i].p.roll = randomRollPitch(generator);
                particles[i].p.pitch = randomRollPitch(generator);
                particles[i].p.yaw = randomTh(generator);
                particles[i].s = randomS(generator); //SCALE FACTOR
                valid = true;

            }while(!valid);
        }

        starting=false;
    }else{

        numParticles*=2;
        particles.resize(numParticles);

        std::default_random_engine generator;
    //    std::uniform_real_distribution<double> randomX(0.25*globalMaps[0].cols,0.75*globalMaps[0].cols);
    //    std::uniform_real_distribution<double> randomY(0.25*globalMaps[0].cols,0.75*globalMaps[0].cols);
        std::uniform_real_distribution<double> randomX(0.0,globalMaps[0].cols);
        std::uniform_real_distribution<double> randomY(0.0,globalMaps[0].rows);
        std::uniform_real_distribution<double> randomTh(-M_PI,M_PI);
        std::uniform_real_distribution<double> randomS(minscale, maxscale);  //SCALE FACTOR
        std::uniform_real_distribution<double> randomRollPitch(-rollPitchAngRange*(M_PI/180), rollPitchAngRange*(M_PI/180));

        // generate initial set
        for(int i=0; i<particles.size(); i++){

            bool valid = false;
            do{

                // sample particle pose
                particles[i].p.x = randomX(generator);
                particles[i].p.y = randomY(generator);
                particles[i].p.roll = randomRollPitch(generator);
                particles[i].p.pitch = randomRollPitch(generator);
                particles[i].p.yaw = randomTh(generator);
                particles[i].s = randomS(generator); //SCALE FACTOR
                valid = true;

            }while(!valid);
        }

        initialRun(u,is_u_reliable,z,time,real,totalElapsedTime, redMap, nirMap, regMap, greMap, currentMapOriginal, droneAngles[currentImage]);
    }

    return true;

}

bool MCL::run(Pose3d &u, bool is_u_reliable, cv::Mat &z, double time, Pose3d& real, cv::Mat &redMap, cv::Mat &nirMap, cv::Mat &regMap, cv::Mat &greMap, cv::Mat &currentMapOriginal)//"z" é o mapa local atual, a imagem que a câmera do drone acaba de capturar.
{
    int KLD = 1; // 1 - STANDARD MCL | 2 - PARTICLE BY PARTICLE WITH KLD ALGORITHMS | 3 - JUST KLD
    if(KLD == 1){
    //    if(starting)
    //        initialRun(u,is_u_reliable,z,time,real,0.0);

    //    double delta = sqrt(pow(u.x-lastOdometry.x,2)+pow(u.y-lastOdometry.y,2));
    //    if(delta<1.0)
    //        return false;

        // Create different versions of the input image
        createColorVersions(z);

        realPose = real;
        realPath.push_back(realPose);

    //    cout << "Starting MCL" << endl;

    //    vector<MCLparticle> aux;
    //    aux.clear();
    //    for(int i = 0; i < particles.size(); i++){
    //         aux.push_back(samplingKLD(u,is_u_reliable, particles[i])); //SAMPLING IS ADAPTED TO KDL
    //    }
    //    particles.clear();
    //    particles = aux;

        sampling(u,is_u_reliable);

        if(multispectralCam)
            prepareWeighting(z, redMap, nirMap, regMap, greMap, currentMapOriginal, droneAngles[currentImage]);
        else
            prepareWeighting(z);

        weighting(z, u, redMap, nirMap, regMap, greMap, currentMapOriginal);

    //    if(locTechnique == SSD)
    //       weightingSSD(z);
    //    if(locTechnique == DENSITY)
    //       weightingDensity(densities,u,gradients);
    //    if(locTechnique == COLOR_ONLY)
    //       weightingColor(); // receive color maps

        double sumWeights = 0.0;
        for(int i=0; i<particles.size(); i++)
            sumWeights += particles[i].w;


    //    if(computeNeff() < numParticles/2.0)
        if(sumWeights!=0)
            resampling();

        lastOdometry = u;


//        particleLog  <<  "elapsed time MCL: " << elapsedTime << endl;
    }else{
        if(KLD == 2){
            createColorVersions(z);
            vector<MCLparticle> aux;

            realPose = real;
            realPath.push_back(realPose);

            //RESAMPLING
            double sumWeights = 0.0;
            for(int i=0; i<particles.size(); i++)
                sumWeights += particles[i].w;

            if(sumWeights!=0){
                aux.clear();
                for(int i = 0; i < particles.size(); i++){
                    aux.push_back(resamplingKLD());
                }
                particles.clear();
                particles = aux;
            }

            //SAMPLING
            updateOdomPath(u);
            aux.clear();
            for(int i = 0; i < particles.size(); i++){
              aux.push_back(samplingKLD(u, is_u_reliable, particles[i]));
            }
            particles.clear();
            particles = aux;

            //WEIGHTING - ELE FAZ NORMALIZAÇÃO AQUI NO WEIGHTING
            if(multispectralCam)
                prepareWeighting(z, redMap, nirMap, regMap, greMap, currentMapOriginal, droneAngles[currentImage]);
            else
                prepareWeighting(z);

            weighting(z, u, redMap, nirMap, regMap, greMap, currentMapOriginal);

            lastOdometry = u;
        }else{
            kldSampling(u, is_u_reliable, z, time, real, redMap, nirMap, regMap, greMap, currentMapOriginal);
//            cout<<"*********************************** ESTA TOTALMENTE NO KLD AGORA **********************"<<endl;
        }
    }
    return true;
}

void MCL::kldSampling(Pose3d &u, bool is_u_reliable, cv::Mat &z, double time, Pose3d& real, cv::Mat &redMap, cv::Mat &nirMap, cv::Mat &regMap, cv::Mat &greMap, cv::Mat &currentMapOriginal){
    int M, Mx, k, minparticle, maxparticle;
    float error, zvalue, confidence, averageScale;
    vector<MCLparticle> newSetParticles;
    MCLparticle newParticle;

    M = k = 0;
    Mx = std::numeric_limits<int>::max();
    newSetParticles.clear();

    createColorVersions(z);

    realPose = real;
    realPath.push_back(realPose);

    maxparticle = 70000;
    minparticle = 15000;
    error = .48;
    confidence = 0.4999; //ztable is from right side of mean (values between 0 ~ .5)
    confidence = fmin(0.49998, fmax(0, confidence));

    zvalue = getingZValue(confidence);

    setingEmptyBins();
    updateOdomPath(u);

    double sumWeights = 0.0;
    averageScale = 0;
    for(int i=0; i<particles.size(); i++)
        sumWeights += particles[i].w;

    if(sumWeights == 0.0)
        for(int i = 0; i < particles.size(); i++)
            particles[i].w = 1.0/(float)particles.size();

    //SCALE AdM MAP
    do{
        do{
            newParticle = resamplingKLD();
            newParticle = samplingKLD(u, is_u_reliable, newParticle);
        }while(verifyParticlePosition(newParticle));
        newSetParticles.push_back(newParticle);

        if(isEmpty(newParticle)){
            k++;
            if(k > 1){
                Mx = (int)ceil(((k-1)/(2*error))*pow(1 - (2/(9.0*(k-1))) + (sqrt(2/(9.0*(k-1))))*zvalue,3));
//                Mx = 6000;
            }
            if(Mx > maxparticle)
                Mx = maxparticle;
            if(Mx < minparticle)
                Mx = minparticle;
        }
        M++;
        //cout << M << endl;
        averageScale += newParticle.p.z;
    }while(M < Mx);
    cout<<" -------------- PARTICLE SET INFORMATION --------------"<<endl;
    cout<<"               Target:"<<Mx<<" - Final:"<<M<<"              "<<endl;
    particles.clear();
    particles = newSetParticles;

    cout<<"                  AverageScale:"<<averageScale/M<<"                 "<<endl;
    cout<<" ------------------------------------------------------"<<endl;

    if(multispectralCam)
        prepareWeighting(z, redMap, nirMap, regMap, greMap, currentMapOriginal, droneAngles[currentImage]);
    else
        prepareWeighting(z);

    weighting(z, u, redMap, nirMap, regMap, greMap, currentMapOriginal);

    lastOdometry = u;

}

void MCL::writeLogFile3d()
{
    double elapsedTime=timer.getLapTime();

    /**************************************************************
     ******************  Position information  ********************
     **************************************************************/
    // For each particle compute the weighed mean position error
    double normalizer = 0.0;
    double meanPX = 0.0;
    double meanPY = 0.0;
    double meanPZ = 0.0;

    double varPX = 0.0;
    double varPY = 0.0;
    double varPZ = 0.0;

    double NEFF = 0.0;
    // Evaluate weighed mean particle
    for(int i=0; i<particles.size(); i++) {
        normalizer+=particles[i].w;
        NEFF+=particles[i].w*particles[i].w;
    }
    cout << "Normalizer data:" << normalizer << endl;
    NEFF = 1/NEFF;

    if(normalizer==0.0)
        for(int i=0; i<particles.size(); i++)
            particles[i].w=1.0/particles.size();

    // Evaluate weighed mean particle
    for(int i=0; i<particles.size(); i++)
    {
        meanPX += particles[i].p.x*particles[i].w;
        meanPY += particles[i].p.y*particles[i].w;
        meanPZ += particles[i].p.z*particles[i].w;
        //normalizer+=particles[i].w;
        //normalizer++;
    }

    meanPX  /= normalizer;
    meanPY  /= normalizer;
    meanPZ  /= normalizer;

    // Evaluate weighed mean particle variance
    double meanParticleVar = 0.0;
    for(int i=0; i<particles.size(); i++)
    {
      //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@2
      //Variancia de X, Y e Z
      varPX = pow((particles[i].p.x-meanPX), 2.0)*particles[i].w;
      varPY = pow((particles[i].p.y-meanPY), 2.0)*particles[i].w;
      varPZ = pow((particles[i].p.z-meanPZ), 2.0)*particles[i].w;
    }
    //meanParticleVar/=normalizer;
    // double meanParticleStdev = sqrt(meanParticleVar);

    /**************************************************************
     ********************  Angle information  *********************
     **************************************************************/
    // For each particle compute the weighed mean angle

    double meanAngle  = 0.0;
    double angle,sin_ang,cos_ang;
    sin_ang = 0;
    cos_ang = 0;
    for(int i=0; i<particles.size(); i++){
        angle = particles[i].p.yaw;
        cos_ang += cos(angle)*particles[i].w;
        sin_ang += sin(angle)*particles[i].w;
    }
    cos_ang /= normalizer;
    sin_ang /= normalizer;
    meanAngle = atan2(sin_ang, cos_ang)*180/M_PI;
//    meanAngle  /= normalizer;

    // For each particle compute the mean particle angle var and stdev
    double stddev = sqrt(-1.0 * log(pow(sin_ang,2)+pow(cos_ang,2)));
    double angleVar = pow(stddev,2);


//    stddev/=normalizer;

//    THE OLD WAY THAT THE MEAN ANGLE WAS CALCULATED
//    double meanAngle  = 0.0;
//    for(int i=0; i<particles.size(); i++){
//        meanAngle   += particles[i].p.yaw*particles[i].w;
//    }
//    meanAngle  /= normalizer;
//    while (meanAngle > M_PI)
//        meanAngle -= 2*M_PI;
//    while (meanAngle < -M_PI)
//        meanAngle += 2*M_PI;

    // For each particle compute the mean particle angle var and stdev
//    double angleVar = 0.0;
//    for(int i=0; i<particles.size(); i++){
//        angleVar   += pow((particles[i].p.yaw-meanAngle), 2.0)*particles[i].w;
//    }
//    angleVar/=normalizer;
//    double angleStdev = sqrt(angleVar);
//    while (angleStdev > M_PI)
//        angleStdev -= 2*M_PI;
//    while (angleStdev < -M_PI)
//        angleStdev += 2*M_PI;
//    angleVar = angleStdev*angleStdev;


//    particleLog << "meanPX meanPY  meanPZ varPX varPY varPZ meanAngle angleStdev NEFF elapsedTime\n";
    particleLog  << meanPX << " " << meanPY << " " << meanPZ << " " << varPX<< " " << varPY << " " << varPZ << " "
                 << meanAngle << " " << angleVar << " "
                 << NEFF << " " << elapsedTime << endl;
    particleLog.flush();

    //Armazenando a média e a variância da nuvem, para plotar esse caminho no mapa.
    meamParticlePath.push_back(Pose3d(meanPX, meanPY, meanPY, meanAngle));
    varParticlePath.push_back(Pose3d(varPX, varPY, varPZ, angleVar));
    timer.startLap();    
}

void MCL::writeErrorLogFile3d(double trueX, double trueY, double trueTh)
{
    double elapsedTime=timer.getLapTime();

    /**************************************************************
     ******************  Position information  ********************
     **************************************************************/
    // For each particle compute the weighed mean position error
    double normalizer = 0.0;
    double meanParticleError = 0.0;
    double meanPX = 0.0;
    double meanPY = 0.0;

    double NEFF = 0.0;
    // Evaluate weighed mean particle
    for(int i=0; i<particles.size(); i++) {
        normalizer+=particles[i].w;
        NEFF+=particles[i].w*particles[i].w;
    }
    cout << "Normalizer data:" << normalizer << endl;
    NEFF = 1/NEFF;

    if(normalizer==0.0)
        for(int i=0; i<particles.size(); i++)
            particles[i].w=1.0/particles.size();

    // Evaluate weighed mean particle
    for(int i=0; i<particles.size(); i++)
    {
        meanPX += particles[i].p.x*particles[i].w;
        meanPY += particles[i].p.y*particles[i].w;
        //normalizer+=particles[i].w;
        //normalizer++;
    }

    meanPX  /= normalizer;
    meanPY  /= normalizer;
    meanParticleError = computeError(trueX, trueY, meanPX, meanPY);

    // Evaluate weighed mean particle variance
    double meanParticleVar = 0.0;
    for(int i=0; i<particles.size(); i++)
    {
        meanParticleVar +=
        pow(computeError(meanPX, meanPY, particles[i].p.x, particles[i].p.y)-meanParticleError, 2.0)*particles[i].w;
    }
    meanParticleVar/=normalizer;
    double meanParticleStdev = sqrt(meanParticleVar);

    // Evaluate mean error
    double meanError = 0.0;
    MCLparticle closest=particles[0];
    double bestError = DBL_MAX;
    for(int i=0; i<particles.size(); i++){
        double x = particles[i].p.x;
        double y = particles[i].p.y;

        // get closest particle
        double error = computeError(trueX, trueY, x, y);
        if(error<bestError)
        {
            closest = particles[i];
            bestError = error;
        }
        // weighing the error
        error *= particles[i].w;
        meanError += error;
    }
    meanError/=normalizer;

    // Evaluate mean error variance
    double varError = 0.0;
    for(int i=0; i<particles.size(); i++){
        double x = particles[i].p.x;
        double y = particles[i].p.y;
        varError += pow(computeError(trueX, trueY, x, y)-meanError, 2.0)*particles[i].w;
    }
    varError = varError/normalizer;
    double stdevError = sqrt(varError);

    /**************************************************************
     ********************  Angle information  *********************
     **************************************************************/
    // For each particle compute the weighed mean angle

    double meanAngle  = 0.0;
    double ponderada,sin_ang,cos_ang;
    sin_ang = 0;
    cos_ang = 0;
    for(int i=0; i<particles.size(); i++){
        ponderada = particles[i].p.yaw;
        cos_ang += cos(ponderada)*particles[i].w;
        sin_ang += sin(ponderada)*particles[i].w;
    }
    cos_ang /= normalizer;
    sin_ang /= normalizer;
    meanAngle = atan2(sin_ang, cos_ang)*180/M_PI;
//    meanAngle  /= normalizer;

    // For each particle compute the mean particle angle var and stdev    
    double stddev = sqrt(-1.0 * log(pow(sin_ang,2)+pow(cos_ang,2)));
    double angleVar = pow(stddev,2);

//    stddev/=normalizer;

    double angleStdev = sqrt(angleVar);
//    double meanAngle  = 0.0;
//    for(int i=0; i<particles.size(); i++){
//        meanAngle   += particles[i].p.yaw*particles[i].w;
//    }
//    meanAngle  /= normalizer;
//    while (meanAngle > M_PI)
//        meanAngle -= 2*M_PI;
//    while (meanAngle < -M_PI)
//        meanAngle += 2*M_PI;

//    // For each particle compute the mean particle angle var and stdev
//    double angleVar = 0.0;
//    for(int i=0; i<particles.size(); i++){
//        angleVar   += pow((particles[i].p.yaw-meanAngle), 2.0)*particles[i].w;
//    }
//    angleVar/=normalizer;
//    double angleStdev = sqrt(angleVar);
//    while (angleStdev > M_PI)
//        angleStdev -= 2*M_PI;
//    while (angleStdev < -M_PI)
//        angleStdev += 2*M_PI;
//    angleVar = angleStdev*angleStdev;


    // Compute the mean angle error
    double angleError = computeAngleError(trueTh, meanAngle);
//    angleError = 0.0;
//    for(int i=0; i<particles.size(); i++){
//        angleError   += (particles[i].p.theta-trueTh)*particles[i].w;
//    }
    angleError/=normalizer;
    while (angleError > M_PI)
        angleError -= 2*M_PI;
    while (angleError < -M_PI)
        angleError += 2*M_PI;

    // Compute the angle var error
    double varAngleError = 0.0;
    //cout << angleError << endl;
    for(int i=0; i<particles.size(); i++){
        varAngleError   += pow((particles[i].p.yaw-trueTh)-angleError, 2.0)*particles[i].w;
    }
    varAngleError/=normalizer;
    double stdevAngleError = sqrt(varAngleError);
    while (stdevAngleError > M_PI)
        stdevAngleError -= 2*M_PI;
    while (stdevAngleError < -M_PI)
        stdevAngleError += 2*M_PI;
    varAngleError = stdevAngleError*stdevAngleError;

    //particleLog << "trueX trueY meanPX meanPY closestx cloesty closestw meanParticleError meanParticleStdev meanError stdevError trueTh meanAngle angleStdev angleError stdevAngleError NEFF elapsedTime\n";
    particleLog  << trueX <<  " " << trueY << " " << " " << meanPX << " " << meanPY << " "
                 << closest.p.x << " " << closest.p.y << " " << closest.p.yaw << " " << closest.w << " "
                 << meanParticleError <<  " " << meanParticleStdev  << " " << meanError << " " << stdevError << " "
                 << trueTh << " " << meanAngle << " " << angleStdev << " " << angleError << " " << stdevAngleError << " "
                 << NEFF << " " << elapsedTime << endl;
    particleLog.flush();

    timer.startLap();
}

void MCL::setNumParticles(int _numParticles)
{
    this->numParticles = _numParticles;
}

//////////////////////
// Métodos Privados //
//////////////////////

void MCL::sampling(Pose3d &u, bool reliable)
{
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator (seed);
    std::uniform_real_distribution<double> randomValue(-1.0,1.0);
    std::uniform_real_distribution<double> randomS(-0.02578,0.02578);
    std::uniform_real_distribution<double> randomRollPitch(-rollPitchAngRange*(M_PI/180), rollPitchAngRange*(M_PI/180));

    updateOdomPath(u);

//    cout << "Real Pose " << realPose << endl;
//    cout << "Odom Pose " << odomPose << endl;

    compensateCloudPosition();//Alterar a posição das partículuas com base na orientação do drone

    if(reliable){
        //#pragma omp parallel for num_threads(NUM_THREADS)
        for(int i=0; i<particles.size(); i++){

            float newX = u.x * (particles[i].p.z);
            float newY = u.y * (particles[i].p.z);

            particles[i].p.x += cos(particles[i].p.yaw)*newX - sin(particles[i].p.yaw)*newY + randomValue(generator)*10.0;
            particles[i].p.y += sin(particles[i].p.yaw)*newX + cos(particles[i].p.yaw)*newY + randomValue(generator)*10.0;
            particles[i].p.yaw += u.yaw + randomValue(generator)*5*M_PI/180.0;
            particles[i].p.roll = randomRollPitch(generator);
            particles[i].p.pitch = randomRollPitch(generator);

            particles[i].p.z = (particles[i].p.z * u.z) + randomS(generator); //Z é uma escala, e não um deslocamento, então, ele é multiplicado. Se a altura não mudar nas imagens, ele é igual a 1, e não altera o valor

            while(particles[i].p.yaw > M_PI)
                particles[i].p.yaw -= 2*M_PI;
            while(particles[i].p.yaw < -M_PI)
                particles[i].p.yaw += 2*M_PI;
        }
    }else{
        cout << "!UNRELIABLE!!!" << endl;
        #pragma omp parallel for num_threads(NUM_THREADS)
        for(int i=0; i<particles.size(); i++){

            float newX = u.x * (particles[i].p.z);
            float newY = u.y * (particles[i].p.z);

            particles[i].p.x += cos(particles[i].p.yaw)*newX - sin(particles[i].p.yaw)*newY + randomValue(generator)*10.0;
            particles[i].p.y += sin(particles[i].p.yaw)*newX + cos(particles[i].p.yaw)*newY + randomValue(generator)*10.0;
            particles[i].p.yaw += u.yaw + randomValue(generator)*30*M_PI/180.0;
            particles[i].p.roll = randomValue(generator)*M_PI/180.0;
            particles[i].p.pitch = randomValue(generator)*M_PI/180.0;

            particles[i].p.z = (particles[i].p.z * u.z) + randomS(generator); //Z é uma escala, e não um deslocamento, então, ele é multiplicado. Se a altura não mudar nas imagens, ele é igual a 1, e não altera o valor

            while(particles[i].p.yaw > M_PI)
                particles[i].p.yaw -= 2*M_PI;
            while(particles[i].p.yaw < -M_PI)
                particles[i].p.yaw += 2*M_PI;
        }
    }
}


MCLparticle MCL::samplingKLD(Pose3d &u, bool reliable, MCLparticle part)
{
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator (seed);
    std::uniform_real_distribution<double> randomValue(-1.0,1.0);
    std::uniform_real_distribution<double> randomS(-0.01578,0.01578);

    float newX = u.x * (part.p.z);
    float newY = u.y * (part.p.z);

    if(reliable){
        part.p.x += cos(part.p.yaw)*newX - sin(part.p.yaw)*newY + randomValue(generator)*10.0;
        part.p.y += sin(part.p.yaw)*newX + cos(part.p.yaw)*newY + randomValue(generator)*10.0;
        part.p.yaw += u.yaw + randomValue(generator)*5*M_PI/180.0;

        part.p.z = (part.p.z /* * u.z*/) + randomS(generator); //Z é uma escala, e não um deslocamento, então, ele é multiplicado. Se a altura não mudar nas imagens, ele é igual a 1, e não altera o valor

        while(part.p.yaw > M_PI)
            part.p.yaw -= 2*M_PI;
        while(part.p.yaw < -M_PI)
            part.p.yaw += 2*M_PI;
    }else{
        //cout << "@UNRELIABLE!!!" << endl;
        part.p.x += cos(part.p.yaw)*newX - sin(part.p.yaw)*newY + randomValue(generator)*10.0;
        part.p.y += sin(part.p.yaw)*newX + cos(part.p.yaw)*newY + randomValue(generator)*10.0;
        part.p.yaw += u.yaw + randomValue(generator)*30*M_PI/180.0;

        part.p.z = (part.p.z /* * u.z*/) + randomS(generator); //Z é uma escala, e não um deslocamento, então, ele é multiplicado. Se a altura não mudar nas imagens, ele é igual a 1, e não altera o valor

        while(part.p.yaw > M_PI)
            part.p.yaw -= 2*M_PI;
        while(part.p.yaw < -M_PI)
            part.p.yaw += 2*M_PI;
    }
    return part;
}

void MCL::updateOdomPath(Pose3d &u){
    //Matriz de transformação  (Z é a escala)
    //Ao invés de construir uma operação matricial, a multiplicação é montade de uma vez, torando a execução mais rápida.
    //Trata-se de uma rotação e uma escala
    odomPose.x += cos(odomPose.yaw)*u.x*odomPose.z - sin(odomPose.yaw)*u.y*odomPose.z;
    odomPose.y += sin(odomPose.yaw)*u.x*odomPose.z + cos(odomPose.yaw)*u.y*odomPose.z;
    odomPose.z *= u.z;
    odomPose.yaw += u.yaw;
    while(odomPose.yaw > M_PI)
        odomPose.yaw -= 2*M_PI;
    while(odomPose.yaw < -M_PI)
        odomPose.yaw += 2*M_PI;

    odomPath.push_back(odomPose);
}

//int contador = 0;
void MCL::weighting(cv::Mat& z_robot, Pose3d &u, cv::Mat &redMap, cv::Mat &nirMap, cv::Mat &regMap, cv::Mat &greMap, cv::Mat &currentMapOriginal)
{
    //contador++;
    int count = 0;
    double t = (double)cv::getTickCount();
    //bool flagAngEstimate = false;

   // #pragma omp parallel for num_threads(NUM_THREADS) shared(nirMap, redMap, z_robot, count) default(none)
    #pragma omp parallel for num_threads(NUM_THREADS)
    for(int i=0;i<particles.size();i++){

//        if(i%1000 == 0) cout << i << endl;

        int x=round(particles[i].p.x);
        int y=round(particles[i].p.y);

        // check if particle is not valid (unknown or obstacle)
        if(heuristics[0]->getType() != MUTUAL_INFORMATION && heuristics[0]->getType() != BRIEF && heuristics[0]->getType() != SSD && heuristics[0]->getType() != COLOR_ONLY && heuristics[0]->getType() != UNSCENTED_COLOR && heuristics[0]->getType() != HISTOGRAM_MATCHING && heuristics[0]->getType() != SIFT_MCL)
            if(!cachedMaps[0]->isKnown(x,y) || cachedMaps[0]->isObstacle(x,y)){
                particles[i].w = 0.0;
                count++;
                continue;
            }

        double varUnscented = pow(2.5, 2.0); // normalized gaussian
        double varColor = pow(0.5, 2.0); // normalized gaussian
        double varDensity = pow(1.0f,2.0); //10%
        double varEntropy = pow(0.1f,2.0); //10%
        double varMI = pow(.15,2.0);
        double varMeanShift = pow(1.0,2.0); // 1 std dev
        double varHistogram = pow(.33, 2.0);
        double varBrief = pow(.15, 2.0);
        double prob=1.0;

        for(int l=0;l<heuristics.size();++l)
        {
            Heuristic* h = heuristics[l];
            int mapID = selectMapID(h->getColorDifference());
            switch(h->getType())
            {
                case BRIEF:
                {
                    double diff;

                    BriefHeuristic* bh = (BriefHeuristic*) heuristics[l];
                    if(x<0 || x>=globalMaps[mapID].cols || y<0 || y>=globalMaps[mapID].rows)
                    {
                        prob=0;
                    }
                    else
                    {
                        if(multispectralCam)
                        {
                            diff = bh->calculateValue2(particles[i].p, &globalMaps[mapID], redMap, nirMap, regMap, greMap, currentMapOriginal, droneAngles[currentImage]);                            
//                            if(experimentosRollPitchEstimation && !flagAngEstimate)
//                            {
//                                bh->abBriefParicleProb(realPose, CorrectedGT[currentImage], &globalMaps[mapID], redMap, nirMap, regMap, greMap, currentMapOriginal, droneAngles[currentImage]);
//                                flagAngEstimate = true;
//                            }
                        }
                        else
                        {
                            diff = bh->calculateValue2(particles[i].p, &globalMaps[mapID]);
                        }
                        prob *= (1.0/(sqrt(varBrief*2.0*M_PI))*exp(-0.5*(pow(diff,2.0)/varBrief)));
                        //prob /= max((0-contador), 1);

                        //double maxProbGauss = 1;
                        //maxProbGauss *= (1.0/(sqrt(varBrief*2.0*M_PI))*exp(-0.5*(pow(0,2.0)/varBrief)));
                    }

                    //prob = diff;

                    break;
                }
                case SSD:
                {
                    cv::Mat z_particle = Utils::getRotatedROIFromImage(particles[i].p, z_robot.size(), globalMaps[0]);
                    prob = Utils::matchImages(z_robot,z_particle,CV_TM_CCORR_NORMED);
                    break;
                }
                case COLOR_ONLY:
                {
                    ColorHeuristic* ch = (ColorHeuristic*) heuristics[l];
                    if(x<0 || x>=globalMaps[mapID].cols || y<0 || y>=globalMaps[mapID].rows){
                        prob = 0;
                        break;
                    }
                    /// compute color difference
                    double diff  = ch->calculateValue(x,y,&globalMaps[mapID]);

                    /// Gaussian weighing
                    if(diff!=HEURISTIC_UNDEFINED)
                        prob *= 1.0/(sqrt(2*M_PI*varColor))*exp(-0.5*(pow(diff,2)/varColor));
                        //prob *= 1.0/cosh(pow(diff,4.0));//(sqrt(2*M_PI*varColor))*exp(-0.5*(pow(diff,2)/varColor));

                    else
                      prob *= 0.00000000000000000000000000000000000000000001;
//                    if(prob<=0.001){
//                        cout << " " << diff << "   " << prob << endl;
//                    }
                    break;
                }
                case UNSCENTED_COLOR:
                {
//                    cout << "Color diff: " << mapID << endl;

                    UnscentedColorHeuristic* uch = (UnscentedColorHeuristic*) heuristics[l];
                    if(x<uch->deltax*4 || x>=globalMaps[mapID].cols-uch->deltax*4 || y<uch->deltay*4 || y>=globalMaps[mapID].rows-uch->deltay*4){
                        prob = 0;
                        break;
                    }
                    /// compute color difference
                    double diff  = uch->calculateValue(x,y,particles[i].p, &globalMaps[mapID]);

                    /// Gaussian weighing
                    if(diff!=HEURISTIC_UNDEFINED)
                        //prob *= 1.0/cosh(pow(diff,4.0));//(sqrt(2*M_PI*varColor))*exp(-0.5*(pow(diff,2)/varColor));

                        prob *= 1.0/(sqrt(2*M_PI*varColor))*exp(-0.5*(pow(diff,2)/varUnscented));
                    else
                        prob *= 0.00000000000000000000000000000000000000000001;//1.0/(numParticles);
                    break;
                }
                case DENSITY:
                {
                    /// Gaussian weighing
                    if(heuristicValues[l]!=HEURISTIC_UNDEFINED)
                        prob *= 1.0/(sqrt(2*M_PI*varDensity))*exp(-0.5*(pow((cachedMaps[l]->getPureHeuristicValue(x,y)-heuristicValues[l]),2)/varDensity));

                    if(heuristicValues[l]==HEURISTIC_UNDEFINED || cachedMaps[l]->getHeuristicValue(x,y)==HEURISTIC_UNDEFINED)
                        prob *= 1.0/numParticles;
                    break;
                }
                case MEAN_SHIFT:
                {
                    double val  = h->calculateValue(x,y,&globalMaps[mapID]);
                    prob *= 1.0/(sqrt(2*M_PI*varMeanShift))*exp(-0.5*(pow(val,2)/varMeanShift));
                    break;
                }
                case SIFT_MCL:
                {
                    prob *= h->calculateValue(x,y,NULL);
                    break;
                }
                case ENTROPY:
                {
                    /// Gaussian weighing
                    if(heuristicValues[l]!=HEURISTIC_UNDEFINED)
                        prob *= 1.0/(sqrt(2*M_PI*varEntropy))*exp(-0.5*(pow((cachedMaps[l]->getPureHeuristicValue(x,y)-heuristicValues[l]),2)/varEntropy));

                    if(heuristicValues[l]==HEURISTIC_UNDEFINED || cachedMaps[l]->getPureHeuristicValue(x,y)==HEURISTIC_UNDEFINED)
                        prob *= 0.000000000000000000000000001;
                    break;
                }
                case MUTUAL_INFORMATION:
                {
                    //prob = 1;
                    //break;
                    double diff;

                    MutualInfHeuristic* mih = (MutualInfHeuristic*) heuristics[l];
                    if(x<0 || x>=globalMaps[mapID].cols || y<0 || y>=globalMaps[mapID].rows){
                        prob=0;
                    }
                    else{

                        diff = mih->calculateValue2(particles[i].p,frameColorConverted[mapID],&globalMaps[mapID]);
                        prob *= (1.0/(sqrt(varMI*2.0*M_PI))*exp(-0.5*(pow(diff,2.0)/varMI)));
//                        cout<<"PROB NO W:"<<prob<<" DIFF:"<<diff<<endl;
                    }



//                    if(heuristicValues[l]!=HEURISTIC_UNDEFINED)
//                    {
//                        // Get cashed entropy
//                        mih->setCashedEntropy(cachedMaps[l]->getPureHeuristicValue(x,y));
//                        double val=mih->calculateValue(x, y,
//                                    globalMaps[mapID],
//                                    &globalMaps[globalMaps.size()-1], // Mask
//                                    &frameColorConverted[mapID],
//                                    &binaryFrameMask,
//                                    particles[i].p);

//                        prob *= 1.0/(sqrt(2*M_PI*varMI))*exp(-0.5*(pow(1-val,2)/varMI));
//                        //prob *= 1+val;

//                    }
//                    if(heuristicValues[l]==HEURISTIC_UNDEFINED || cachedMaps[l]->getPureHeuristicValue(x,y)==HEURISTIC_UNDEFINED)
//                        prob *= 0.000000000000000000000000001;
                    break;
                }
            case HISTOGRAM_MATCHING:
                {
                    if(heuristicValues[l]!=HEURISTIC_UNDEFINED)
                    {
                        HistogramHeuristic* hh = (HistogramHeuristic*) heuristics[l];
                        double distance =hh->calculateValue(x, y,
                                                            &globalMaps[mapID],
                                                            &globalMaps[globalMaps.size()-1]);
//                        if(distance == 0)
//                            distance = 1.0/particles.size();
//                        else
                            prob *= 1.0/(sqrt(2*M_PI*varHistogram))*exp(-0.5*(pow(distance,2)/varHistogram));
                    }
                }
            }
        }
        //2.6596152026762176) (CONFERIR)é o valor equivalente a um match perfeito, levando em consideração a minha gaussiana
        particles[i].w = prob;
        //particles[i].w = Utils::Normalize(prob, percentualPxVegImg, 1, 0, 2.6596152026762176);

        //if(contador< 10)
        //if(percentualPxVegImg > 15)
        //    particles[i].w = sqrt(particles[i].w);

        //a raiz quadrada tem efeito opsoto para números entre 0 e 1, aumenta a distância, poia raiz de um número nesse intervalo é maior do q ele mesmo
        //particles[i].w = (percentualPxVegImg * sqrt(particles[i].w)) + ((1-percentualPxVegImg)* particles[i].w);
    }

    t = (double)cv::getTickCount() - t;
    cout << endl << "Particles calculated in: " << (t*1000./cv::getTickFrequency())/1000 << "s" << endl;
    cout << "Matei: " << count << " partículas." << endl;

    /// FALTA ARRUMAR ESSA FUNCAO
    //discardInvalidDeltaAngles(u,gradients);

    double sumWeights = 0.0;
    for(int i=0; i<particles.size(); i++){
        sumWeights += particles[i].w;
    }
    cout << "SumWeights B " << sumWeights << endl;

    // Correct zero error
    if(sumWeights==0.0)
    {
        for(int i=0; i<particles.size(); i++) {
            if(heuristics[0]->getType() != MUTUAL_INFORMATION && heuristics[0]->getType() != BRIEF && heuristics[0]->getType() != SSD && heuristics[0]->getType() != COLOR_ONLY && heuristics[0]->getType() != UNSCENTED_COLOR && heuristics[0]->getType() != HISTOGRAM_MATCHING)
            {

                // check if particle is valid (known and not obstacle)
                if(!cachedMaps[0]->isKnown((int)particles[i].p.x,(int)particles[i].p.y) ||
                        cachedMaps[0]->isObstacle((int)particles[i].p.x,(int)particles[i].p.y)) {
                    particles[i].w = 0.0;
                }
                else {
                    particles[i].w = 1.0;
                    sumWeights+=1.0;
                }
            }
            else
            {
                if(particles[i].p.x<0 || particles[i].p.y<0 || particles[i].p.y>=this->globalMaps[0].rows
                        || particles[i].p.x>=this->globalMaps[0].cols )
                    particles[i].w = 0.0;
                else{
                    particles[i].w = 1.0;
                    sumWeights+=1.0;
                 }
            }
        }
    }

    count = 0; //number of particles dead
    neff = 0;  //número de partículas efetivas
    sumW = sumWeights;
    //normalize particles
    if(sumWeights!=0.0)
        for(int i=0; i<particles.size(); i++){
            particles[i].w /= sumWeights;

            if(particles[i].w == 0.0)
                count++;

            neff+=pow(particles[i].w,2.0);
        }
    else {
        for(int i=0; i<particles.size(); i++)
            particles[i].w = 1.0/numParticles;
        neff=numParticles;
    }
    cout << "Confirmando, matei: " << count << " partículas." << endl;
    neff=1.0/neff;
    cout << "NEFF: " << neff << endl;

    if(!quiet)
    {
        //Tempo médio do cálcuo das  partículas
        cout << "\nTempo médio do cálculo das partículas(tempo por partícula): " << tmpExecPxCalc/qtdChamadasPxCalc << " s \n";
        //cout << "\nTempo médio do cálculo das partículas(projeção das "<< numParticles <<"): " << (tmpExecPxCalc/qtdChamadasPxCalc)*numParticles << " s \n"; está errado
    }
}

void MCL::resampling()
{
    cout<<"RESAMPLING"<<endl;
    vector<int> children;
    children.resize(numParticles,0);

    // low variance sampler (table 4.4 in Thrun, 2005)

    // construct a trivial random generator engine from a time-based seed:
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator (seed);
//    std::default_random_engine generator;

    std::uniform_real_distribution<double> randomValue(0.0,1.0/((double)numParticles));

    int i=0;
    double r=randomValue(generator);
    double c=particles[i].w;

    float averageScale = 0;
    float averageRoll = 0;
    float averagePitch = 0;
    float averageYaw = 0;

    for(int m=0; m<particles.size(); m++){
        double U = r + m*1.0/(double)numParticles;
        while(U>c){
            i++;
            c+=particles[i].w;
        }
        children[i]++;
        averageScale += particles[i].p.z;
        averageRoll += particles[i].p.roll;
        averagePitch += particles[i].p.pitch;
        averageYaw += particles[i].p.yaw;
    }

    cout<<"                  AverageScale:"<<averageScale/particles.size()<<"                 "<<endl;
    cout<< std::fixed<< std::setprecision(5)<<"                  AverageRoll:"<<(averageRoll/particles.size()) * (180/M_PI)<<" deg                 "<<endl;
    cout<< std::fixed<< std::setprecision(5)<<"                  AveragePitch:"<<(averagePitch/particles.size()) * (180/M_PI)<<" deg                 "<<endl;
    cout<< std::fixed<< std::setprecision(5)<<"                  AverageYaw:"<<(averageYaw/particles.size())* (180/M_PI)<<" deg                 "<<endl;
    cout<<" ------------------------------------------------------"<<endl;

    // generate children from current particles
    vector<MCLparticle> nextGeneration;

    if(removeDuplicates){
        std::set<pair<int,int> > particlesSet;
        std::set<pair<int,int> >::iterator it;

        for(int m=0; m<particles.size(); m++){
            if(children[m]>0){
                pair<int,int> p(particles[m].p.x,particles[m].p.y);
                if(particlesSet.find(p) == particlesSet.end()){
                    nextGeneration.push_back(particles[m]);
                    particlesSet.insert(p);
                }
            }
        }
    }else{
        //    cout << "Children ";
            for(int m=0; m<particles.size(); m++){
        //        //cout << children[m] << ' ';
                for(int c=0; c<children[m]; c++)
                    nextGeneration.push_back(particles[m]);
            }
    }

//    cout << " size nextGeneration " << nextGeneration.size();



    particles = nextGeneration;

//    double sumweights=0.0;

//    for(int m=0; m<particles.size(); m++){
//           sumweights+= particles[m].w;
//    }
//    for(int m=0; m<particles.size(); m++){
//           particles[m].w= particles[m].w/sumweights;
//    }

}

MCLparticle MCL::resamplingKLD(){
    double totalWeight=0.0;
    for(int k=0;k<particles.size();k++){
        totalWeight += particles[k].w;
    }

    if(totalWeight==0.0) cout<<"Resampling KLD -  totalWeight is 0"<<endl;

    //GENERATING THE NEW SAMPLE
    int l;
    double r= (double)rand() / RAND_MAX;
    l=-1;
    do{
        l++;
        if(l>particles.size()-1){
            l=particles.size()-1;
        }
        r=r-particles[l].w;
    } while(r>0.0);
    return particles[l];
}

////////////////////////
// Métodos Auxiliares //
////////////////////////

void MCL::discardInvalidDeltaAngles(Pose3d &u, vector<double> &gradients)
{
    if(gradients.size()<=1)
        return;

    vector<bool> keep(particles.size(),false);

    // for each density map
    for(int h=0; h<gradients.size(); h++){
        if(IS_UNDEF(gradients[h]))
            continue;

        double deltaRobot = u.yaw - gradients[h];
//        cout << " robot " << RAD2DEG(u.theta) << " - " << RAD2DEG(gradients[h]) << " = " << RAD2DEG(deltaRobot) << endl;

        // Evaluate all particles
        for(int i=0; i<particles.size(); i++){
            if(particles[i].w == 0.0)
                continue;

            double grad = cachedMaps[h]->getOrientation(particles[i].p.x,particles[i].p.y);
            if(IS_UNDEF(grad)){
                continue;
            }
//            cout << " particle (" << i << ") " << RAD2DEG(particles[i].p.theta) << " - " << RAD2DEG(grad);
            grad = particles[i].p.yaw - grad;
//            cout << " = " << RAD2DEG(grad);

            double delta = acos(cos(grad)*cos(deltaRobot) + sin(grad)*sin(deltaRobot));
            if(RAD2DEG(delta)<5.0)
                keep[i]=true;
//            cout << " DELTA: " << RAD2DEG(delta) << " w " << 0.5 + 0.5*cos(delta) << endl;

//            particles[i].w *= 0.5 + 0.5*cos(delta);
        }
    }

    int count=0;
    for(int i=0; i<particles.size(); i++){
        if(keep[i])
            count++;
    }

    if(count<particles.size()/4) // Avoid discard if robot cagated
        return;

    for(int i=0; i<particles.size(); i++){
        if(!keep[i])
            particles[i].w = 0.0;
    }


}

double MCL::computeNeff()
{
    double Neff=0.0;
    for(int i=0; i<particles.size(); i++)
        Neff += particles[i].w * particles[i].w;
    return 1.0/Neff;
}

double MCL::computeError(double trueX, double trueY,double particleX, double particleY)
{
    return sqrt(pow(trueX-particleX, 2.0) + pow(trueY-particleY, 2.0));
}

double MCL::computeError3d(double trueX, double trueY, double trueZ, double particleX, double particleY, double particleZ)
{
    return sqrt(pow(trueX-particleX, 2.0) + pow(trueY-particleY, 2.0) + pow(trueZ-particleZ, 2.0));
}

double MCL::computeAngleError(double trueTh, double particleTh)
{
    //cout <<  "trueTh: " << trueTh << " " "particleTh: " << particleTh << endl;
    //return acos(cos(trueTh)*cos(particleTh)+sin(trueTh)*sin(particleTh));
    double angleError=particleTh-trueTh;

    while (angleError > M_PI)
        angleError -= 2*M_PI;
    while (angleError < -M_PI)
        angleError += 2*M_PI;

    // error in radians
    return angleError;

}
double MCL::sumAngles(double a, double b)
{
    //cout <<  "trueTh: " << trueTh << " " "particleTh: " << particleTh << endl;
    //return acos(cos(trueTh)*cos(particleTh)+sin(trueTh)*sin(particleTh));
    double c = a + b;

    while (c > M_PI)
        c -= 2*M_PI;
    while (c < -M_PI)
        c += 2*M_PI;

    // error in radians
    return c;

}
void MCL::prepareWeighting(cv::Mat &z, cv::Mat &redMap, cv::Mat &nirMap, cv::Mat &regMap, cv::Mat &greMap, cv::Mat &currentMapOriginal, Pose3d orientacao)
{
    // precomputing halfRows e halfCows
    int halfRows = frameColorConverted[0].rows/2;
    int halfCols = frameColorConverted[0].cols/2;


    if(experimentosRollPitchEstimation)
    {
        BriefHeuristic* bh = (BriefHeuristic*) heuristics[0];
        bh->abBriefParicleProb(realPose, CorrectedGT[currentImage], &globalMaps[2], redMap, nirMap, regMap, greMap, currentMapOriginal, droneAngles[currentImage]);
    }

    // Set mask if not set yet
    if(!binaryFrameMask.data)
        binaryFrameMask = cv::Mat(frameColorConverted[0].cols,
                              frameColorConverted[0].rows,
                              CV_8SC3,cv::Scalar(0,0,0));


    // Compute value at the center of the frame using
    // appropriate color space
    for(int c = 0; c<heuristics.size();++c)
    {
        Heuristic* h = heuristics[c];

        // get proper color space
        int mapID = selectMapID(h->getColorDifference());


        cout << "Color Difference: "<< h->getColorDifference() << "Color Map: " << mapID << endl;

        double val = 0.0;
        double grad = val;

        BriefHeuristic* bh = (BriefHeuristic*) heuristics[c];
        bh->updateDroneDescriptor(frameColorConverted[mapID], redMap, nirMap, regMap, greMap, currentMapOriginal, orientacao, realPose);
    }
}

void MCL::prepareWeighting(cv::Mat &z)
{
    // precomputing halfRows e halfCows
    int halfRows = frameColorConverted[0].rows/2;
    int halfCols = frameColorConverted[0].cols/2;

    // Set mask if not set yet
    if(!binaryFrameMask.data)
        binaryFrameMask = cv::Mat(frameColorConverted[0].cols,
                              frameColorConverted[0].rows,
                              CV_8SC3,cv::Scalar(0,0,0));


    // Compute value at the center of the frame using
    // appropriate color space    
    for(int c = 0; c<heuristics.size();++c)
    {
        Heuristic* h = heuristics[c];

        // get proper color space
        int mapID = selectMapID(h->getColorDifference());


        cout << "Color Difference: "<< h->getColorDifference() << "Color Map: " << mapID << endl;

        double val = 0.0;
        double grad = val;

        switch(h->getType())
        {
        case SSD:
            break;
        case BRIEF:
            {

            BriefHeuristic* bh = (BriefHeuristic*) heuristics[c];            
            bh->updateDroneDescriptor(frameColorConverted[mapID]);
            break;
            }
        case COLOR_ONLY:
            {
            ColorHeuristic* ch = (ColorHeuristic*) heuristics[c];
            ch->setBaselineColor(halfCols,
                                 halfRows,
                                 &frameColorConverted[mapID]);
            break;
            }
        case UNSCENTED_COLOR:
            {
            UnscentedColorHeuristic* uch = (UnscentedColorHeuristic*) heuristics[c];
            uch->setBaselineColors(halfCols,
                                 halfRows,
                                 &frameColorConverted[mapID]);
            break;
            }
        case DENSITY:
        case ENTROPY:
            {
            // create discrete density value according to the corresonding mapgrid
            heuristicValues[c] = heuristics[c]->calculateValue(
                        halfCols,
                        halfRows,
                        &frameColorConverted[mapID], &binaryFrameMask);
            // and do the same for the angles
            heuristicGradients[c] = heuristics[c]->calculateGradientSobelOrientation(
                        halfCols,
                        halfRows,
                        &frameColorConverted[mapID], &binaryFrameMask);
            break;
            }
        case MUTUAL_INFORMATION:
            {
            MutualInfHeuristic* mih = (MutualInfHeuristic*) heuristics[c];
            mih->updateDroneDescriptor(frameColorConverted[mapID]);
//            MIHeuristic* mih = (MIHeuristic*) heuristics[c];
//            // create discrete density value according to the corresonding mapgrid
//            mih->setObservedEntropy(
//                        halfCols,
//                        halfRows,
//                        &frameColorConverted[mapID], &binaryFrameMask);
//            heuristicValues[c] = mih->getObservedEntropy();
//            // and do the same for the angles
//            heuristicGradients[c] = heuristics[c]->calculateGradientSobelOrientation(
//                        halfCols,
//                        halfRows,
//                        &frameColorConverted[mapID], &binaryFrameMask);
            break;
            }
        case MEAN_SHIFT:
            {
            MeanShiftHeuristic* msh = (MeanShiftHeuristic*) heuristics[c];
            msh->updateSimilarityMap(z,globalMaps[3]);

            for(int i=0; i<particles.size(); i++){
                Pose delta = msh->computeMeanShift(particles[i].p.x,particles[i].p.y);
                particles[i].p.x += delta.x;
                particles[i].p.y += delta.y;
            }

            break;
            }
        case SIFT_MCL:
            {
            SIFTHeuristic* sifth = (SIFTHeuristic*) heuristics[c];
            sifth->updateMatcher(z);
            break;
        }
        case HISTOGRAM_MATCHING:
            {
                HistogramHeuristic* hh = (HistogramHeuristic*) heuristics[c];
                heuristicValues[c] = hh->setObservedHistogram(    halfCols,
                                             halfRows,
                                             &frameColorConverted[mapID],
                                             &binaryFrameMask);
                break;
            }
        }
    }
}

void MCL::createColorVersions(cv::Mat& imageRGB)
{
    // RGB
    frameColorConverted[0]=imageRGB.clone();

    // INTENSITYC:
    cvtColor(imageRGB, frameColorConverted[1],CV_BGR2GRAY);
    cvtColor(frameColorConverted[1], frameColorConverted[1],CV_GRAY2BGR);

    // CIELAB1976 || CIELAB1994 || CMCLAB1984 || CIELAB2000 || CIELAB1994MIX || CIELAB2000MIX
    cvtColor(imageRGB,frameColorConverted[2], CV_BGR2Lab);

}

void MCL::normalizingParticles(){
    double sumWeights = 0.0;
    for(int i=0; i < particles.size(); i++)
        sumWeights += particles[i].w;

    for(int i=0; i < particles.size(); i++)
        particles[i].w /= sumWeights;
}


void MCL::buildTableZ(){
    float tmp;
    ifstream ifile;
    ifile.open("/home/phi/Documents/Workspace/phir2framework/ztable.data");
    //ifile.open("/home/raziel/Documents/Workspace/phir2framework/ztable.data");

    if(!ifile.is_open())//Procurar no diretório do executável
        ifile.open("ztable.data");

    if(ifile.is_open())
    {
        while(!ifile.eof())
        {
            ifile >> tmp;
            ztable.push_back(tmp);
        }
    }
    else
    {
        cout<<"ERROR - FILE ztable.data isn't open"<<endl;
        exit(-1);
    }
}

void MCL::setingEmptyBins(){
    bins.clear();
    for(int i = 0; i < globalMaps[0].cols;i++){
        vector<bool> temp;
        for(int j = 0; j < globalMaps[0].rows;j++){
            temp.push_back(true);
        }
        bins.push_back(temp);
    }
}

bool MCL::isEmpty(MCLparticle part){
    if(bins[(int)part.p.x][(int)part.p.y]){ //THE bin(x,y) IS EMPTY
        bins[(int)part.p.x][(int)part.p.y] = false;
        return true;
    }else{                        //THE bin(x,y) IS NON-EMPTY
        return false;
    }
}

float MCL::getingZValue(float confidence){
    float zvalue = 4.1;
    for(int i = 0; i < ztable.size(); i++){
        if(ztable[i] >= confidence){
            zvalue = i/100.00;
            break;
        }
    }
    return zvalue;
}

bool MCL::verifyParticlePosition(MCLparticle part){
    if(part.p.x >= 0 && part.p.x <= globalMaps[0].cols && part.p.y >= 0 && part.p.y <= globalMaps[0].rows){
//        cout<<"POSICAO VALIDA ----------------------------------------------"<<endl;
        return false;
    }else{
//        cout<<"POSICAO INVALIDA **********************************************"<<endl;
        return true;
    }
}

//Compensa a posiçao de todas as partículas, baseado nos ângulos (roll, pitch e yaw) do drone naquele momento(IMU).
void MCL::compensateCloudPosition()
{
    if(compensateParticles)
    {
        // generate initial set
        for(int i = 0; i<particles.size(); i++)
        {
            compensateParticlePosition(particles[i]);
        }
    }
}

//Compensa a posiçao de uma partícula, baseado nos ângulos (roll, pitch e yaw) do drone naquele momento(IMU).
void MCL::compensateParticlePosition(MCLparticle part)
{
    /* %UAV Coordinate Frames and Rigid Body Dynamics (vehicle to body transformation)
        %Rot =   [(cos(pitch)*cos(yaw))                             (cos(pitch)*sin(yaw))                               -sin(pitch)             0
        %        (sin(roll)*sin(pitch)*cos(yaw)-cos(roll)*sin(yaw)) (sin(roll)*sin(pitch)*sin(yaw)+cos(roll)*cos(yaw))	(sin(roll)*cos(pitch))  0
        %        (cos(roll)*sin(pitch)*cos(yaw)+sin(roll)*sin(yaw))	(cos(roll)*sin(pitch)*sin(yaw)-sin(roll)*cos(yaw))	(cos(roll)*cos(pitch))  0
        %        0                                                   0                                                   0                       1];

    xtst = x*(cos(pitch)*cos(yaw)) + y*(sin(roll)*sin(pitch)*cos(yaw)-cos(roll)*sin(yaw)) + z*(cos(roll)*sin(pitch)*cos(yaw)+sin(roll)*sin(yaw));
    ytst = x*(cos(pitch)*sin(yaw)) + y*(sin(roll)*sin(pitch)*sin(yaw)+cos(roll)*cos(yaw)) + z*(cos(roll)*sin(pitch)*sin(yaw)-sin(roll)*cos(yaw));
    ztst = x*-sin(pitch) + y*(sin(roll)*cos(pitch)) + z*(cos(roll)*cos(pitch));
*/
    Pose3d orientacao = droneAngles[currentImage];
    double dir[4] = {0, 0, (-part.p.z /0.366197183) * (multExcalaZ), 0};//Direcao inicial (Apontando p baixo)[x y z 0], e localizado na origem
    double dirCamera[4] = {1, 0, 0, 0};

    //Mover o ponto de origem do centro do drone para a posição da câmera
    part.p.x += dirCamera[0]*(cos(orientacao.pitch)*cos(orientacao.yawInvertido)) + dirCamera[1]*(sin(orientacao.roll)*sin(orientacao.pitch)*cos(orientacao.yawInvertido)-cos(orientacao.roll)*sin(orientacao.yawInvertido)) + dirCamera[2]*(cos(orientacao.roll)*sin(orientacao.pitch)*cos(orientacao.yawInvertido)+sin(orientacao.roll)*sin(orientacao.yawInvertido));
    part.p.y += dirCamera[0]*(cos(orientacao.pitch)*sin(orientacao.yawInvertido)) + dirCamera[1]*(sin(orientacao.roll)*sin(orientacao.pitch)*sin(orientacao.yawInvertido)+cos(orientacao.roll)*cos(orientacao.yawInvertido)) + dirCamera[2]*(cos(orientacao.roll)*sin(orientacao.pitch)*sin(orientacao.yawInvertido)-sin(orientacao.roll)*cos(orientacao.yawInvertido));
    part.p.z += ((dirCamera[0]*-sin(orientacao.pitch) + dirCamera[1]*(sin(orientacao.roll)*cos(orientacao.pitch)) + dirCamera[2]*(cos(orientacao.roll)*cos(orientacao.pitch)))) * 0,366197183 / 45*0,188679245;

    //Rotacionar a reta do drone até o chão
    part.p.x += dir[0]*(cos(orientacao.pitch)*cos(orientacao.yawInvertido)) + dir[1]*(sin(orientacao.roll)*sin(orientacao.pitch)*cos(orientacao.yawInvertido)-cos(orientacao.roll)*sin(orientacao.yawInvertido)) + dir[2]*(cos(orientacao.roll)*sin(orientacao.pitch)*cos(orientacao.yawInvertido)+sin(orientacao.roll)*sin(orientacao.yawInvertido));
    part.p.y += dir[0]*(cos(orientacao.pitch)*sin(orientacao.yawInvertido)) + dir[1]*(sin(orientacao.roll)*sin(orientacao.pitch)*sin(orientacao.yawInvertido)+cos(orientacao.roll)*cos(orientacao.yawInvertido)) + dir[2]*(cos(orientacao.roll)*sin(orientacao.pitch)*sin(orientacao.yawInvertido)-sin(orientacao.roll)*cos(orientacao.yawInvertido));
    part.p.z += ((dir[0]*-sin(orientacao.pitch) + dir[1]*(sin(orientacao.roll)*cos(orientacao.pitch)) + dir[2]*(cos(orientacao.roll)*cos(orientacao.pitch)))) * 0,366197183 / 45*0,188679245;

    //teste
    /*float zAntPx = (realPose.z/0.366197183) * (multExcalaZ);
    //Mover o ponto de origem do centro do drone para a posição da câmera
    int xCamera = realPose.x + dirCamera[0]*(cos(orientacao.pitch)*cos(orientacao.yaw)) + dirCamera[1]*(sin(orientacao.roll)*sin(orientacao.pitch)*cos(orientacao.yaw)-cos(orientacao.roll)*sin(orientacao.yaw)) + dirCamera[2]*(cos(orientacao.roll)*sin(orientacao.pitch)*cos(orientacao.yaw)+sin(orientacao.roll)*sin(orientacao.yaw));
    int yCamera = realPose.y + dirCamera[0]*(cos(orientacao.pitch)*sin(orientacao.yaw)) + dirCamera[1]*(sin(orientacao.roll)*sin(orientacao.pitch)*sin(orientacao.yaw)+cos(orientacao.roll)*cos(orientacao.yaw)) + dirCamera[2]*(cos(orientacao.roll)*sin(orientacao.pitch)*sin(orientacao.yaw)-sin(orientacao.roll)*cos(orientacao.yaw));
    int zCamera = zAntPx + dirCamera[0]*-sin(orientacao.pitch) + dirCamera[1]*(sin(orientacao.roll)*cos(orientacao.pitch)) + dirCamera[2]*(cos(orientacao.roll)*cos(orientacao.pitch));
    //Rotacionar a reta do drone até o chão
    xCamera = xCamera + dir[0]*(cos(orientacao.pitch)*cos(orientacao.yaw)) + dir[1]*(sin(orientacao.roll)*sin(orientacao.pitch)*cos(orientacao.yaw)-cos(orientacao.roll)*sin(orientacao.yaw)) + zAntPx*(cos(orientacao.roll)*sin(orientacao.pitch)*cos(orientacao.yaw)+sin(orientacao.roll)*sin(orientacao.yaw));
    yCamera = yCamera + dir[0]*(cos(orientacao.pitch)*sin(orientacao.yaw)) + dir[1]*(sin(orientacao.roll)*sin(orientacao.pitch)*sin(orientacao.yaw)+cos(orientacao.roll)*cos(orientacao.yaw)) + zAntPx*(cos(orientacao.roll)*sin(orientacao.pitch)*sin(orientacao.yaw)-sin(orientacao.roll)*cos(orientacao.yaw));
    zCamera = zCamera + dir[0]*-sin(orientacao.pitch) + dir[1]*(sin(orientacao.roll)*cos(orientacao.pitch)) + zAntPx*(cos(orientacao.roll)*cos(orientacao.pitch));
    */
}
