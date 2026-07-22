#pragma once

#include <QtGui/QPixmap>

#include <QtNodes/NodeData>
#include <opencv2/opencv.hpp>

using QtNodes::NodeData;
using QtNodes::NodeDataType;

/// The class can potentially incapsulate any user data which
/// need to be transferred within the Node Editor graph
class NodeImage : public NodeData
{
public:
    NodeImage() {}

    NodeImage(cv::Mat const &image)
    {
        _image=image;
    }

    NodeDataType type() const override
    {
        //       id      name
        return {"图像", "图像"};
    }

     cv::Mat &image() { return _image; }

private:
    cv::Mat _image;
};
