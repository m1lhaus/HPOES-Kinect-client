#ifndef SEGMENTATION_H
#define SEGMENTATION_H

#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

bool compareWhichIsBigger(std::vector<cv::Point> contour1, std::vector<cv::Point> contour2);
bool compareWhichIsSmaller(std::vector<cv::Point> contour1, std::vector<cv::Point> contour2);

class Segmentation
{
public:
    Segmentation(int imgWidth, int imgHeight);

    bool segmentImage(const cv::Mat &colorInput, const cv::Mat &depthInput, cv::Mat &output);

private:
    int dstHeight;
    int dstWidth;
    Scalar lowerb;
    Scalar upperb;
    Mat filterElement;
};

#endif // SEGMENTATION_H
