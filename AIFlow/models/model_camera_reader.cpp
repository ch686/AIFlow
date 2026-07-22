#include "model_camera_reader.h"
#ifdef _WIN32
#include <objbase.h>
#include <dshow.h>
#include <oleauto.h>   // 新增：VariantInit 头文件
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "oleaut32.lib") // 新增：解决 VariantInit 链接缺失
#endif


#include <QtCore/QDir>
#include <QtCore/QEvent>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QComboBox>
#include <QMessageBox>

#include <QtWidgets/QFileDialog>
#include "utils.h"

QStringList ModelCameraReader::m_CamDevices;

CameraWorker::CameraWorker(QObject *parent)
    : QObject(parent)
{}

CameraWorker::~CameraWorker()
{
    stop();
}

void CameraWorker::start(int cameraIndex)
{
    _cameraIndex = cameraIndex;
    _isStop = false;

    if (!_vcp.open(_cameraIndex))
    {
        emit errorOccurred(QString::asprintf("打开摄像头 %d 失败", _cameraIndex));
        return;
    }
    _fps =  _vcp.get(cv::CAP_PROP_FPS);
    int w = _vcp.get(cv::CAP_PROP_FRAME_WIDTH);
    int h = _vcp.get(cv::CAP_PROP_FRAME_HEIGHT);;
    emit cameraInit(_fps,w,h);
    doGrab();
}

void CameraWorker::stop()
{
    QMutexLocker lock(&_mutex);
    _trigger = 0;
    _isStop = true;
    _cond.wakeAll();
}

// 外部调用：发送信号唤醒线程取一帧
void CameraWorker::wakeOneFrame()
{
    QMutexLocker lock(&_mutex);
    _trigger = 1;
    _cond.wakeOne();
}

void CameraWorker::wakeContinueFrame(bool is_stop)
{
    QMutexLocker lock(&_mutex);
    _trigger = is_stop?0:2;
    _cond.wakeOne();
}


void CameraWorker::doGrab()
{
    if (_cameraIndex < 0)
    {
        emit errorOccurred("未选择有效摄像头");
        return;
    }

    if (!_vcp.isOpened())
    {
        emit errorOccurred(QString::asprintf("打开摄像头 %d 失败", _cameraIndex));
        return;
    }

    int time_delay = qRound(1000.0 / _fps);
    cv::Mat frame;
    while (!_isStop)
    {
        // 上锁，等待触发信号或停止
        QMutexLocker lock(&_mutex);
        // 阻塞：只有 _trigger==true 或 _isStop==true 才会往下走
        while (_trigger==0 && !_isStop)
        {
            _cond.wait(&_mutex);
        }

        // 停止标记优先，直接退出循环
        if (_isStop)
            break;

        // 消耗触发标记，下一次继续等待
        if(_trigger==1)
            _trigger = 0;
        lock.unlock();

        if (!_vcp.read(frame))
        {
            emit errorOccurred("读取摄像头失败");
            break;
        }

        emit frameReady(frame.clone());
        std::this_thread::sleep_for(std::chrono::milliseconds(time_delay));
    }

    _vcp.release();
}

ModelCameraReader::ModelCameraReader()
{
    qRegisterMetaType<cv::Mat>("cv::Mat");

    InitUI();

    auto &devices = EnumCamera();
    _isRunning = false;
    _is_to_list =false;
    _fps = 0;
}

ModelCameraReader::~ModelCameraReader()
{
    if (_worker)
        _worker->stop();

    if (_grabThread)
    {
        _grabThread->quit();
        _grabThread->wait(30000);
        delete _grabThread;
        _grabThread=nullptr;
    }

    delete _worker;
   // _vcp.release();
}

