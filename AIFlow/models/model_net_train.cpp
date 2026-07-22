#include "model_net_train.h"


#include <QtCore/QDir>
#include <QtCore/QEvent>

#include <QtWidgets/QFileDialog>
#include "utils.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include "traindlg.h"

ModelNetTrain::ModelNetTrain()
{
    //_label=new QLabel(QStringLiteral("双击加载图像"));
    //_dataReady=false;
    //_label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);

//    QFont f = _label->font();
//    f.setBold(true);
//    f.setItalic(true);

//    _label->setFont(f);

//    _label->setMinimumSize(200, 200);
 //   _label->setMaximumSize(500, 300);

 //   _label->installEventFilter(this);
    InitUI();
    //_is_reload=false;
}

void ModelNetTrain::InitUI()
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




    _label= new QLabel("类别名称");
    _label->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    vLayout->addWidget(_label,1);


    _list =  new QListWidget();
    vLayout->addWidget(_list,100);

    _label2= new QLabel("数据总数:0");
    _label2->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    vLayout->addWidget(_label2,1);

    auto add_combo=[](QComboBox *combo,const QString&text,const QStringList&items)
    {
        QHBoxLayout *hLayout = new QHBoxLayout();
        QLabel *label= new QLabel(text);
        combo->addItems(items);
        combo->setCurrentIndex(0);
        hLayout->addWidget(label,0);
        hLayout->addWidget(combo,0);
        return hLayout;
    };
    _combo1 =  new QComboBox();
    QHBoxLayout *hLayout1=add_combo(_combo1,"训练轮数:",{"10","20","30","40","50"});
    _combo1->setCurrentIndex(1);
    vLayout->addLayout(hLayout1,1);
    _combo2 =  new QComboBox();
    QHBoxLayout *hLayout2=add_combo(_combo2,"训练批次:",{"4","8","16"});
    vLayout->addLayout(hLayout2,1);
    _combo3 =  new QComboBox();
    QHBoxLayout *hLayout3=add_combo(_combo3,"学习率:",{"0.001","0.005","0.010"});
    vLayout->addLayout(hLayout3,1);
    _checkbox= new QCheckBox("预训练模型");
    _checkbox->setCheckState(Qt::CheckState::Checked);
    vLayout->addWidget(_checkbox,1);


    _label3= new QLabel("未训练");
    _label3->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    vLayout->addWidget(_label3,1);


    _button1= new QPushButton("开始训练");
    vLayout->addWidget(_button1);


    gridLayout->addLayout(vLayout, 0, 0);


   // _widget->installEventFilter(this);

    //connect(_combo,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),this,&ModelCameraReader::on_combo_indexChanged);
    connect(_button1,&QPushButton::pressed,this,&ModelNetTrain::on_button1_pressed);

   _widget->setMinimumSize(400, 400);
   _widget->setMaximumSize(500, 600);
}


unsigned int ModelNetTrain::nPorts(PortType portType) const
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





