#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/video.hpp>
#include "opencv2/highgui/highgui.hpp"
#include <opencv2/opencv.hpp>
#include "opencv2/xfeatures2d.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/core/matx.hpp"
#include "opencv2/core/types.hpp"
#include "opencv2/core/bufferpool.hpp"
#include "opencv2/core/mat.inl.hpp"


using namespace std;
using namespace cv;
using namespace cv::xfeatures2d;

const int MAX_FEATURES = 500;
const float GOOD_MATCH_PERCENT = 0.15f;
const int nPixelX = 1280; //pixels - along X axis  1280
const int nPixelY = 960; //pixels - along Y axis  960

/*  Feature detectors
    "FAST" – FastFeatureDetector
    "STAR" – StarFeatureDetector
    "SIFT" – SIFT (nonfree module)
    "SURF" – SURF (nonfree module)
    "ORB" – ORB
    "BRISK" – BRISK
    "MSER" – MSER
    "GFTT" – GoodFeaturesToTrackDetector
    "HARRIS" – GoodFeaturesToTrackDetector with Harris detector enabled
    "Dense" – DenseFeatureDetector
    "SimpleBlob" – SimpleBlobDetector
*/

void alignImagesORB(Mat &im1, Mat &im2, Mat &im1Reg, Mat &h)
{
  Mat vecXd(cv::Size(nPixelX * nPixelY, 1), CV_32F, Scalar(0));
  Mat vecYd(cv::Size(nPixelX * nPixelY, 1), CV_32F, Scalar(0));

  // Convert images to grayscale
  Mat im1Gray, im2Gray;
  cvtColor(im1, im1Gray, CV_BGR2GRAY);
  cvtColor(im2, im2Gray, CV_BGR2GRAY);

  // Variables to store keypoints and descriptors
  std::vector<KeyPoint> keypoints1, keypoints2;
  Mat descriptors1, descriptors2;

  //ORB
  // Detect ORB features and compute descriptors.
  Ptr<Feature2D> orb = ORB::create(MAX_FEATURES);
  orb->detectAndCompute(im1Gray, Mat(), keypoints1, descriptors1);
  orb->detectAndCompute(im2Gray, Mat(), keypoints2, descriptors2);

  // Match features.
  std::vector<DMatch> matches;
  Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create("BruteForce-Hamming");
  matcher->match(descriptors1, descriptors2, matches, Mat());

  // Sort matches by score
  std::sort(matches.begin(), matches.end());

  // Remove not so good matches
  const int numGoodMatches = matches.size() * GOOD_MATCH_PERCENT;
  matches.erase(matches.begin()+numGoodMatches, matches.end());


  // Draw top matches
  Mat imMatches;
  drawMatches(im1, keypoints1, im2, keypoints2, matches, imMatches);
  imwrite("/home/raziel/images/undistorted_opencv/matches.jpg", imMatches);
  imshow("Matches", imMatches);

  // Extract location of good matches
  std::vector<Point2f> points1, points2;

  for( size_t i = 0; i < matches.size(); i++ )
  {
    points1.push_back( keypoints1[ matches[i].queryIdx ].pt );
    points2.push_back( keypoints2[ matches[i].trainIdx ].pt );
  }

  // Find homography
  h = findHomography( points1, points2, RANSAC );

  // Use homography to warp image
  warpPerspective(im1, im1Reg, h, im2.size());
}

void alignImagesSIFT(Mat &imSlave, Mat &imMaster, Mat &imSlaveReg, Mat &h)
{
  Mat vecXd(cv::Size(nPixelX * nPixelY, 1), CV_32F, Scalar(0));
  Mat vecYd(cv::Size(nPixelX * nPixelY, 1), CV_32F, Scalar(0));

  // Convert images to grayscale
  Mat imSlaveGray, imMasterGray;
  cvtColor(imSlave, imSlaveGray, CV_GRAY2BGR565);
  cvtColor(imMaster, imMasterGray, CV_GRAY2BGR565);
  //cvtColor(imSlave, imSlaveGray, CV_BGR2BGR555);
  //cvtColor(imMaster, imMasterGray, CV_BGR2BGR555);

  // Variables to store keypoints and descriptors
  std::vector<KeyPoint> keypointsSlave, keypointsMaster;
  Mat descriptorsSlave, descriptorsMaster;

  //SIFT
  Ptr<Feature2D> sift = SIFT::create();
  sift->detectAndCompute(imSlaveGray, Mat(), keypointsSlave, descriptorsSlave);
  sift->detectAndCompute(imMasterGray, Mat(), keypointsMaster, descriptorsMaster);

  // Match features.
  //Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create("BruteForce-Hamming");//DescriptorMatcher::FLANNBASED
  Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create(DescriptorMatcher::FLANNBASED);
  std::vector< std::vector<DMatch> > knn_matches;
  matcher->knnMatch( descriptorsSlave, descriptorsMaster, knn_matches, 2 );

  //-- Filter matches using the Lowe's ratio test
  const float ratio_thresh = 0.7f;
  std::vector<DMatch> good_matches;
  for (size_t i = 0; i < knn_matches.size(); i++)
  {
      if (knn_matches[i][0].distance < ratio_thresh * knn_matches[i][1].distance)
      {
          good_matches.push_back(knn_matches[i][0]);
      }
  }

  //Desenhar os matches
  Mat img_matches;
  drawMatches( imSlave, keypointsSlave, imMaster, keypointsMaster, good_matches, img_matches, Scalar::all(-1),
               Scalar::all(-1), std::vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS );
  //Exibir os matches
  imshow("Good Matches", img_matches );
  waitKey(0);

  std::vector<Point2f> pointsSlave, pointsMaster;

  for( size_t i = 0; i < good_matches.size(); i++ )
  {
    pointsSlave.push_back( keypointsSlave[ good_matches[i].queryIdx ].pt );
    pointsMaster.push_back( keypointsMaster[ good_matches[i].trainIdx ].pt );
  }

  // Find homography
  h = findHomography( pointsSlave, pointsMaster, RANSAC );

  // Use homography to warp image
  warpPerspective(imSlave, imSlaveReg, h, imMaster.size());

  //imshow("Imagem alinhada (SIFT)", imSlaveReg);
  //waitKey();

  imSlave = imSlaveReg;
}
