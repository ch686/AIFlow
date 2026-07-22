#pragma once

#include <QtGui/QPixmap>

#include <QtNodes/NodeData>
#include <opencv2/opencv.hpp>

using QtNodes::NodeData;
using QtNodes::NodeDataType;

/// The class can potentially incapsulate any user data which
/// need to be transferred within the Node Editor graph
class NodeAddImage : public NodeData
{
public:
    NodeAddImage(bool is_out=true) {_is_out=is_out;}

    NodeAddImage(cv::Mat const &image)
    {
        _image=image;
    }

    NodeDataType type() const override
    {
        //       id      name
        if(_is_out)
            return {"导出图像", "导出图像"};
        else
            return {"导出图像", "导入图像"};
    }

     cv::Mat &image() { return _image; }

private:
     bool _is_out=false;
    cv::Mat _image;
};