void ModelNetTrain::on_button1_pressed()
{
    if(_data_path.isEmpty()||_exporter==nullptr)
    {
        QMessageBox::information(nullptr, "提示", "请输入训练数据！");
        return;
    }
    if(_exporter->DataCount()<20)
    {
        QMessageBox::information(nullptr, "提示", "训练数据需要至少20条！");
        return;

    }
//    _combo1 =  new QComboBox();
//    QHBoxLayout *hLayout1=add_combo(_combo1,"训练轮数:",{"10","20","30"});
//    vLayout->addLayout(hLayout1,1);
//    _combo2 =  new QComboBox();
//    QHBoxLayout *hLayout2=add_combo(_combo2,"训练批次:",{"5","10","15"});
//    vLayout->addLayout(hLayout2,1);
//    _combo3 =  new QComboBox();
//    QHBoxLayout *hLayout3=add_combo(_combo3,"学习率:",{"0.001","0.005","0.010"});
    std::string gbk_data_dir=util::utf8_to_gbk(_data_path.toUtf8().constData());
    std::string best_model_path=gbk_data_dir+"/.data/best.pt";
    std::string best_path=gbk_data_dir+"/.data/best_loss.txt";


    std::ifstream fs1(best_model_path);
    std::ifstream fs2(best_path);
    float best_loss=-1;
    if(fs1.is_open()&&fs2.is_open())
    {
        fs1.close();
        fs2.close();
        best_loss=util::loadBestLoss(best_path);
        _label3->setText(QString::asprintf("已训练 最佳Loss:%.6f",best_loss));
    }


    TrainDlg dlg;
    dlg._data_folder=_data_path;
    dlg._exporter=_exporter;
    dlg._num_epochs=_combo1->currentText().toInt();
    dlg._batch_size=_combo2->currentText().toInt();
    dlg._learning_rate=_combo3->currentText().toFloat();
    dlg._best_loss=best_loss;
    dlg._is_pre=_checkbox->checkState()==Qt::CheckState::Checked;
    dlg.exec();


    fs1.open(best_model_path);
    fs2.open(best_path);
    if(fs1.is_open()&&fs2.is_open())
    {
        fs1.close();
        fs2.close();
        float best_loss=util::loadBestLoss(best_path);
        _label3->setText(QString::asprintf("已训练 最佳Loss:%.6f",best_loss));
    }
    //_is_reload=true;
    QString best_model_path1=_data_path+"/.data/best.pt";
    if(_nodeNet==nullptr)
        _nodeNet=std::make_shared<NodeNet>(best_model_path1,_class_names);
    _nodeNet->set_reload(true);
    Q_EMIT dataUpdated(0);

}

//bool ModelNetTrain::eventFilter(QObject *object, QEvent *event)
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

NodeDataType ModelNetTrain::dataType(PortType const type, PortIndex const index) const
{
    if(type==PortType::In)
        return NodeDataset().type();
    else
        return NodeNet().type();

}

std::shared_ptr<NodeData> ModelNetTrain::outData(PortIndex index)
{
    if(_data_path!="")
    {
        QString best_model_path=_data_path+"/.data/best.pt";
        if(_nodeNet==nullptr)
            _nodeNet=std::make_shared<NodeNet>(best_model_path,_class_names);

        return _nodeNet;
    }
    else
        return nullptr;
}

void ModelNetTrain::setInData(std::shared_ptr<NodeData> nodeData, PortIndex const)
{
    if (nodeData)
    {
        auto d = std::dynamic_pointer_cast<NodeDataset>(nodeData);
        if (d)
        {
            _data_path=d->data_dir();
            if(_data_path.isEmpty())
                return;
            QString dbpath=_data_path+"/.data/data.db";
            //QMessageBox::information(nullptr, "提示", "1");
            if (!QFile::exists(dbpath))
            {
                //QMessageBox::information(nullptr, "提示", "2");
                if(_exporter)
                    _exporter.reset();
                _data_path="";
                Q_EMIT dataUpdated(0);
                return;
            }
            //QMessageBox::information(nullptr, "提示", "3");
            if(_exporter)
                _exporter.reset();
            _exporter=std::make_shared<Exporter>(dbpath);

            _class_names=_exporter->get_class_names();
            _list->clear();
            if(_class_names.size()>0)
                _list->addItems(_class_names);
            _label2->setText(QString::asprintf("数据总数:%d",_exporter->DataCount()));

            std::string gbk_data_dir=util::utf8_to_gbk(_data_path.toUtf8().constData());
            std::string best_model_path=gbk_data_dir+"/.data/best.pt";
            std::string best_path=gbk_data_dir+"/.data/best_loss.txt";


            std::ifstream fs1(best_model_path);
            std::ifstream fs2(best_path);
            if(fs1.is_open()&&fs2.is_open())
            {
                fs1.close();
                fs2.close();
                float best_loss=util::loadBestLoss(best_path);
                _label3->setText(QString::asprintf("已训练 最佳Loss:%.6f",best_loss));
            }
            //update_image(d->image());
        }
    }
    else
    {
        if(_exporter)
            _exporter.reset();
        _data_path="";
    }

    Q_EMIT dataUpdated(0);
}
