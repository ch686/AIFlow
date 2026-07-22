#include "traindlg.h"
#include "utils.h"
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QStatusBar>
#include <QToolBar>
#include <QDockWidget>
#include <QAction>
#include <QActionGroup>
#include <QMainWindow>
#include <QSplitter>
#include <QMessageBox>
#include <QInputDialog>
#include <QDebug>
#include <QPushButton>
#include <QDir>
#include <QDialogButtonBox>
#include "rectshape.h"
#include "qcustomplot.h"



TrainDlg::TrainDlg(QWidget *parent)
    : QDialog(parent)
{
    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::WindowMaximizeButtonHint;
    setWindowFlags(flags);

    QVBoxLayout *vLayout = new QVBoxLayout(this);
    vLayout->setContentsMargins(4,4,4,4);
    vLayout->setSpacing(4);
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->setContentsMargins(2, 2, 2, 2);
    mainLayout->setSpacing(2);
    // ========== 创建绘图控件 ==========
    m_plot = new QCustomPlot(this);
    // 曲线1：Train Loss
    m_plot->addGraph();
    m_plot->graph(0)->setPen(QPen(QColor(80, 160, 255), 2));
    m_plot->graph(0)->setName("Train Loss");

    // 曲线2：Val Loss
    m_plot->addGraph();
    m_plot->graph(1)->setPen(QPen(QColor(255, 160, 60), 2));
    m_plot->graph(1)->setName("Val Loss");

    // 坐标轴文字
    m_plot->xAxis->setLabel("Epoch");
    m_plot->yAxis->setLabel("Loss Value");
    // 显示图例
    m_plot->legend->setVisible(true);
    // 开启网格辅助查看
    m_plot->xAxis->grid()->setVisible(true);
    m_plot->yAxis->grid()->setVisible(true);

    // X轴默认范围 0 ~ 50000
    m_plot->xAxis->setRange(0, 10);
    // Y轴给一个初始区间，避免空白界面
    m_plot->yAxis->setRange(0, 20.0);

    // 数据容器初始化
    m_epochData.clear();
    m_trainLossData.clear();
    m_valLossData.clear();

    // ========== 按钮区域 ==========
    _buttonBox = new QDialogButtonBox(this);
    _buttonBox->setOrientation(Qt::Horizontal);
    _buttonBox->setStandardButtons(QDialogButtonBox::Ok);

    QPushButton *okBtn = _buttonBox->button(QDialogButtonBox::Ok);
    if (okBtn)
    {
        okBtn->setText("停止训练");
    }

    // 布局装载：绘图占大部分空间，按钮底部
    _label= new QLabel("数据总数:0");
    _label->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    _label2= new QLabel("");
    _label2->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    _progress1 = new QProgressBar(this);
    _progress1->setRange(0, 100);
    _progress1->setFormat("%p%");
    _progress1->setValue(0);
    _progress1->setTextVisible(true);

    _progress2 = new QProgressBar(this);
    _progress2->setRange(0, 100);
    _progress2->setFormat("%p%");
    _progress2->setValue(0);
    _progress2->setTextVisible(true);

    mainLayout->addWidget(m_plot, 1);
    mainLayout->addWidget(_progress1, 0);
    mainLayout->addWidget(_progress2, 0);
    mainLayout->addWidget(_label, 0);
    mainLayout->addWidget(_label2, 0);
    vLayout->addLayout(mainLayout, 1);  // 拉伸系数1，填满窗口
    vLayout->addWidget(_buttonBox, 0);

    _detector=new Detector(this);
    connect(_detector, &Detector::sigTrainProgress, this, &TrainDlg::OnTrainProgress);
    connect(_detector, &Detector::sigTrainLoss, this, &TrainDlg::OnTrainLoss);

    //connect(_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(_buttonBox, &QDialogButtonBox::clicked, this,&TrainDlg::OnAbstractButton);

    setMinimumSize(800, 600);
    QMainWindow *pWnd=util::getTopMainWindow(this);
    if(pWnd)
    {
        QSize size=pWnd->size();
        resize(size.width()/3*2, size.height()/3*2);
    }
    else
        resize(1200, 800);
    _is_init=false;
}


