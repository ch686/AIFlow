#include "model_image_reader.h"


#include <QtCore/QDir>
#include <QtCore/QEvent>

#include <QtWidgets/QFileDialog>
#include "utils.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QCoreApplication>
#include <QDateTime>
#include "canvasdlg.h"

ModelImageReader::ModelImageReader()

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
}

void ModelImageReader::InitUI()
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
    //QFont font("微软雅黑", 9);



    _button1= new QPushButton("导入图片");
    vLayout->addWidget(_button1);

    _label= new QLabel("请导入图片");
    _label->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    vLayout->addWidget(_label,1);

    _list =  new QListWidget();
    vLayout->addWidget(_list,100);

    _button2= new QPushButton("标注图片");
    vLayout->addWidget(_button2);
    _button3= new QPushButton("重置数据");
    vLayout->addWidget(_button3);


    gridLayout->addLayout(vLayout, 0, 0);


   // _widget->installEventFilter(this);

    //connect(_combo,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),this,&ModelCameraReader::on_combo_indexChanged);
    connect(_button1,&QPushButton::pressed,this,&ModelImageReader::on_button1_pressed);
    connect(_button2,&QPushButton::pressed,this,&ModelImageReader::on_button2_pressed);
    connect(_button3,&QPushButton::pressed,this,&ModelImageReader::on_button3_pressed);
    connect(_list,&QListWidget::itemClicked,this,&ModelImageReader::on_list_itemClicked);
   //connect(_button2,&QPushButton::pressed,this,&ModelCameraReader::on_button2_pressed);
    _button3->hide();

   // _list->installEventFilter(this);

}


unsigned int ModelImageReader::nPorts(PortType portType) const
{
    unsigned int result = 1;

    switch (portType) {
    case PortType::In:
        result = 1;
        break;

    case PortType::Out:
        result = 2;

    default:
        break;
    }

    return result;
}




QStringList ModelImageReader::LoadFolder(const QString& path,const QStringList& tags)
{
    QStringList file_list;
    QDir dir(path);
    file_list.clear();

    foreach(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot |
        QDir::Files |
        QDir::Dirs))
    {
        if (info.isDir())
        {
           // LoadFolder(info.absoluteFilePath());
            continue;
        }
        else
        {
            //qDebug() << "absoluteFilePath:" << info.absoluteFilePath();
            //qDebug() << "fileName:" << info.fileName();
            QString  path_lower=info.absoluteFilePath().toLower();
            bool add=false;
            foreach(QString tag,tags)
            {
                QString  tag_lower=tag.toLower();
                if (path_lower.indexOf(tag_lower) != -1)
                {
                    add=true;
                    break;
                }
            }
            if(add)
                file_list.append(info.absoluteFilePath());
        }
    }

    return file_list;
}

void ModelImageReader::on_list_itemClicked(QListWidgetItem *item)
{
    if (item)
    {
        QString path = item->data(Qt::UserRole+1).toString();
        _image = cv::imread(util::utf8_to_gbk(path.toStdString().c_str()));
        if (_image.empty()) {
           // qDebug() << "[ImageLoader] 图片加载失败!";
           QMessageBox::warning(nullptr, "错误", "图片加载失败");
            return;
        }
        _label->setText(QString::asprintf("图片尺寸:%dx%d",_image.cols,_image.rows));

        Q_EMIT dataUpdated(0);
    }
}


static bool removeDir(const QString &dirPath)
{
    QDir dir(dirPath);
    if (!dir.exists())
        return true;

    // 获取目录下所有文件、子文件夹（不包含 . 和 ..）
    dir.setFilter(QDir::NoDotAndDotDot | QDir::AllEntries);
    QFileInfoList list = dir.entryInfoList();

    for (const QFileInfo &fileInfo : list)
    {
        QString absPath = fileInfo.absoluteFilePath();
        if (fileInfo.isDir())
        {
            // 子目录递归删除
            removeDir(absPath);
        }
        else
        {
            // 文件直接删除
            QFile::remove(absPath);
        }
    }
    // 清空内容后删除空文件夹
    return dir.rmdir(dirPath);
}


void ModelImageReader::on_button3_pressed()
{
    if(_folderPath=="./camera")
    {
        QMessageBox::StandardButton ret = QMessageBox::question(
                    _widget,
                    "提示",
                    "确定删除所有拍照数据与标注训练数据？",
                    QMessageBox::Ok | QMessageBox::Cancel,  // 显示确定、取消两个按钮
                    QMessageBox::Cancel                        // 默认焦点在确定按钮
                    );

        if (ret == QMessageBox::Ok)
        {
            removeDir("./camera");
            _list->clear();

        }
    }
}


void ModelImageReader::on_button2_pressed()
{
    if(_list->count()==0)
    {
        QMessageBox::information(nullptr, "提示", "请先加载图片！");
        return;
    }
    CanvasDlg dlg;
    dlg._folderPath=_folderPath;
    //QList<QListWidgetItem*> res;
    for(int i=0; i<_list->count(); i++)
    {
        auto oldItem = _list->item(i);
        // 克隆条目（文字、图标、选中、颜色全部复制）
        QListWidgetItem* newItem = oldItem->clone();
        dlg._list_items.append(newItem);
    }

    dlg.exec();
    Q_EMIT dataUpdated(1);
}