void ModelCameraReader::InitUI()
{

    _widget=new QWidget();
    _widget->setWindowTitle("Form");
    // 顶层网格布局
    QGridLayout *gridLayout = new QGridLayout(_widget);

    // 垂直布局容器
    QVBoxLayout *vLayout = new QVBoxLayout();

    QHBoxLayout *hLayout1 = new QHBoxLayout();
    QHBoxLayout *hLayout2 = new QHBoxLayout();

    // 公共字体
    QFont font("微软雅黑", 9);

    QLabel *label = new QLabel("名称:");
    label->setFont(font);
    label->setAlignment(Qt::AlignRight);
    hLayout1->addWidget(label);


    _combo =  new QComboBox();
    auto & list=EnumCamera();
    //combo->clear();
    _combo->addItems(list);
    _combo->setCurrentIndex(0);
    hLayout1->addWidget(_combo);

    _pix1 = _widget->style()->standardPixmap(QStyle::SP_MessageBoxQuestion);
    _pix2 = _widget->style()->standardPixmap(QStyle::SP_DialogYesButton);
    _pix3 = _widget->style()->standardPixmap(QStyle::SP_DialogNoButton);
    _pix4 = _widget->style()->standardPixmap(QStyle::SP_MediaPlay);
    _pix5 = _widget->style()->standardPixmap(QStyle::SP_MediaStop);
    _label_state = new QLabel();
    _label_state->setPixmap(_pix1.scaled(32,
                               32,
                               Qt::KeepAspectRatio,
                               Qt::SmoothTransformation));
    hLayout1->addWidget(_label_state);
    _label_info= new QLabel("请选择摄像头");
    _checkbox= new QCheckBox("图像翻转");
    _check_state=false;

    _button1= new QPushButton("拍照");
    _button2= new QPushButton("视频");
    _button3= new QPushButton("拍照至列表");

    // 设置图标
    _button1->setIcon(QIcon(_pix4));
    _button1->setIconSize(QSize(32, 32));
    _button3->setIcon(QIcon(_pix4));
    _button3->setIconSize(QSize(32, 32));

    _button2->setIcon(QIcon(_pix4));
    // 可选：设置图标大小
    _button2->setIconSize(QSize(32, 32));

    _button1->setEnabled(false);
    _button2->setEnabled(false);
    _button3->setEnabled(false);
    _button2->hide();

    hLayout2->addWidget(_button1);
    hLayout2->addWidget(_button2);

    vLayout->addLayout(hLayout1, 0);
    vLayout->addWidget(_label_info, 0);
    vLayout->addWidget(_checkbox, 0);
    vLayout->addLayout(hLayout2, 0);
    vLayout->addWidget(_button3, 0);

    gridLayout->addLayout(vLayout, 0, 0);


   // _widget->installEventFilter(this);

    connect(_combo,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),this,&ModelCameraReader::on_combo_indexChanged);
    connect(_button1,&QPushButton::pressed,this,&ModelCameraReader::on_button1_pressed);
    connect(_button2,&QPushButton::pressed,this,&ModelCameraReader::on_button2_pressed);
    connect(_button3,&QPushButton::pressed,this,&ModelCameraReader::on_button3_pressed);
}

void ModelCameraReader::on_button3_pressed()
{
    _check_state=_checkbox->checkState()==Qt::CheckState::Checked;
    _worker->wakeOneFrame();
    _is_continue_trigger=false;
    _button2->setIcon(QIcon(_pix4));
    _is_to_list=true;
}

void ModelCameraReader::on_button1_pressed()
{
//    if(!_vcp.isOpened())
//    {
//        QMessageBox::information(nullptr, "提示", "摄像头未开启！");
//        return;
//    }
//   bool ret=_vcp.read(_image);
//   if(!ret)
//   {
//       QMessageBox::warning(nullptr, "错误", "摄像头拍照失败！");
//       return;
//   }
    _check_state=_checkbox->checkState()==Qt::CheckState::Checked;
    _worker->wakeOneFrame();
    _is_continue_trigger=false;
     _button2->setIcon(QIcon(_pix4));
     _is_to_list=false;
   // Q_EMIT dataUpdated(0);
}


void ModelCameraReader::on_button2_pressed()
{
//    if (!_vcp.isOpened())
//    {
//        QMessageBox::information(nullptr, "提示", "摄像头未开启！");
//        return;
//    }=true;
    //_button2->setIcon(QIcon(_pix5));
    if(!_is_continue_trigger)
    {
        _worker->wakeContinueFrame(false);
        _button2->setIcon(QIcon(_pix5));
    }
    else
    {
         _worker->wakeContinueFrame(true);
        _button2->setIcon(QIcon(_pix4));

    }
    _is_continue_trigger=!_is_continue_trigger;
}

