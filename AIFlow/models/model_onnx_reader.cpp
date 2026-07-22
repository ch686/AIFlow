#include "model_onnx_reader.h"


#include <QtCore/QDir>
#include <QtCore/QEvent>

#include <QtWidgets/QFileDialog>
#include "utils.h"


ModelOnnxReader::ModelOnnxReader()
{
    _yolo=nullptr;
}

ModelOnnxReader::ModelOnnxReader(const std::string &name,
                                 const std::string &onnx_path,
                                 const std::vector<std::string> &class_names)
{
    _name=QString::fromStdString(name);
    _label=new QLabel(_name);
    _dataReady=false;
    _label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    _gestureNames=class_names;
    QFont f = _label->font();
    f.setBold(true);
    f.setItalic(true);

    _label->setFont(f);

    _label->setMinimumSize(200, 200);
    _label->setMaximumSize(500, 300);



    _yolo=nullptr;
    _onnx_path=onnx_path;
    _class_names=class_names;


   // _label->installEventFilter(this);
}


ModelOnnxReader::~ModelOnnxReader()
{
    if(_yolo)
        delete _yolo;
}

unsigned int ModelOnnxReader::nPorts(PortType portType) const
{
    unsigned int result = 1;

    switch (portType) {
    case PortType::In:
        result = 0;
        break;

    case PortType::Out:
        result = 1;

    default:
        break;
    }

    return result;
}



NodeDataType ModelOnnxReader::dataType(PortType const, PortIndex const) const
{
    return NodeNet().type();
}


std::shared_ptr<NodeData> ModelOnnxReader::outData(PortIndex)
{
  //  return _nodeData;
    //QString best_model_path=_data_path+"/.data/best.pt";
    QStringList name_list;
    for(auto &it:_class_names)
        name_list.append(QString::fromStdString(it));
    return std::make_shared<NodeNet>(QString::fromStdString(_onnx_path),name_list,true);

}


void ModelOnnxReader::setInData(std::shared_ptr<NodeData> nodeData, PortIndex const)
{
#if 0
    if (nodeData)
    {
        auto d = std::dynamic_pointer_cast<NodeImage>(nodeData);
        if (d)
        {
            if(_yolo==nullptr)
            {
#ifdef USE_CVDNN
                _yolo=new DnnYolo(_onnx_path,_class_names);
#else
                _yolo=new yolo_detect();
                _yolo->load_model(_onnx_path);
                _class_names=_yolo->get_class_names();
#endif
            }

            cv::Mat &dimage=d->image();
            if(!dimage.empty())
            {
                cv::Mat image=dimage.clone();
#ifdef USE_CVDNN
                std::vector<Detection> det_results;
                _yolo->detect(image,det_results);
                for(int i=0;i<det_results.size();i++)
                {
                    auto &res=det_results[i];
                    cv::Scalar color=color_map(res.class_id,_gestureNames.size());
                    cv::rectangle(image,res.box,color,2);
                   // cv::putText(d->image(),_gestureNames[res.class_id],res.box.tl()-cv::Point(0,20),color,cv::FontFace("simsun"),1.0,2);
                    _ftFont->putText(image, _gestureNames[res.class_id], res.box.tl()-cv::Point(0,20), 24, color, -1, 8, true);
                }
#else
               _yolo->detect({image});
               auto det_results=_yolo->get_result(0);
               for(int i=0;i<det_results.size();i++)
               {
                   auto &res=det_results[i];
                   cv::Rect rcBox=scale_boxes(image,res.box);
                   cv::Scalar color=color_map(res.classid,_class_names.size());
                   cv::rectangle(image,rcBox,color,2);
                  // cv::putText(d->image(),_gestureNames[res.class_id],res.box.tl()-cv::Point(0,20),color,cv::FontFace("simsun"),1.0,2);
                   _ftFont->putText(image, _class_names[res.classid], rcBox.tl()-cv::Point(0,20), 24, color, -1, 8, true);
               }
#endif

                _nodeData = std::make_shared<NodeImage>(image);

            }
        }
    }

    Q_EMIT dataUpdated(0);
#endif
}


