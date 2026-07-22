#ifndef DNN_YOLO_H
#define DNN_YOLO_H
#include <opencv2/opencv.hpp>

struct PosePoint
{
    float x;
    float y;
    float conf;
};

struct Detection {
    short class_id;
    std::string class_name;
    float confidence;
    cv::Rect box;
    std::vector<PosePoint> points;
};

class DnnYolo
{
public:
    DnnYolo(const std::string &modelpath,const std::vector<std::string> &class_names, float confThreshold=0.45f, float nmsThreshold=0.5f);
    void detect(cv::Mat& frame,std::vector<Detection> &det_results);
private:
    std::vector<std::string> _class_names;
    int _class_num;
    cv::Mat resize_image(cv::Mat srcimg, int *newh, int *neww, int *padh, int *padw);
    const bool keep_ratio = true;
    const int inpWidth = 640;
    const int inpHeight = 640;
    float _confThreshold;
    float _nmsThreshold;
    const int reg_max = 16;
    cv::dnn::Net net;
    cv::Point pad_offset_;
    float scale_factor_;
    float generate_scale(const cv::Size& image_size, const std::vector<int>& target_size);
    float letterbox(const cv::Mat &input_image, cv::Mat &output_image, const std::vector<int> &target_size);
    cv::Rect GetBoundingBox(const cv::Rect &src) const;
    std::vector<PosePoint> GetBoundingPoints(const std::vector<PosePoint> &points) const;

};



#endif // DNN_YOLO_H
