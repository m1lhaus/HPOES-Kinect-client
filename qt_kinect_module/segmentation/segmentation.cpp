#include "segmentation.h"

Segmentation::Segmentation(int imgWidth, int imgHeight)
{
    this->lowerb = Scalar(190./360.*180., 0.4*255, 0.3*255);
    this->upperb = Scalar(225./360.*180., 255.0, 255.0);
    this->filterElement = getStructuringElement( MORPH_CROSS, Size(3, 3));
    this->dstWidth = imgWidth;
    this->dstHeight = imgHeight;

    cout << "OPENCV VERSION: "<< CV_VERSION << endl;
}

bool Segmentation::segmentImage(const cv::Mat &colorInput, const cv::Mat &depthInput, cv::Mat &output) {
    Mat depthMat = depthInput.clone();          // CV_32FC1
    Mat colorMat = colorInput.clone();          // CV_8UC4

    Mat hsvImg;
    cvtColor(colorMat, hsvImg, COLOR_BGR2HSV);  // CV_8UC3

    // segment colorMat and filter noise
    Mat colorMask;
    inRange(hsvImg, lowerb, upperb, colorMask);
    morphologyEx(colorMask, colorMask, MORPH_OPEN, filterElement);

    // find contours and keep only the biggest one
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;
    findContours(colorMask, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
    if (contours.size() == 0) {
        cout << "ERROR: No contour found!" << endl;
        return false;
    } else if (contours.size() > 1) {
        std::sort(contours.begin(), contours.end(), compareWhichIsBigger);
    }
    if (contourArea(contours[0]) < 100) {
        cout << "ERROR: Contour area is less than 100px!" << endl;
        return false;
    }

    // get center of mass and get mean depth value of hand
    Moments imgMoments = moments(contours[0]);
    Point2f massCenter = Point2f((float) (imgMoments.m10 / imgMoments.m00), (float) (imgMoments.m01 / imgMoments.m00));
    Mat cntMask = Mat::zeros(depthMat.size(), CV_8UC1);
    drawContours(cntMask, contours, 0, Scalar(255), CV_FILLED);     // fill mask
    double handDist = mean(depthMat, cntMask)[0];

    // extract only hand from depthMat
    Mat handOnly;
    depthMat.copyTo(handOnly, cntMask);

    cv::imshow("handOnly", handOnly);

    // scale image to keep hand size independent on hand distance
    double resizeRatio = handDist / 1000.0;
    resizeRatio = (resizeRatio > 0.5) ? resizeRatio : 0.5;
    resizeRatio = (resizeRatio <= 5) ? resizeRatio : 5;
    resize(handOnly, handOnly, Size(0, 0), resizeRatio, resizeRatio, INTER_NEAREST);

    // add border to perform safe data slice Mat(Roi())
    Mat safeHandOnly;
    int border = (int) ceil(this->dstWidth / 2.0);
    copyMakeBorder(handOnly, safeHandOnly, border, border, border, border, BORDER_CONSTANT, Scalar(0));

    Rect handROI((int) round(massCenter.x*resizeRatio),(int) round(massCenter.y*resizeRatio), dstWidth, dstHeight);
    safeHandOnly(handROI).copyTo(output);

    Mat meanMask;
    threshold(output, meanMask, 0, handDist, THRESH_BINARY);

    // norm window
    output -= meanMask;
    output /= 150.f;

    return true;
}

bool compareWhichIsBigger(std::vector<cv::Point> contour1, std::vector<cv::Point> contour2) {
    double i = fabs( contourArea(cv::Mat(contour1)) );
    double j = fabs( contourArea(cv::Mat(contour2)) );
    return ( i > j );
}

bool compareWhichIsSmaller(std::vector<cv::Point> contour1, std::vector<cv::Point> contour2) {
    double i = fabs( contourArea(cv::Mat(contour1)) );
    double j = fabs( contourArea(cv::Mat(contour2)) );
    return ( i < j );
}


