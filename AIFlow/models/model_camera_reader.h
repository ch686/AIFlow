#pragma once

#include <iostream>
#include <atomic>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>

#include "node_image.h"
#include "node_add_image.h"

#include <QPushButton>
#include <QCheckBox>
#include <QThread>
#include <QComboBox>
#include <QMutex>
#include <QWaitCondition>

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

/// 在独立子线程中执行摄像头采集的工作对象。
/// 注意：它只负责 read 帧并通过信号发回主线程，不接触任何 UI。
class CameraWorker : public QObject
{
    Q_OBJECT

public:
    CameraWorker(QObject *parent = nullptr);
    ~CameraWorker();

public slots:
    void start(int cameraIndex);
    void stop();
    void wakeOneFrame();
    void wakeContinueFrame(bool is_stop);

signals:
    void frameReady(cv::Mat frame);
    void cameraInit(double fps,int w,int h);
    void errorOccurred(QString err);

private:
    void doGrab();
    cv::VideoCapture _vcp;
    double _fps = 30.0;
    std::atomic<bool> _isStop{false};
    int _cameraIndex = -1;

    std::atomic<int> _trigger = 0;       // 触发标记
    QMutex _mutex;
    QWaitCondition _cond;
};

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class ModelCameraReader : public NodeDelegateModel
{
    Q_OBJECT

public:
    ModelCameraReader();

    ~ModelCameraReader();

public:
    QString caption() const override { return QString(QStringLiteral("摄像头")); }

    QString name() const override { return QString(QStringLiteral("摄像头")); }

public:
    virtual QString modelName() const { return QString(QStringLiteral("图像模块")); }

    unsigned int nPorts(PortType const portType) const override;

    NodeDataType dataType(PortType const portType, PortIndex const portIndex) const override;

    std::shared_ptr<NodeData> outData(PortIndex const port) override;

    void setInData(std::shared_ptr<NodeData>, PortIndex const portIndex) override {}

    QWidget *embeddedWidget() override { return _widget; }

    bool resizable() const override { return true; }

public slots:
    void onCameraInit(double fps, int w, int h);
    void on_combo_indexChanged(int index);
    void on_button1_pressed();
    void on_button2_pressed();
    void on_button3_pressed();

    /// 子线程采集到新帧后，在主线程接收
    void onFrameReady(cv::Mat frame);
signals:
    void wakeOneFrame();
    void wakeContinueFrame(bool is_stop);
protected:
 //   bool eventFilter(QObject *object, QEvent *event) override;

private:
    QPixmap _pix1;
    QPixmap _pix2;
    QPixmap _pix3;
    QPixmap _pix4;
    QPixmap _pix5;
    QThread* _grabThread{nullptr};
    CameraWorker* _worker{nullptr};

    static QStringList m_CamDevices;
    static const QStringList &EnumCamera();
    void InitUI();
    QWidget* _widget;
    QComboBox *_combo{nullptr};
    //cv::VideoCapture _vcp;
    std::atomic<bool> _isRunning{false};
    std::atomic<bool> _is_continue_trigger{false};
    int _cameraIndex = -1;
    double _fps = 0;
    cv::Mat _image;
    QLabel *_label_state;
    QLabel *_label_info;
    QPushButton *_button1;
    QPushButton *_button2;
    QPushButton *_button3;
    QCheckBox *_checkbox;
    bool _check_state;
    bool _is_to_list;
};
