/* INCLUDES FOR THIS PROJECT */
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <limits>
#include <opencv2/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/xfeatures2d.hpp>
#include <opencv2/xfeatures2d/nonfree.hpp>

#include "dataStructures.h"
#include "matching2D.hpp"


// #define ITERATE     // with this you request full iteration which generates the result.txt file used to generate the diagram


// global variables
double gDescKeyPointsExectime_ms;   // contains exec time of last descriptor extraction in ms
double gKeypointsExectime_ms;       // contains exec time of last keypoint detector in ms


using namespace std;

/* MAIN PROGRAM */
int main(int argc, const char *argv[])
{

    // keep all relevant data to assess the combinations
    vector<string> detectDescr;     // contains name of combination
    vector<double> descrExectime;   // average exec time of descriptor extration
    vector<double> detectExectime;  // average exec time of detector
    vector<double> resMatches;      // average number of matches
    
#ifdef ITERATE
    const vector<string> detectors = {"SHITOMASI", "HARRIS", "FAST", "BRISK", "ORB", "AKAZE", "SIFT"};
#else
    const vector<string> detectors = {"FAST"};
#endif
  
    for(auto detectorType : detectors)
    {
#ifdef ITERATE
        const vector<string> descriptors = {"BRISK", "BRIEF", "ORB", "FREAK", "AKAZE", "SIFT"};
#else
        const vector<string> descriptors = {"BRIEF"};
#endif        
        for(auto descriptorType: descriptors)
        {
            // out exclude list for stuff conbinations which crash - result of a trial and error approach
            if( (detectorType.compare("AKAZE")!=0     && descriptorType.compare("AKAZE")==0) ||      // AKAZE descriptors only work with AKAZE keypoints (see OpenCV doc)
                (detectorType.compare("SIFT")==0      && descriptorType.compare("ORB")==0) 
            )
            {
               std::cout << "### " << detectorType << " " << descriptorType << " SKIPPED" << std::endl;
                continue;
            }
            // else std::cout << "### " << detectorType << " " << descriptorType << " start" << std::endl;

            
            unsigned long totalKeyPoints = 0UL;
            size_t nMatches = 0;
            
            /* INIT VARIABLES AND DATA STRUCTURES */

            // data location
            string dataPath = "../";

            // camera
            string imgBasePath = dataPath + "images/";
            string imgPrefix = "KITTI/2011_09_26/image_00/data/000000"; // left camera, color
            string imgFileType = ".png";
            int imgStartIndex = 0; // first file index to load (assumes Lidar and camera names have identical naming convention)
            int imgEndIndex = 9;   // last file index to load
            int imgFillWidth = 4;  // no. of digits which make up the file index (e.g. img-0001.png)

            // misc
            int dataBufferSize = 2;       // no. of images which are held in memory (ring buffer) at the same time
            vector<DataFrame> dataBuffer; // list of data frames which are held in memory at the same time
            bool bVis = false;            // visualize results

            double accum_detect_time = 0.0;
            double accum_desc_time = 0.0;
            
            /* MAIN LOOP OVER ALL IMAGES */

            for (size_t imgIndex = 0; imgIndex <= imgEndIndex - imgStartIndex; imgIndex++)
            {
                /* LOAD IMAGE INTO BUFFER */

                // assemble filenames for current index
                ostringstream imgNumber;
                imgNumber << setfill('0') << setw(imgFillWidth) << imgStartIndex + imgIndex;
                string imgFullFilename = imgBasePath + imgPrefix + imgNumber.str() + imgFileType;

                // load image from file and convert to grayscale
                cv::Mat img, imgGray;
                img = cv::imread(imgFullFilename);
                cv::cvtColor(img, imgGray, cv::COLOR_BGR2GRAY);

                //// STUDENT ASSIGNMENT
                //// TASK MP.1 -> replace the following code with ring buffer of size dataBufferSize

                // push image into data frame buffer
                DataFrame frame;
                frame.cameraImg = imgGray;
                // remove first element
                if(dataBuffer.size()>=dataBufferSize)
                    dataBuffer.erase(dataBuffer.begin());
                dataBuffer.push_back(frame);

                //// EOF STUDENT ASSIGNMENT
                cout << "#1 : LOAD IMAGE INTO BUFFER done" << endl;

                /* DETECT IMAGE KEYPOINTS */

                // extract 2D keypoints from current image
                vector<cv::KeyPoint> keypoints; // create empty feature list for current image
                // string detectorType = "AKAZE";  // SHITOMASI, HARRIS, FAST, BRISK, ORB, AKAZE, SIFT

                //// STUDENT ASSIGNMENT
                //// TASK MP.2 -> add the following keypoint detectors in file matching2D.cpp and enable string-based selection based on detectorType
                //// -> HARRIS, FAST, BRISK, ORB, AKAZE, SIFT
                // string detectorType = "AKAZE";  // SHITOMASI, HARRIS, FAST, BRISK, ORB, AKAZE, SIFT

                if (detectorType.compare("SHITOMASI") == 0)
                {
                    detKeypointsShiTomasi(keypoints, imgGray, false);
                } else if(detectorType.compare("HARRIS") == 0) {
                    detKeypointsHarris(keypoints, imgGray, false);
                } else {
                    // void detKeypointsModern(std::vector<cv::KeyPoint> &keypoints, cv::Mat &img, std::string detectorType, bool bVis=false);
                    detKeypointsModern(keypoints, imgGray, detectorType);
                }
                accum_detect_time += gKeypointsExectime_ms;
                
                //// EOF STUDENT ASSIGNMENT

                //// STUDENT ASSIGNMENT
                //// TASK MP.3 -> only keep keypoints on the preceding vehicle

                // only keep keypoints on the preceding vehicle
                bool bFocusOnVehicle = true;
                cv::Rect vehicleRect(535, 180, 180, 150);
                if (bFocusOnVehicle)
                {
                    for(auto it=keypoints.begin(); it != keypoints.end(); )   // iterate through all keypoints
                    {
                        // check if it is outside of rectangle
                        if((it->pt.x < vehicleRect.x) || (it->pt.x >= (vehicleRect.x + vehicleRect.width)) ||
                            (it->pt.y < vehicleRect.y) || (it->pt.y >= (vehicleRect.y + vehicleRect.height)))
                        {
                            keypoints.erase(it);  // remove the keypoint
                        }
                        else
                        {
                            ++it;
                        }
                    }
                }
                totalKeyPoints += keypoints.size();
                
                //// EOF STUDENT ASSIGNMENT

                // optional : limit number of keypoints (helpful for debugging and learning)
                bool bLimitKpts = false;
                if (bLimitKpts)
                {
                    int maxKeypoints = 50;

                    if (detectorType.compare("SHITOMASI") == 0)
                    { // there is no response info, so keep the first 50 as they are sorted in descending quality order
                        keypoints.erase(keypoints.begin() + maxKeypoints, keypoints.end());
                    }
                    cv::KeyPointsFilter::retainBest(keypoints, maxKeypoints);
                    cout << " NOTE: Keypoints have been limited!" << endl;
                }

                // push keypoints and descriptor for current frame to end of data buffer
                (dataBuffer.end() - 1)->keypoints = keypoints;
                cout << "#2 : DETECT KEYPOINTS done" << endl;

                /* EXTRACT KEYPOINT DESCRIPTORS */

                //// STUDENT ASSIGNMENT
                //// TASK MP.4 -> add the following descriptors in file matching2D.cpp and enable string-based selection based on descriptorType
                //// -> BRIEF, ORB, FREAK, AKAZE, SIFT

                cv::Mat descriptors;
                // string descriptorType = "BRIEF"; // BRIEF, ORB, FREAK, AKAZE, SIFT
                descKeypoints((dataBuffer.end() - 1)->keypoints, (dataBuffer.end() - 1)->cameraImg, descriptors, descriptorType);
                //// EOF STUDENT ASSIGNMENT

                // push descriptors for current frame to end of data buffer
                (dataBuffer.end() - 1)->descriptors = descriptors;

                accum_desc_time += gDescKeyPointsExectime_ms;
                cout << "#3 : EXTRACT DESCRIPTORS done" << endl;

                if (dataBuffer.size() > 1) // wait until at least two images have been processed
                {

                    /* MATCH KEYPOINT DESCRIPTORS */
                    vector<cv::DMatch> matches;
                    string matcherType = "MAT_BF";        // MAT_BF, MAT_FLANN
                    // string descriptorType;                // DES_BINARY, DES_HOG
                    string selectorType = "SEL_NN";       // SEL_NN, SEL_KNN
        
                    //// STUDENT ASSIGNMENT
                    //// TASK MP.5 -> add FLANN matching in file matching2D.cpp
                    //// TASK MP.6 -> add KNN match selection and perform descriptor distance ratio filtering with t=0.8 in file matching2D.cpp

                    matchDescriptors((dataBuffer.end() - 2)->keypoints, (dataBuffer.end() - 1)->keypoints,
                                    (dataBuffer.end() - 2)->descriptors, (dataBuffer.end() - 1)->descriptors,
                                    matches, descriptorType, matcherType, selectorType);

                    //// EOF STUDENT ASSIGNMENT
                    nMatches += matches.size();

                    // store matches in current data frame
                    (dataBuffer.end() - 1)->kptMatches = matches;
                    
                    cout << "#4 : MATCH KEYPOINT DESCRIPTORS done" << endl;

                    // visualize matches between current and previous image
#ifndef ITERATE
                    bVis = true;
#endif
                    if (bVis)
                    {
                        cv::Mat matchImg = ((dataBuffer.end() - 1)->cameraImg).clone();
                        cv::drawMatches((dataBuffer.end() - 2)->cameraImg, (dataBuffer.end() - 2)->keypoints,
                                        (dataBuffer.end() - 1)->cameraImg, (dataBuffer.end() - 1)->keypoints,
                                        matches, matchImg,
                                        cv::Scalar::all(-1), cv::Scalar::all(-1),
                                        vector<char>(), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
                        string windowName = "Matching keypoints between two camera images";
                        cv::namedWindow(windowName, 7);
                        cv::imshow(windowName, matchImg);
                        cout << "Press key to continue to next image" << endl;
                        cv::waitKey(0); // wait for key to be pressed
                    }
                    bVis = false;
                }
            } // eof loop over all images
            
            std::cout << "### " << detectorType << " " << descriptorType << ": " << nMatches << " matches " << std::endl;
            detectDescr.push_back(detectorType + "_" + descriptorType);
            descrExectime.push_back( accum_desc_time / ((double) (imgEndIndex-imgStartIndex-1)));
            detectExectime.push_back( accum_detect_time / ((double) (imgEndIndex-imgStartIndex-1)));
            resMatches.push_back(nMatches/ ((double) (imgEndIndex-imgStartIndex-1)));            
        } // descriptors
    } // detectors
    
#ifdef ITERATE
    // output the measured results
    fstream outfile;
    outfile.open("../data/result.txt", ios::out | ios::trunc);
    for(int i=0; i<descrExectime.size(); ++i)
        outfile << detectDescr[i] << " " << resMatches[i] << " " <<  1000.0/(detectExectime[i] + descrExectime[i]) << endl;
        outfile.close();
#endif
        
    return 0;
}
