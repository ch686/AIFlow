#pragma once
#include<opencv2/opencv.hpp>

struct  BBox
{
	int class_id = 0;
	float cx = 0;
	float cy = 0;
	float w = 0;
	float h = 0;
};

struct TrainDataset {
    std::string image_path;
    std::vector<BBox> bboxes;
};


struct Data {
	Data(cv::Mat img, std::vector<BBox> boxes) :image(img), bboxes(boxes) {};
	cv::Mat image;
	std::vector<BBox> bboxes;
};

class Augmentations
{
public:
	static Data Resize(Data mData, int width, int height, float probability);
};

