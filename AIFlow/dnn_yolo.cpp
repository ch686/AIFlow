#include "dnn_yolo.h"
#include <QDebug>

DnnYolo::DnnYolo(const std::string &modelpath,const std::vector<std::string> &class_names, float confThreshold, float nmsThreshold)
{
    _class_names=class_names;
    _class_num=class_names.size();
    _confThreshold = confThreshold;
    _nmsThreshold = nmsThreshold;
    this->net = cv::dnn::readNet(modelpath);
}

float DnnYolo::generate_scale(const cv::Size& image_size, const std::vector<int>& target_size)
{
    int origin_w = image_size.width;
    int origin_h = image_size.height;

    int target_h = target_size[0];
    int target_w = target_size[1];

    float ratio_h = static_cast<float>(target_h) / static_cast<float>(origin_h);
    float ratio_w = static_cast<float>(target_w) / static_cast<float>(origin_w);
    float resize_scale = std::min(ratio_h, ratio_w);
    return resize_scale;
}

float DnnYolo::letterbox(const cv::Mat &input_image, cv::Mat &output_image, const std::vector<int> &target_size)
{
    //qDebug()<<"input_image.size():"<<input_image.size();
    std::cout<<"target_size():"<<target_size[0]<<"x"<<target_size[1]<<std::endl;
    if (input_image.cols == target_size[1] && input_image.rows == target_size[0]) {
        if (input_image.data == output_image.data) {
            return 1.;
        } else {
            output_image = input_image.clone();
            return 1.;
        }
    }

    float resize_scale = generate_scale(input_image.size(), target_size);
    int new_shape_w = std::round(input_image.cols * resize_scale);
    int new_shape_h = std::round(input_image.rows * resize_scale);
    float padw = (target_size[1] - new_shape_w) / 2.;
    float padh = (target_size[0] - new_shape_h) / 2.;

    int top = std::round(padh - 0.1);
    int bottom = std::round(padh + 0.1);
    int left = std::round(padw - 0.1);
    int right = std::round(padw + 0.1);
    pad_offset_.x=left;
    pad_offset_.y=top;
    std::cout<<"new_shape_w:"<<new_shape_w<<" new_shape_h:"<<new_shape_h<<std::endl;
    cv::resize(input_image, output_image,
               cv::Size(new_shape_w, new_shape_h),
               0, 0, cv::INTER_AREA);
    //cv::cvtColor(output_image,output_image,cv::COLOR_BGR2RGB);
    cv::copyMakeBorder(output_image, output_image, top, bottom, left, right,
                       cv::BORDER_CONSTANT, cv::Scalar(114,114,114));
    return resize_scale;
}



cv::Rect DnnYolo::GetBoundingBox(const cv::Rect &src) const
{
    cv::Rect rcBox = src;
    rcBox.x=src.x-src.width/2;
    rcBox.y=src.y-src.height/2;

    rcBox.x = cvRound((float)(rcBox.x-pad_offset_.x)/scale_factor_);
    rcBox.y = cvRound((float)(rcBox.y-pad_offset_.y)/ scale_factor_);
    rcBox.width = cvRound((float)rcBox.width/scale_factor_);
    rcBox.height =  cvRound((float)rcBox.height /scale_factor_);
    return rcBox;
}

std::vector<PosePoint> DnnYolo::GetBoundingPoints(const std::vector<PosePoint> &points) const
{
    std::vector<PosePoint> bd_points;
    for(auto &pt:points)
    {
        PosePoint bd_pt;
        bd_pt.x=cvRound((float)(pt.x-pad_offset_.x)/scale_factor_);
        bd_pt.y=cvRound((float)(pt.y-pad_offset_.y)/ scale_factor_);

        bd_points.push_back(bd_pt);
    }
    return bd_points;
}


void DnnYolo::detect(cv::Mat& srcimg,std::vector<Detection> &det_results)
{
    cv::Mat dst ;
    scale_factor_= letterbox(srcimg, dst, {640,640});
    cv::Mat blob;
    cv::dnn::blobFromImage(dst, blob, 1 / 255.0, cv::Size(this->inpWidth, this->inpHeight), cv::Scalar(0, 0, 0), true, false);
    this->net.setInput(blob);
    cv::Mat outs;
    ///net.enableWinograd(false);  ////如果是opencv4.7，那就需要加上这一行
    this->net.forward(outs, this->net.getUnconnectedOutLayersNames());

    std::vector<int> class_list;
    std::vector<float> confidence_list;
    std::vector<cv::Rect> box_list;
    std::vector<std::vector<PosePoint>> kpt_list;

    qDebug()<<"outs";
    qDebug()<<outs.size.size();
    for(int i=0;i<outs.size.size();i++)
        qDebug()<<" "<<outs.size[i];
    cv::Mat detection_outputs=outs.reshape(1, outs.size[1]);


    //std::cout<<detection_outputs.size()<<std::endl;
    // Iterate over detections and collect class IDs, confidence scores, and bounding boxes
    for (int i = 0; i < detection_outputs.cols; ++i)
    {
        const cv::Mat classes_scores = detection_outputs.col(i).rowRange(4, 4+_class_num);
        const cv::Mat kpt_scores = detection_outputs.col(i).rowRange(4+_class_num, detection_outputs.rows);
        cv::Point class_id;
        double score;
        cv::minMaxLoc(classes_scores, nullptr, &score, nullptr, &class_id); // Find the class with the highest score

        // Check if the detection meets the confidence threshold
        if (score > _confThreshold)
        {
            class_list.push_back(class_id.y);
            confidence_list.push_back(score);

            const float x = detection_outputs.at<float>(0, i);
            const float y = detection_outputs.at<float>(1, i);
            const float w = detection_outputs.at<float>(2, i);
            const float h = detection_outputs.at<float>(3, i);

            cv::Rect box;
            box.x = static_cast<int>(x);
            box.y = static_cast<int>(y);
            box.width = static_cast<int>(w);
            box.height = static_cast<int>(h);
            box_list.push_back(box);
            int kp_size=kpt_scores.rows/3;
            std::vector<PosePoint> points;
            for(int k=0;k<kp_size;k++)
            {
                PosePoint pt;
                pt.x= kpt_scores.at<float>(k * 3 + 0,0);
                pt.y= kpt_scores.at<float>(k * 3 + 1,0);
                pt.conf= kpt_scores.at<float>(k * 3 + 2,0);
                points.push_back(pt);
            }
            kpt_list.push_back(points);

        }
    }

    // Apply Non-Maximum Suppression (NMS) to filter overlapping bounding boxes
    std::vector<int> NMS_result;
    cv::dnn::NMSBoxes(box_list, confidence_list, _confThreshold, _nmsThreshold, NMS_result);

    det_results.clear();
    // Collect final detections after NMS
    for (int i = 0; i < NMS_result.size(); ++i)
    {
        Detection result;
        const unsigned short id = NMS_result[i];

        result.class_id = class_list[id];
        result.class_name=std::to_string(result.class_id);
        result.confidence = confidence_list[id];
        result.box = GetBoundingBox(box_list[id]);
        result.points=GetBoundingPoints(kpt_list[id]);
        det_results.push_back(result);
        //DrawDetectedObject(frame, result);
    }

}