void ModelImageReader::on_button1_pressed()
{
    // 弹出一个文件夹选择对话框
    QString folderPath = QFileDialog::getExistingDirectory(
                nullptr,                    // 父窗口，此处为nullptr表示没有父窗口
                "选择图片文件夹",        // 对话框标题
                QDir::currentPath(),                // 默认目录，空字符串代表系统默认目录
                QFileDialog::ShowDirsOnly   // 只显示文件夹，不显示文件
                );

    // 检查用户是否选择了文件夹
    if (!folderPath.isEmpty())
    {
        qDebug() << "用户选择的文件夹路径是:" << folderPath;
        _folderPath=folderPath;
        QStringList label_list = LoadFolder(folderPath, { ".jpg",".bmp",".png" });

        for (int i = 0; i < label_list.size(); i++)
        {
            QString label_path = label_list.at(i);
            QString label_name = label_path.section('/', -1);
            label_name = label_name.section('.', 0, 0);

            // 1. 创建Item，设置显示文字
            QListWidgetItem* item = new QListWidgetItem(label_name);
            // 2. 把完整路径存入自定义数据位 Qt::UserRole
            item->setData(Qt::UserRole+1, label_path);
            // 3. 添加到列表
            _list->addItem(item);
        }

        if(label_list.size()>0)
        {
            _list->setCurrentRow(0);
                // 自动滚动到可视区域
            _list->scrollToTop();
            QListWidgetItem* curItem = _list->currentItem();
            on_list_itemClicked(curItem);
        }
        Q_EMIT dataUpdated(1);
    }

//    int w = _label->width();
//    int h = _label->height();

//    QString fileName = QFileDialog::getOpenFileName(nullptr,
//                                                    tr("Open Image"),
//                                                    QDir::homePath(),
//                                                    tr("Image Files (*.png *.jpg *.bmp)"));

//    if (fileName.isEmpty()) {
//        qDebug() << "[ImageLoader] 用户取消了文件选择";
//        return;
//    }

//    qDebug() << "[ImageLoader] 选择的文件:" << fileName;
    

//    _image = cv::imread(util::utf8_to_gbk(fileName.toStdString().c_str()));
    
//    if (_image.empty()) {
//        qDebug() << "[ImageLoader] 图片加载失败!";
//        _label->setText(" 图片加载失败!");
//        return;
//    }

//    _label->setText(QString::asprintf("图片尺寸:%dx%d",_image.cols,_image.rows));

}

//bool ModelImageReader::eventFilter(QObject *object, QEvent *event)
//{
//    // 只处理列表的滚轮事件
//    if (object == _list)
//    {
//        if(event->type() == QEvent::Wheel)
//        {
//            QWheelEvent* wheelEv = static_cast<QWheelEvent*>(event);
//            // 先让列表执行原生滚动
//            QCoreApplication::sendEvent(_list, wheelEv);
//            // 消费事件，阻止向上冒泡到GraphView缩放
//            event->accept();
//            return true;
//        }
//    }
//    return QtNodes::NodeDelegateModel::eventFilter(object, event);
//}

NodeDataType ModelImageReader::dataType(PortType const type, PortIndex const index) const
{
    if(type==PortType::Out)
    {
        if(index==0)
            return NodeImage().type();
        else
            return NodeDataset().type();
    }
    else
    {
        return NodeAddImage(false).type();

    }

}

void ModelImageReader::setInData(std::shared_ptr<NodeData> nodeData, PortIndex const portIndex)
{
    if (nodeData)
    {
        auto d = std::dynamic_pointer_cast<NodeAddImage>(nodeData);
        if (d)
        {
            //update_image(d->image());
            _folderPath="./camera";
            QDir dir;
            dir.mkpath(_folderPath);
            if(_list->count()==0)
            {
                QStringList label_list = LoadFolder(_folderPath, { ".jpg",".bmp",".png" });
                for (int i = 0; i < label_list.size(); i++)
                {
                    QString label_path = label_list.at(i);
                    QString label_name = label_path.section('/', -1);
                    label_name = label_name.section('.', 0, 0);

                    // 1. 创建Item，设置显示文字
                    QListWidgetItem* item = new QListWidgetItem(label_name);
                    // 2. 把完整路径存入自定义数据位 Qt::UserRole
                    item->setData(Qt::UserRole+1, label_path);
                    // 3. 添加到列表
                    _list->addItem(item);
                }
            }

            QString file_name=QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz");
            QString file_path = _folderPath+"/"+file_name+".png";
            if(!d->image().empty())
            {
                cv::imwrite(util::utf8_to_gbk(file_path.toUtf8().constData()),d->image());
                QListWidgetItem* item = new QListWidgetItem(file_name);
                // 2. 把完整路径存入自定义数据位 Qt::UserRole
                item->setData(Qt::UserRole+1, file_path);
                // 3. 添加到列表
                _list->addItem(item);
                _list->setCurrentRow(_list->count()-1);
                    // 自动滚动到可视区域
                QListWidgetItem* curItem = _list->currentItem();
                on_list_itemClicked(curItem);
                _list->scrollToItem(curItem);

            }
            _button3->show();
            _button1->hide();
            Q_EMIT dataUpdated(1);
        }
    }
    else
    {
        _folderPath="";
        _list->clear();
        _button3->hide();
        _button1->show();
        Q_EMIT dataUpdated(1);
    }
}

std::shared_ptr<NodeData> ModelImageReader::outData(PortIndex index)
{
    if(index==0)
        return std::make_shared<NodeImage>(_image);
    else
    {
        if(_folderPath=="")
            return nullptr;
        return std::make_shared<NodeDataset>(_folderPath);
    }

}
