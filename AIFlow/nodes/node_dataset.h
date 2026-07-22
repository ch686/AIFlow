#pragma once

#include <QtGui/QPixmap>

#include <QtNodes/NodeData>
#include <opencv2/opencv.hpp>

using QtNodes::NodeData;
using QtNodes::NodeDataType;

/// The class can potentially incapsulate any user data which
/// need to be transferred within the Node Editor graph
class NodeDataset : public NodeData
{
public:
    NodeDataset() {}

    NodeDataset(const QString &data_dir)
    {
        _data_dir=data_dir;
    }

    NodeDataType type() const override
    {
        //       id      name
        return {"训练数据集", "训练数据集"};
    }

    QString data_dir() { return _data_dir; }

private:
    QString _data_dir;
};