void TrainDlg::showEvent(QShowEvent* event)
{
    if(!_is_init)
    {


        _is_init=true;


#ifdef Q_OS_WIN
        HWND hwnd = reinterpret_cast<HWND>(winId());
        BOOL dark = TRUE;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(BOOL));
        // 刷新窗口让设置立即生效
        SetWindowPos(hwnd, nullptr,0,0,0,0, SWP_NOMOVE|SWP_NOSIZE|SWP_FRAMECHANGED);

        // 深色模式图表配色
        // ===================== QCustomPlot 深色全套配色 =====================
        // 画布背景
        m_plot->setBackground(QBrush(QColor(28, 28, 30)));
        // 绘图区域（曲线区域）背景
        m_plot->axisRect()->setBackground(QBrush(QColor(38, 38, 40)));

        // 坐标轴轴线
        m_plot->xAxis->setBasePen(QPen(Qt::white, 1));
        m_plot->yAxis->setBasePen(QPen(Qt::white, 1));
        m_plot->xAxis2->setBasePen(QPen(Qt::white, 1));
        m_plot->yAxis2->setBasePen(QPen(Qt::white, 1));

        // 坐标轴标签文字（Epoch / Loss Value）
        m_plot->xAxis->setLabelColor(Qt::white);
        m_plot->yAxis->setLabelColor(Qt::white);

        // 刻度数字颜色
        m_plot->xAxis->setTickLabelColor(Qt::white);
        m_plot->yAxis->setTickLabelColor(Qt::white);

        // 网格线浅灰色，弱化不刺眼
        m_plot->xAxis->grid()->setPen(QPen(QColor(80,80,80), 1, Qt::DotLine));
        m_plot->yAxis->grid()->setPen(QPen(QColor(80,80,80), 1, Qt::DotLine));
        m_plot->xAxis->grid()->setSubGridPen(QPen(QColor(60,60,60), 1, Qt::DotLine));
        m_plot->yAxis->grid()->setSubGridPen(QPen(QColor(60,60,60), 1, Qt::DotLine));

        // ========== 图例（右上角白色看不清核心修复） ==========
        // 图例背景深色
        m_plot->legend->setBrush(QBrush(QColor(50,50,55)));
        // 图例边框浅灰
        m_plot->legend->setBorderPen(QPen(QColor(100,100,100)));
        // 图例文字白色
        m_plot->legend->setTextColor(Qt::white);
        // 图例字号微调
        m_plot->legend->setFont(QFont("Arial", 9));

        // 曲线颜色保持亮蓝色，对比明显
        //m_plot->graph(0)->setPen(QPen(QColor(80, 180, 255), 2));

        // X轴默认范围 0 ~ 50000
        m_plot->xAxis->setRange(0, _num_epochs);
        // Y轴给一个初始区间，避免空白界面
        m_plot->yAxis->setRange(0, 20.0);

        // 开启鼠标交互
        m_plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

        // 滚轮只作用于Y轴（可选，默认XY同时缩放）
        // m_plot->axisRect()->setRangeZoomAxes(m_plot->xAxis, m_plot->yAxis);
        // 只允许滚轮缩放Y，X不动：
        m_plot->axisRect()->setRangeZoomAxes(nullptr, m_plot->yAxis);

        // 拖拽仅Y轴
        m_plot->axisRect()->setRangeDragAxes(m_plot->xAxis, m_plot->yAxis);

#endif
        m_autoRescaleY=true;
        if(_best_loss>0)
            _label2->setText(QString::asprintf("最佳Loss:%.6f",_best_loss));
        StartTrain();
    }
}


bool TrainDlg::eventFilter(QObject *object, QEvent *event)
{
    // 只处理列表的滚轮事件

    return QDialog::eventFilter(object, event);
}

void TrainDlg::resizeEvent(QResizeEvent *event)
{

}

void TrainDlg::OnTrainProgress(bool is_train,int cur_epoch, int total_epoch, int batch_cnt,int total_batch)
{
    static QString label_text;
    QString cur_label_text=QString::asprintf("训练轮数:%d/%d %s...(%d/%d) ",cur_epoch,total_epoch,
                                             is_train?"训练中":"验证中",batch_cnt,total_batch);
    if(cur_label_text!=label_text)
    {
        _label->setText(cur_label_text);
        label_text=cur_label_text;
    }
    static int pct1=0;
    int cur_pct1=(float)cur_epoch*100/total_epoch;
    if(pct1!=cur_pct1)
    {
        _progress1->setValue(cur_pct1);
    }

    static int pct2=0;
    int cur_pct2=(float)batch_cnt*100/total_batch;
    if(pct2!=cur_pct2)
    {
        _progress2->setValue(cur_pct2);
    }
}

