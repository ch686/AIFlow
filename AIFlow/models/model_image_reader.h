#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>
#include <QListWidget>
#include <QPushButton>

#include "node_image.h"
#include "node_add_image.h"
#include "node_dataset.h"
#include <QWheelEvent>

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;


/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class ModelImageReader : public NodeDelegateModel
{
    Q_OBJECT

public:
    ModelImageReader();

    ~ModelImageReader() = default;

public:
    QString caption() const override { return QString(QStringLiteral("图像列表")); }

    QString name() const override { return QString(QStringLiteral("图像列表")); }

public:
    virtual QString modelName() const { return QString(QStringLiteral("图像模块")); }

    unsigned int nPorts(PortType const portType) const override;

    NodeDataType dataType(PortType const portType, PortIndex const portIndex) const override;

    std::shared_ptr<NodeData> outData(PortIndex const port) override;

    void setInData(std::shared_ptr<NodeData>, PortIndex const portIndex) override;

    QWidget *embeddedWidget() override { return _widget; }

    bool resizable() const override { return true; }

public slots:
    void on_button1_pressed();
    void on_button2_pressed();
    void on_button3_pressed();
 //   void propagateData();  // 发送缓存的数据
    void on_list_itemClicked(QListWidgetItem *item);
protected:
   // bool eventFilter(QObject *object, QEvent *event) override;

private:
    QStringList LoadFolder(const QString& path,const QStringList& tags);
    QWidget* _widget;
    QLabel* _label;
    QListWidget *_list;
    QPushButton *_button1;
    QPushButton *_button2;
    QPushButton *_button3;
    void InitUI();
    QString _folderPath;
    cv::Mat _image;
    bool _dataReady;  // 标记数据是否已准备好但未发送
};