void ModelCameraReader::on_combo_indexChanged(int index)
{
    if(_cameraIndex==index-1)
        return;
    _button1->setEnabled(false);
    _button2->setEnabled(false);
    _button3->setEnabled(false);
    _label_state->setPixmap(
            _pix3.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    if (_grabThread)
    {
        _worker->stop();
        _grabThread->quit();
        _grabThread->wait(30000);
        delete _grabThread;
        _grabThread=nullptr;
    }

    if (index - 1 < 0)
    {
        _cameraIndex = -1;
        return;
    }
    _combo->setEnabled(false);
    _cameraIndex = index - 1;



    if (!_grabThread)
    {
        _grabThread = new QThread(this);
        _worker = new CameraWorker();
        _worker->moveToThread(_grabThread);

        connect(_grabThread, &QThread::started, _worker, [this]() {
            _worker->start(_cameraIndex);
        });
        connect(_worker, &CameraWorker::frameReady, this, &ModelCameraReader::onFrameReady);
        connect(_worker, &CameraWorker::cameraInit, this, &ModelCameraReader::onCameraInit);
    }

    _isRunning = true;
    _grabThread->start();
}

void ModelCameraReader::onCameraInit(double fps, int w, int h)
{
    _label_state->setPixmap(
            _pix2.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    QString sInfo = QString::asprintf("%dx%d fps:%.1f", w, h, fps);
    _label_info->setText(sInfo);
    _combo->setEnabled(true);
    _button1->setEnabled(true);
    _button2->setEnabled(true);
    _button3->setEnabled(true);

}

void ModelCameraReader::onFrameReady(cv::Mat frame)
{
    if(_check_state)
        cv::flip(frame,_image,-1);
    else
        _image = frame.clone();
    Q_EMIT dataUpdated(0);
    if(_is_to_list)
        Q_EMIT dataUpdated(1);
}

const QStringList &ModelCameraReader::EnumCamera()
{
    bool done = false;
    int deviceCounter = 0;
    int cameraIndex = -1;

    // Initialize COM
    HRESULT hr = NULL;
    hr = CoInitialize(NULL);
    // always true when python is called
    //if (FAILED(hr))
    //{
    //	return cameraID;
    //}
    m_CamDevices.clear();
    m_CamDevices.append("请选择摄像头");
    //std::vector<std::string> cam_names;
    // Create the System Device Enumerator.
    ICreateDevEnum* pSysDevEnum = NULL;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, reinterpret_cast<void**>(&pSysDevEnum));
    if (FAILED(hr))
    {
        CoUninitialize();
        return m_CamDevices;
    }
    // Obtain a class enumerator for the video input category.
    IEnumMoniker* pEnumCat = NULL;
    hr = pSysDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnumCat, 0);
    if (hr == S_OK)
    {
        // Enumerate the monikers.
        IMoniker* pMoniker = NULL;
        ULONG cFetched;
        while ((!done) && (pEnumCat->Next(1, &pMoniker, &cFetched) == S_OK))
        {
            // Bind the first moniker to an object
            IPropertyBag* pPropBag;
            hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropBag);
            if (SUCCEEDED(hr))
            {
                // To retrieve the filter's DevicePath, do the following:
                VARIANT varName;
                varName.vt = VT_BSTR;
                VariantInit(&varName);

                hr = pPropBag->Read(L"FriendlyName", &varName, 0);
                if (SUCCEEDED(hr))
                {
                    // convert DevicePath to char*
                    char devicePath[256] = { 0 };
                    int count = 0;
                    while (varName.bstrVal[count] != 0x00)
                    {
                        devicePath[count] = varName.bstrVal[count];
                        count++;
                    }
                    QString devicePathString = devicePath; // lower string

                    // to lower
                    //std::string hwidString = hwid;
                    //std::transform(hwidString.begin(), hwidString.end(), hwidString.begin(), ::tolower);

                    // match
                    //int matched = devicePathString.find(hwidString);
                    //if (matched >= 0)
                    //{
                    //	cameraIndex = deviceCounter;
                    //	done = true;
                    //}
                    m_CamDevices.append(devicePathString);
                }

                deviceCounter++;

                VariantClear(&varName);
                pPropBag->Release();
                pPropBag = NULL;
            }
            pMoniker->Release();
            pMoniker = NULL;
        }
        pEnumCat->Release();
        pEnumCat = NULL;
    }
    pSysDevEnum->Release();
    pSysDevEnum = NULL;
    CoUninitialize();

    return m_CamDevices;
}

unsigned int ModelCameraReader::nPorts(PortType portType) const
{
    unsigned int result = 1;

    switch (portType) {
    case PortType::In:
        result = 0;
        break;

    case PortType::Out:
        result = 2;

    default:
        break;
    }

    return result;
}


NodeDataType ModelCameraReader::dataType(PortType const, PortIndex const index) const
{
    if(index==0)
        return NodeImage().type();
    else
        return NodeAddImage().type();

}

std::shared_ptr<NodeData> ModelCameraReader::outData(PortIndex index)
{
    if(index==0)
        return std::make_shared<NodeImage>(_image);
    else
        return std::make_shared<NodeAddImage>(_image);

}
