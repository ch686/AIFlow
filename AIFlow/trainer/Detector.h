#pragma once
#include"net_utils/netutil.h"
#include"net/yolo4_tiny.h"
#include<opencv2/opencv.hpp>
#include "exporter.h"
#include <QObject>
#include"DetDataset.h"
#include <opencv2/freetype.hpp>


class Detector: public QObject
{
    // 必须宏，启用信号槽机制
    Q_OBJECT
public:
    int _num_epochs;
    int _batch_size;
    float _learning_rate;
    float _best_loss;
    bool _is_finish;
    bool _is_pre;
    explicit Detector(QObject *parent = nullptr);
    void InitializeTrain(int gpu_id, int width, int height,
                    std::shared_ptr<Exporter> exporter,
                    const std::string &data_dir);
    void InitializePredict(int gpu_id, int width, int height,const QStringList &class_list);

	void LoadWeight(std::string weight_path);
	void loadPretrained(std::string pretrained_pth);
    void Predict(cv::Mat &_image, bool show = false, float conf_thresh = 0.3, float nms_thresh = 0.3);
signals:
    // 训练进度信号：当前epoch，总epoch，loss值
    void sigTrainProgress(bool is_train,int cur_epoch, int total_epoch, int batch_cnt,int total_batch);
    void sigTrainLoss(bool is_train,int cur_epoch, int total_epoch,float loss);
    void sigTrainFinished();
public slots:
    void Train();
    void StopTrain();

private:
    std::shared_ptr<Exporter> _exporter;
    std::string _data_dir;
    int _width = 416;
    int _height = 416;
    std::vector<std::string> _name_list;
    torch::Device _device = torch::Device(torch::kCPU);
    YoloBody_tiny _detector{ nullptr };
    std::atomic<bool> _stop_flag{false};
    static cv::Ptr<cv::freetype::FreeType2> _ftFont2;
    void show_bbox(cv::Mat image, torch::Tensor bboxes,
                   std::vector<std::string> name_list,bool bShow);

};

