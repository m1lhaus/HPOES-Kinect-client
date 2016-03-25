#include "segmentation.h"

Segmentation::Segmentation()
{

}

void Segmentation::segmentImage(const cv::Mat &input, cv::Mat &output)
{
    output = input.clone();
}
