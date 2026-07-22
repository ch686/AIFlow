#ifndef YOLOV8_H
#define YOLOV8_H


// 再引入所有头文件
#include"net_utils/netutil.h"
#include <opencv2/opencv.hpp>


//#define CV_CUDA

#ifdef CV_CUDA
#include <opencv2/cudacodec.hpp>
#include <opencv2/cudaimgproc.hpp>
#include <opencv2/cudawarping.hpp>
#include <opencv2/cudafeatures2d.hpp>
#include <opencv2/cudaarithm.hpp>
#endif

//typedef struct tagYoloBox
//{
//    int classid;
//    std::string  classname;
//    float conf;
//    cv::Rect2f box;
//}YOLO_BOX;

typedef struct tagYoloPosePoint
{
    float x;
    float y;
    float conf;
}POSE_PT;


typedef struct tagYoloResult
{
    int classid;
    std::string  classname;
    float conf;
    cv::Rect2f box;
    std::vector<POSE_PT> points;
}YOLO_RESULT;


class yolo_detect
{
private:
    std::vector<int> m_kpt_shape;
    static int have_cuda;
    int m_ImgW;
    int m_ImgH;
    std::vector<std::vector<YOLO_RESULT>> m_DetResults;
    torch::DeviceType m_DeviceType;
    torch::jit::script::Module yolo_model;
    torch::Tensor xyxy2xywh(const torch::Tensor& x);
    torch::Tensor xywh2xyxy(const torch::Tensor& x);
    torch::Tensor nms(const torch::Tensor& bboxes, const torch::Tensor& scores, float iou_threshold);
    std::vector<torch::Tensor> non_max_supperession(torch::Tensor& prediction, float conf_thres = 0.25, float iou_thres = 0.45, int max_det = 300);
    torch::Tensor clip_boxes(torch::Tensor& boxes, const std::vector<int>& shape);
    torch::Tensor scale_boxes(const std::vector<int>& img1_shape, torch::Tensor& boxes, const std::vector<int>& img0_shape);

#ifdef CV_CUDA
    std::vector<std::shared_ptr<cv::cuda::HostMem>> m_HostmemsInput;
    float letterbox_gpu(const cv::cuda::GpuMat& input_image, cv::cuda::GpuMat& output_image, const std::vector<int>& target_size);

#endif

    float generate_scale(const cv::Size& image_size, const std::vector<int>& target_size);
    float letterbox(const cv::Mat &input_image, cv::Mat &output_image, const std::vector<int> &target_size);
    std::map<int,std::string> m_ClassMap;
public:
    static int is_cuda_available();
    int load_model(const std::string &model_path);
#ifdef CV_CUDA
    int detect_gpu(const std::vector<cv::cuda::GpuMat> &images);
#endif
    int detect(const std::vector<cv::Mat> &images);
    void test(int bs);

    std::vector<YOLO_RESULT> get_result(int index);
    std::string get_class_name(int id);
    std::vector<std::string> get_class_names();
    yolo_detect();
};

#endif // YOLOV8_H
