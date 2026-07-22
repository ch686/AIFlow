#pragma once

#include <QtGui/QPixmap>

#include <QtNodes/NodeData>
#include <opencv2/opencv.hpp>

using QtNodes::NodeData;
using QtNodes::NodeDataType;

/// The class can potentially incapsulate any user data which
/// need to be transferred within the Node Editor graph
class NodeNet : public NodeData
{
public:
    NodeNet() {}

    NodeNet(const QString &model_path,const QStringList &class_names,bool is_onnx=false)
    {
        _model_path=model_path;
        _class_names=class_names;
        _is_onnx=is_onnx;
        _is_reload=true;
    }

    NodeDataType type() const override
    {
        //       id      name
        return {"模型", "模型"};
    }

    const QString &model_path() { return _model_path; }
    const QStringList &class_names() { return _class_names; }
    bool is_onnx() { return _is_onnx; }
    bool is_reload() { return _is_reload; }
    void set_reload(bool is_reload){_is_reload=is_reload;}
private:
    QString _model_path;
    QStringList _class_names;
    bool _is_onnx;
    bool _is_reload;
};
