#ifndef SEGMENTATION_H
#define SEGMENTATION_H

#include <opencv2/opencv.hpp>

class Segmentation
{
public:
    Segmentation();

    void segmentImage(const cv::Mat &input, cv::Mat &output);
};

#endif // SEGMENTATION_H
