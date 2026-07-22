#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>

#include "node_net.h"

#include <opencv2/freetype.hpp>
#ifdef USE_CVDNN
#include "dnn_yolo.h"
#else
#include "yolo_detect.h"
#endif

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class ModelOnnxReader : public NodeDelegateModel
{
    Q_OBJECT

public:
    ModelOnnxReader();
    ModelOnnxReader(const std::string &name,const std::string &onnx_path,const std::vector<std::string> &class_names);

    ~ModelOnnxReader();

public:
    QString caption() const override { return _name; }

    QString name() const override { return _name; }

public:
    virtual QString modelName() const { return QString("预训练模型"); }

    unsigned int nPorts(PortType const portType) const override;

    NodeDataType dataType(PortType const portType, PortIndex const portIndex) const override;

    std::shared_ptr<NodeData> outData(PortIndex const port) override;

    void setInData(std::shared_ptr<NodeData>, PortIndex const portIndex) override;

    QWidget *embeddedWidget() override { return _label; }

    bool resizable() const override { return true; }

public slots:
 //   void propagateData();  // 发送缓存的数据

protected:
  //  bool eventFilter(QObject *object, QEvent *event) override;

private:
    QString _name;
    cv::Scalar color_map(int n, int total);
    std::vector<std::string> _gestureNames;
#ifdef USE_CVDNN
    DnnYolo *_yolo;
#else
    yolo_detect *_yolo;
#endif
    QLabel *_label;
    bool _dataReady;  // 标记数据是否已准备好但未发送
    std::shared_ptr<NodeData> _nodeData;
    std::string _onnx_path;
    std::vector<std::string> _class_names;
};
