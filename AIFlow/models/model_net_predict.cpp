#include "model_net_predict.h"


#include <QtCore/QDir>
#include <QtCore/QEvent>

#include <QtWidgets/QFileDialog>
#include "utils.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QMessageBox>
cv::Ptr<cv::freetype::FreeType2> ModelNetPredict::_ftFont=nullptr;

ModelNetPredict::ModelNetPredict()
{
    _label=new QLabel(name());
    _dataReady=false;
    _label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    QFont f = _label->font();
    f.setBold(true);
    f.setItalic(true);

    _label->setFont(f);

    _label->setMinimumSize(200, 200);
    _label->setMaximumSize(500, 300);

 //   _label->installEventFilter(this);
    _detector=nullptr;
    _yolo=nullptr;
    if(!_ftFont)
    {
        _ftFont =cv::freetype::createFreeType2();
        _ftFont->loadFontData( "C:/Windows/Fonts/simhei.ttf", 0 );
    }
    //InitUI();
}

ModelNetPredict::~ModelNetPredict()
{
    if(_yolo!=nullptr)
    {
        delete _yolo;
        _yolo=nullptr;
    }
}

void ModelNetPredict::InitUI()
{

    _widget=new QWidget();
    _widget->setWindowTitle("Form");
    // 顶层网格布局
    QGridLayout *gridLayout = new QGridLayout(_widget);

    // 垂直布局容器
    QVBoxLayout *vLayout = new QVBoxLayout();

   // QHBoxLayout *hLayout1 = new QHBoxLayout();
   // QHBoxLayout *hLayout2 = new QHBoxLayout();

    // 公共字体
    QFont font("微软雅黑", 9);



    _button1= new QPushButton("导入图片");
    vLayout->addWidget(_button1);

    _label= new QLabel("请导入图片");
    _label->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    vLayout->addWidget(_label,1);


    gridLayout->addLayout(vLayout, 0, 0);


   // _widget->installEventFilter(this);

    //connect(_combo,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),this,&ModelCameraReader::on_combo_indexChanged);
    connect(_button1,&QPushButton::pressed,this,&ModelNetPredict::on_button1_pressed);
}


unsigned int ModelNetPredict::nPorts(PortType portType) const
{
    unsigned int result = 1;

    switch (portType) {
    case PortType::In:
        result = 2;
        break;

    case PortType::Out:
        result = 1;

    default:
        break;
    }

    return result;
}





void ModelNetPredict::on_button1_pressed()
{

}

//bool ModelNetPredict::eventFilter(QObject *object, QEvent *event)
//{
//    if (object == _label)
//    {
//        int w = _label->width();
//        int h = _label->height();

////        if (event->type() == QEvent::Resize)
////        {
////            if (!_pixmap.isNull() && w > 0 && h > 0)
////            {
////                _label->setPixmap(_pixmap.scaled(w, h, Qt::KeepAspectRatio));
////            }
////        }
//    }

//    return false;
//}

NodeDataType ModelNetPredict::dataType(PortType const type, PortIndex const index) const
{
    if(type==PortType::In)
    {
        if(index==0)
            return NodeImage().type();
        else
            return NodeNet().type();
    }
    else
        return NodeImage().type();

}


cv::Scalar ModelNetPredict::color_map(int n, int total)
{
    int hue = (n % total) * 360 / total;

    // 让不同类别亮度有轻微变化
    int sat = 80 + (3-(n % 3)) * 80;   // 200~240
    int val = 80 + (3-(n % 3)) * 80;   // 200~230

    cv::Mat color_hsv(1, 1, CV_8UC3, cv::Scalar(hue / 2, sat, val));
    cv::cvtColor(color_hsv, color_hsv, cv::COLOR_HSV2BGR);

    cv::Vec3b bgr = color_hsv.at<cv::Vec3b>(0, 0);
    return cv::Scalar(bgr[2], bgr[1], bgr[0]);
}

cv::Rect ModelNetPredict::scale_boxes(const cv::Mat& image, const cv::Rect2f& box)
{
    cv::Rect rect;
    rect.x = cvRound((box.x - box.width / 2) * image.cols);
    rect.y = cvRound((box.y - box.height / 2) * image.rows);
    rect.width = cvRound(box.width * image.cols);
    rect.height = cvRound(box.height * image.rows);
    if (rect.x < 0)rect.x = 0;
    if (rect.y < 0)rect.y = 0;
    if (rect.x + rect.width > image.cols)rect.width -= (rect.x + rect.width - image.cols);
    if (rect.y + rect.height > image.rows)rect.height -= (rect.y + rect.height - image.rows);

    return rect;
}

void ModelNetPredict::setInData(std::shared_ptr<NodeData> nodeData, PortIndex const portIndex)
{
    if(portIndex==0)
    {
        _nodeData1 = nodeData;
        if(_nodeData3!=nullptr)
        {
            _nodeData3.reset();
        }
    }
    else
    {
        _nodeData2 = nodeData;
        if(_nodeData2==nullptr)
        {
            if(_detector!=nullptr)
            {
                delete _detector;
                _detector=nullptr;
            }
            if(_yolo!=nullptr)
            {
                delete _yolo;
                _yolo=nullptr;
            }
        }
    }

    auto d1 = std::dynamic_pointer_cast<NodeImage>(_nodeData1);
    auto d2 = std::dynamic_pointer_cast<NodeNet>(_nodeData2);
    if (d1!=nullptr&&d2!=nullptr)
    {
        if(d2->is_reload())
        {
            if(_detector!=nullptr)
            {
                delete _detector;
                _detector=nullptr;
            }
            if(_yolo!=nullptr)
            {
                delete _yolo;
                _yolo=nullptr;
            }
            d2->set_reload(false);
        }
        if(!d2->is_onnx())
        {
            if(_detector==nullptr)
            {
                _detector=new Detector(this);
                _detector->InitializePredict(0,256,256,d2->class_names());
                _detector->LoadWeight(util::utf8_to_gbk(d2->model_path().toUtf8().constData()));
            }

            cv::Mat image=d1->image().clone();
            if(!image.empty())
            {
                _detector->Predict(image);
                _nodeData3 = std::make_shared<NodeImage>(image);
            }
        }
        else
        {
            if(_yolo==nullptr)
            {
                _yolo=new yolo_detect();
                _yolo->load_model(util::utf8_to_gbk(d2->model_path().toUtf8().constData()));
                _class_names=_yolo->get_class_names();
            }

            cv::Mat image=d1->image().clone();
            if(!image.empty())
            {
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
                _nodeData3 = std::make_shared<NodeImage>(image);
            }

        }
    }
    else if (d1!=nullptr)
    {
        _nodeData3=_nodeData1;
    }
    Q_EMIT dataUpdated(0);

}

std::shared_ptr<NodeData> ModelNetPredict::outData(PortIndex index)
{
    return _nodeData3;
}
