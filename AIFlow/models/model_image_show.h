#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>
#include "node_image.h"
#include <QTimer>

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class ModelImageShow : public NodeDelegateModel
{
    Q_OBJECT

public:
    ModelImageShow();

    ~ModelImageShow() = default;

public:
    QString caption() const override { return QString(QStringLiteral("显示图像")); }

    QString name() const override { return QString(QStringLiteral("显示图像")); }

public:
    virtual QString modelName() const { return QString(QStringLiteral("图像模块")); }

    unsigned int nPorts(PortType const portType) const override;

    NodeDataType dataType(PortType const portType, PortIndex const portIndex) const override;

    std::shared_ptr<NodeData> outData(PortIndex const port) override;

    void setInData(std::shared_ptr<NodeData> nodeData, PortIndex const port) override;

    QWidget *embeddedWidget() override { return _widget; }

    bool resizable() const override { return true; }
private slots:
    void onTimerRefresh();
protected:
    bool eventFilter(QObject *object, QEvent *event) override;
private:
    QPixmap _move_pix;
    QWidget* _widget;
    QLabel *_label;
    cv::Mat _frameBuf;       // 图像缓冲区（线程安全）
    QTimer _uiRefreshTimer; // 主线程刷新定时器
    QPixmap matToPixmap(const cv::Mat &mat);
    void update_image(const cv::Mat&image);

    std::mutex _mtx;
    std::shared_ptr<NodeData> _nodeData;
};
