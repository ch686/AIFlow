#include "model_image_show.h"



#include <QtNodes/NodeDelegateModelRegistry>

#include <QtCore/QDir>
#include <QtCore/QEvent>
#include <QtWidgets/QFileDialog>
#include <QTimer>
#include <QVBoxLayout>

ModelImageShow::ModelImageShow()
{

    _widget=new QWidget();
    _widget->setWindowTitle("Form");

    // 垂直布局容器
    QVBoxLayout *vLayout = new QVBoxLayout(_widget);


    _label=new QLabel("显示图像");

    _label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);

    QFont f = _label->font();
    f.setBold(true);
    f.setItalic(true);

    _label->setFont(f);

    _label->setMinimumSize(640, 480);

    _label->installEventFilter(this);

    vLayout->addWidget(_label);

    // 主线程定时器：30ms刷新一次UI，主线程专属，不存在跨线程
    _uiRefreshTimer.setInterval(30);
    connect(&_uiRefreshTimer, &QTimer::timeout, this, &ModelImageShow::onTimerRefresh);
    _uiRefreshTimer.start();
}

unsigned int ModelImageShow::nPorts(PortType portType) const
{
    unsigned int result = 1;

    switch (portType) {
    case PortType::In:
        result = 1;
        break;

    case PortType::Out:
        result = 1;

    default:
        break;
    }

    return result;
}

QPixmap ModelImageShow::matToPixmap(const cv::Mat &mat)
{
    if (mat.empty())
        return QPixmap();

    QImage::Format imgFormat;
    cv::Mat tempMat = mat.clone(); // 深度拷贝杜绝悬空data

    switch (tempMat.channels())
    {
    case 1:
        imgFormat = QImage::Format_Grayscale8;
        break;
    case 3:
        imgFormat = QImage::Format_RGB888;
        cv::cvtColor(tempMat, tempMat, cv::COLOR_BGR2RGB);
        break;
    case 4:
        imgFormat = QImage::Format_RGBA8888;
        cv::cvtColor(tempMat, tempMat, cv::COLOR_BGRA2RGBA);
        break;
    default:
        return QPixmap();
    }

    QImage img(tempMat.data, tempMat.cols, tempMat.rows,
               static_cast<int>(tempMat.step), imgFormat);
    return QPixmap::fromImage(img.copy());
}

// ========== 关键修改：这个函数【只存图，不触发任何UI刷新】==========
void ModelImageShow::update_image(const cv::Mat& image)
{
    std::lock_guard<std::mutex> lock(_mtx);
    image.copyTo(_frameBuf);
}

void ModelImageShow::onTimerRefresh()
{
    // 加锁取出最新图像
    cv::Mat showMat;
    {
        std::lock_guard<std::mutex> lock(_mtx);
        if (!_frameBuf.empty())
            showMat = _frameBuf.clone();
    }

    if (showMat.empty())
        return;

    int w = _label->width();
    int h = _label->height();
    if (w <= 10 || h <= 10)
    {
        w = _label->minimumWidth();
        h = _label->minimumHeight();
    }

    QPixmap pix = matToPixmap(showMat);
    _label->setPixmap(pix.scaled(w, h, Qt::KeepAspectRatio));
}

bool ModelImageShow::eventFilter(QObject *object, QEvent *event)
{
//    if (object == _label && event->type() == QEvent::Resize)
//    {
//        auto d = std::dynamic_pointer_cast<NodeImage>(_nodeData);
//        if (d)
//        {
//            // 不要频繁直接调用update_image，改用单次排队防抖
//            static QTimer resizeTimer;
//            resizeTimer.setSingleShot(true);
//            resizeTimer.setInterval(50);
//            connect(&resizeTimer, &QTimer::timeout, this, [=](){
//                update_image(d->image());
//            }, Qt::UniqueConnection);
//            resizeTimer.start();
//        }
//    }
    return false;
}


NodeDataType ModelImageShow::dataType(PortType const, PortIndex const) const
{
    return NodeImage().type();
}

std::shared_ptr<NodeData> ModelImageShow::outData(PortIndex)
{
    return _nodeData;
}
void ModelImageShow::setInData(std::shared_ptr<NodeData> nodeData, PortIndex const)
{
    _nodeData = nodeData;
    if (_nodeData)
    {
        auto d = std::dynamic_pointer_cast<NodeImage>(_nodeData);
        if (d)
        {
            update_image(d->image());
        }
    }
    else
    {
        std::lock_guard<std::mutex> lock(_mtx);
        _frameBuf.release();
        _label->setPixmap(QPixmap());
    }
    Q_EMIT dataUpdated(0);
}

