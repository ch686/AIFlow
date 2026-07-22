#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>
#include <QListWidget>
#include <QPushButton>

#include "node_net.h"
#include "node_image.h"
#include"Detector.h"
#include "yolo_detect.h"

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class ModelNetPredict : public NodeDelegateModel
{
    Q_OBJECT

public:
    ModelNetPredict();

    ~ModelNetPredict();

public:
    QString caption() const override { return QString("模型预测"); }

    QString name() const override { return QString("模型预测"); }

public:
    virtual QString modelName() const { return QString("机器学习模块"); }

    unsigned int nPorts(PortType const portType) const override;

    NodeDataType dataType(PortType const portType, PortIndex const portIndex) const override;

    std::shared_ptr<NodeData> outData(PortIndex const port) override;

    void setInData(std::shared_ptr<NodeData>, PortIndex const portIndex) override;

    QWidget *embeddedWidget() override { return _label; }

    bool resizable() const override { return true; }

public slots:
    void on_button1_pressed();
protected:
 //   bool eventFilter(QObject *object, QEvent *event) override;

private:
    QStringList LoadFolder(const QString& path,const QStringList& tags);
    QWidget* _widget;
    QLabel* _label;
    QListWidget *_list;
    QPushButton *_button1;
    QPushButton *_button2;
    void InitUI();
    std::shared_ptr<NodeData> _nodeData1;
    std::shared_ptr<NodeData> _nodeData2;
    std::shared_ptr<NodeData> _nodeData3;
    Detector *_detector;
    yolo_detect *_yolo;
    std::vector<std::string> _class_names;
    cv::Scalar color_map(int n, int total);
    cv::Rect scale_boxes(const cv::Mat& image, const cv::Rect2f& box);
    static cv::Ptr<cv::freetype::FreeType2> _ftFont;

    cv::Mat _image;
    bool _dataReady;  // 标记数据是否已准备好但未发送
};
