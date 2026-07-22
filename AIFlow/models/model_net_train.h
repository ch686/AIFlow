#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>
#include <QListWidget>
#include <QPushButton>

#include "node_dataset.h"
#include "node_net.h"
#include "exporter.h"
#include <QComboBox>
#include <QCheckBox>

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class ModelNetTrain : public NodeDelegateModel
{
    Q_OBJECT

public:
    ModelNetTrain();

    ~ModelNetTrain() = default;

public:
    QString caption() const override { return QString(QStringLiteral("模型训练")); }

    QString name() const override { return QString(QStringLiteral("模型训练")); }

public:
    virtual QString modelName() const { return QString(QStringLiteral("机器学习模块")); }

    unsigned int nPorts(PortType const portType) const override;

    NodeDataType dataType(PortType const portType, PortIndex const portIndex) const override;

    std::shared_ptr<NodeData> outData(PortIndex const port) override;

    void setInData(std::shared_ptr<NodeData>, PortIndex const portIndex) override ;

    QWidget *embeddedWidget() override { return _widget; }

    bool resizable() const override { return true; }

public slots:
    void on_button1_pressed();
protected:
 //   bool eventFilter(QObject *object, QEvent *event) override;

private:
    QStringList LoadFolder(const QString& path,const QStringList& tags);
    QWidget* _widget;
    QLabel* _label;
    QLabel* _label2;
    QLabel* _label3;
    QListWidget *_list;
    QPushButton *_button1;
    QPushButton *_button2;
    QCheckBox *_checkbox;
    void InitUI();

    QString _data_path;
    QStringList _class_names;
    //std::shared_ptr<NodeData> _nodeData;
    std::shared_ptr<Exporter> _exporter;
    std::shared_ptr<NodeNet> _nodeNet;
    QComboBox *_combo1;
    QComboBox *_combo2;
    QComboBox *_combo3;
    //bool _is_reload;
    bool _dataReady;  // 标记数据是否已准备好但未发送
};