void TrainDlg::OnTrainLoss(bool is_train,int cur_epoch, int total_epoch,float loss)
{
    if(_best_loss>loss||_best_loss<0)
        _best_loss=loss;
    _label2->setText(QString::asprintf("当前Loss:%.6f 最佳Loss:%.6f",loss,_best_loss));
    appendLossData(is_train,cur_epoch, loss);
}


void TrainDlg::appendLossData(bool is_train,int epoch, double loss)
{
    // 同一epoch只存一次X
    if (m_epochData.size() <= epoch)
    {
        m_epochData.append(epoch);
        m_trainLossData.append(0);
        m_valLossData.append(0);
    }

    if (is_train)
    {
        m_trainLossData[epoch] = loss;
    }
    else
    {
        m_valLossData[epoch] = loss;
    }

    // 分别绑定两条曲线数据
    m_plot->graph(0)->setData(m_epochData, m_trainLossData);
    m_plot->graph(1)->setData(m_epochData, m_valLossData);

    // 自动缩放坐标轴
    //m_plot->xAxis->rescale();
    if(m_autoRescaleY)
    {
        m_plot->yAxis->setRange(0, std::min(loss,20.0));

        //m_plot->yAxis->rescale();
        m_autoRescaleY = false;
    }
    m_plot->replot();
}

void TrainDlg::clearPlotData()
{
    m_epochData.clear();
    m_trainLossData.clear();
    m_valLossData.clear();
    m_plot->graph(0)->setData({}, {});
    m_plot->graph(1)->setData({}, {});
    m_plot->replot();
}

void TrainDlg::closeEvent(QCloseEvent *event)
{
    if(!OnClose())
    {
        event->ignore();
        return;
    }
    QDialog::accept();

}

bool TrainDlg::OnClose()
{
    // 防止重复启动训练
    if (m_trainThread != nullptr && m_trainThread->isRunning())
    {
        // 参数：父窗口、标题、提示文字、按钮组合、默认选中按钮
        QMessageBox::StandardButton ret = QMessageBox::question(
                    this,
                    "提示",
                    "正在训练中是否退出？",
                    QMessageBox::Ok | QMessageBox::Cancel,  // 显示确定、取消两个按钮
                    QMessageBox::Cancel                        // 默认焦点在确定按钮
                    );

        if (ret == QMessageBox::Ok)
        {
            _detector->StopTrain();
            //QDialog::accept();
        }
        return false;
    }
    else
    {
        return true;
    }
}

void TrainDlg::OnAbstractButton(QAbstractButton* btn)
{
    if (btn == _buttonBox->button(QDialogButtonBox::Ok))
    {
        if(OnClose())
        {
            QDialog::accept();
        }
    }
}

// 线程结束回调，释放资源
void TrainDlg::onTrainThreadFinished()
{
    bool is_finish=_detector->_is_finish;
    if (m_trainThread)
    {
        m_trainThread->deleteLater();
        m_trainThread = nullptr;
        if(is_finish)
            QMessageBox::information(this, "完成", "本轮训练全部结束！");
        else
            QMessageBox::information(this, "完成", "训练提前停止！");
        QPushButton *okBtn = _buttonBox->button(QDialogButtonBox::Ok);
        if (okBtn)
        {
            okBtn->setText("退出");
        }

    }
}


void TrainDlg::StartTrain()
{
    if (m_trainThread != nullptr && m_trainThread->isRunning())
    {
        QMessageBox::information(this, "提示", "训练正在进行中，请勿重复点击！");
        return;
    }

    // 1. 初始化检测器
    _detector->InitializeTrain(0, 256, 256, _exporter, _data_folder.toUtf8().constData());
    // 2. 创建子线程
    m_trainThread = new QThread(this);
    // Detector移入子线程，Train函数会在子线程执行
    _detector->moveToThread(m_trainThread);
    // 3. 线程启动时执行训练
    //connect(m_trainThread, &QThread::started, this,&TrainDlg::StartTrain);
    _detector->_num_epochs=_num_epochs;
    _detector->_batch_size=_batch_size;
    _detector->_learning_rate=_learning_rate;
    _detector->_best_loss=_best_loss;
    _detector->_is_pre=_is_pre;
    connect(m_trainThread, &QThread::started, _detector, &Detector::Train);
    // 4. 训练完成停止线程
    connect(_detector, &Detector::sigTrainFinished, m_trainThread, &QThread::quit);
    // 线程结束后清理指针
    connect(m_trainThread, &QThread::finished, this, &TrainDlg::onTrainThreadFinished);

    // 【关键】绑定Detector进度信号到UI更新函数（跨线程自动排队）


    // 启动线程，自动执行Train
    m_trainThread->start();
}
