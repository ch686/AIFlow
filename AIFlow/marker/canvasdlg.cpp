#include "canvasdlg.h"
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
#include <QTimer>
#include "rectshape.h"

#define NEW_CLASS_NAME "新增类别"

CanvasDlg::CanvasDlg(QWidget *parent)
    : QDialog(parent)
{

    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::WindowMaximizeButtonHint;    // 显示最大/最小按钮
      //  flags |= Qt::WindowResizable;            // 允许鼠标拖拽边框缩放窗口
    setWindowFlags(flags);

    // 顶层垂直布局，直接绑定对话框，删除多余gridLayout
    QVBoxLayout *vLayout = new QVBoxLayout(this);
    QWidget* container1 = new QWidget(this);
    QWidget* container2 = new QWidget(this);
    QVBoxLayout *vLayout1 = new QVBoxLayout(container1);
    QVBoxLayout *vLayout2 = new QVBoxLayout(container2);
    vLayout->setContentsMargins(5, 5, 5, 5);
    vLayout->setSpacing(2);

    // 中间水平布局（工具栏 + 画布），父设为vLayoutQSplitter
//    QHBoxLayout *mainLayout = new QHBoxLayout();
//    mainLayout->setContentsMargins(2, 2, 2, 2);
//    mainLayout->setSpacing(2);

    QSplitter *mainLayout = new QSplitter(Qt::Horizontal, this);
    // 分割条粗细
    mainLayout->setHandleWidth(4);
    // 可选：设置初始左右宽度比例（左工具栏占1份，画布占10份）
    //mainLayout->setSizes({100, 2000});

    // 画布，传入父this
    _view = new CanvasView(this);
    _scene =new Canvas(this);
    _view->setScene(_scene);
    // 垂直工具栏，指定父
   // QToolBar *toolBar = new QToolBar("工具栏", this);
   // toolBar->setOrientation(Qt::Vertical);
    //toolBar->setSpacing(4);
    //toolBar->setMovable(false);
    //toolBar->setStyleSheet("QToolBar { border: none; }");
    QSplitter *vSplitter = new QSplitter(Qt::Vertical, this);


    // Action 全部指定父this
    //QAction *actionOpen = new QAction("打开目录", this);
   // QAction *actionRect = new QAction("矩形标注", this);
   // QAction *actionPoly = new QAction("多边形标注", this);
   // QAction *actionPoint = new QAction("点标注", this);
  //  QAction *actionRBox = new QAction("旋转框标注", this);

    // ActionGroup 已有父this，无泄漏
//    QActionGroup *modeGroup = new QActionGroup(this);
//    QList<QAction*> acts({actionRect});//, actionPoly, actionPoint, actionRBox});
//    for (auto &act : acts)
//    {
//        act->setCheckable(true);
//        modeGroup->addAction(act);
//    }
//    actionRect->setChecked(true);
    _scene->setMode(CanvasMode::RECT);

    //toolBar->addAction(actionOpen);
    //toolBar->addSeparator();
    QLabel *label1 =  new QLabel("数据类型");
    label1->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    //_checkbox =  new QCheckBox("新增数据类型");

    _list_class =  new QListWidget();
    _list_class->setItemDelegate(new ListColorDelegate(this));
    vLayout1->addWidget(label1,0);
    vLayout1->addWidget(_list_class,1);
    vSplitter->addWidget(container1);

    //toolBar->addActions(acts); // 简化循环添加
    //toolBar->addSeparator();

    QLabel *label2 =  new QLabel("图片列表");
    label2->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    _list =  new QListWidget();
    vLayout2->addWidget(label2,0);
    vLayout2->addWidget(_list,1);
    vSplitter->addWidget(container2);


    // 工具栏+画布加入水平布局
    mainLayout->addWidget(vSplitter);
    mainLayout->addWidget(_view); // 拉伸系数给画布

    // 底部按钮框，已传this无泄漏
    QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons( QDialogButtonBox::Ok);
    QPushButton *okBtn = buttonBox->button(QDialogButtonBox::Ok);
    if (okBtn)
    {
        okBtn->setText("退出");
    }
    // 水平布局、按钮框依次加入顶层垂直布局
    vLayout->addWidget(mainLayout);
    vLayout->addWidget(buttonBox);

    // 新式信号槽，替代老旧SIGNAL/SLOT
    connect(buttonBox, &QDialogButtonBox::accepted, this, &CanvasDlg::on_accept);
    //connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(_list,&QListWidget::currentItemChanged,this,&CanvasDlg::on_list_currentItemChanged);
    connect(_list_class,&QListWidget::itemDoubleClicked,this,&CanvasDlg::on_list_class_itemDoubleClicked);
    connect(_scene,&Canvas::shapeDrawn,this,&CanvasDlg::handle_new_shape);

    // 可选：限制窗口最小尺寸，防止缩太小
    setMinimumSize(800, 600);
    // 默认打开大小
    QMainWindow *pWnd=util::getTopMainWindow(this);
    if(pWnd)
    {
        QSize size=pWnd->size();
        resize(size.width()/3*2, size.height()/3*2);
    }
    else
        resize(1200, 800);
    _is_init=false;
    _list->installEventFilter(this);
    mainLayout->setStretchFactor(0, 1);
    mainLayout->setStretchFactor(1, 20);
    mainLayout->setSizes({100, 1100});
    vSplitter->setStretchFactor(0, 1);
    vSplitter->setStretchFactor(1, 3);
}

void CanvasDlg::classnames_to_db()
{
    if(_exporter==nullptr)
        return;
    QString class_names;
    int num=_list_class->count();
    for(int i=0;i<num;i++)
    {
       if(i>0)
       {
            class_names+=_list_class->item(i)->text();
            if(i!=num-1)
                class_names+=",";
       }
    }
    if(!class_names.isEmpty())
    {
        _exporter->SetConfig("model_det", "class_names",class_names);
    }
}

void CanvasDlg::on_list_class_itemDoubleClicked(QListWidgetItem *item)
{
    QString cls_name=item->text();
    if(cls_name==NEW_CLASS_NAME)
    {
        return;
    }
    bool ok=false;
    QString new_cls_name = QInputDialog::getText(
        this,                // 父窗口，CanvasDlg* this
        "修改类别名称",          // 标题
        "新类别名称:", // 提示文本
         QLineEdit::Normal,
        cls_name,
        &ok
    );
    if(ok)
    {
        if(cls_name==new_cls_name||new_cls_name=="")
            return;
        else
        {
            for(int i=1;i<_list_class->count();i++)
            {
                QListWidgetItem *item=_list_class->item(i);
                if(new_cls_name==item->text())
                {
                    QMessageBox::information(nullptr, "提示", "输入名称已存在");
                    return;
                }
            }
            item->setText(new_cls_name);

            classnames_to_db();
            auto item=_list->currentItem();
            on_list_currentItemChanged(item,nullptr);

        }
    }
}

void CanvasDlg::showEvent(QShowEvent* event)
{
    if(!_is_init)
    {

        QListWidgetItem *item=nullptr;

        QSize main_size=util::g_main_window->size();
        this->resize(main_size.width()/3*2,main_size.height()/3*2);
        this->move(main_size.width()/6,main_size.height()/6);

        QString dbpath=_folderPath+"/.data";
        QDir dir;
        dir.mkdir(dbpath);
        dbpath=dbpath+"/data.db";

        _exporter=std::make_shared<Exporter>(dbpath);
        if(_exporter->_query==nullptr)
        {
            QTimer::singleShot(0, this, &CanvasDlg::close);
            return;
        }

        item=new QListWidgetItem(NEW_CLASS_NAME);
        item->setData(Qt::UserRole+10, QColor(88,88,88));
        item->setData(Qt::UserRole + 11, QColor(Qt::white));
        _list_class->addItem(item);
        //QListWidgetItem* item = _list_class->item(0); // 获取第0行
        if(item)
        {
            _list_class->setCurrentItem(item);
            // 同时高亮选中
            _list_class->setSelectionMode(QAbstractItemView::SingleSelection);
            item->setSelected(true);
        }

        auto &class_names=_exporter->get_class_names();
        for(auto &name:class_names)
        {
            int idx=_list_class->count()-1;
            QColor bkcolor=Exporter::getClassMapColor(idx);
            item=new QListWidgetItem(name);
            _list_class->setItemDelegate(new ListColorDelegate(this));
            item->setData(Qt::UserRole + 10, bkcolor);
            item->setData(Qt::UserRole + 11, QColor(Qt::white));
            _list_class->addItem(item);
        }
//        if(_list_class->count()>0)
//        {
//            QListWidgetItem* item = _list_class->item(0); // 获取第0行
//            if(item)
//            {
//                _list_class->setCurrentItem(item);
//                // 同时高亮选中
//                _list_class->setSelectionMode(QAbstractItemView::SingleSelection);
//                item->setSelected(true);
//            }
//        }

        _list->clear();
        item=nullptr;
        for(auto it : _list_items)
        {
            if(item==nullptr)
                item=it;
            _list->addItem(it);
        }
        _is_init=true;
        //this->showMaximized();
        _list->setCurrentItem(item);
        _list->setSelectionMode(QAbstractItemView::SingleSelection);
        item->setSelected(true);
        on_list_currentItemChanged(item,nullptr);


#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(winId());
    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(BOOL));
    // 刷新窗口让设置立即生效
    SetWindowPos(hwnd, nullptr,0,0,0,0, SWP_NOMOVE|SWP_NOSIZE|SWP_FRAMECHANGED);
#endif

    }
}


bool CanvasDlg::eventFilter(QObject *object, QEvent *event)
{
    // 只处理列表的滚轮事件
    if (object == _list)
    {
        if(event->type() == QEvent::Resize)
        {
            _view->fitInView(_scene->sceneRect(), Qt::KeepAspectRatio);
        }
    }
    return QDialog::eventFilter(object, event);
}

void CanvasDlg::resizeEvent(QResizeEvent *event)
{
    _view->fitInView(_scene->sceneRect(), Qt::KeepAspectRatio);

}


void CanvasDlg::on_list_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    QStringList class_list;
    for(int i=0; i<_list_class->count(); i++)
    {
        auto item = _list_class->item(i);
        if(i>0)
            class_list<<item->text();
    }
    if (previous)
    {
        QString path = previous->data(Qt::UserRole+1).toString();
        int w = previous->data(Qt::UserRole+2).toInt();
        int h = previous->data(Qt::UserRole+3).toInt();
        _exporter->save_shapes(_scene,previous->text(),QSize(w,h),class_list);
    }
    if (current)
    {
        QString path = current->data(Qt::UserRole+1).toString();
        QString name =current->text();
        _scene->loadImage(path);
        current->setData(Qt::UserRole+2,_scene->ImgW());
        current->setData(Qt::UserRole+3,_scene->ImgH());
        _exporter->load_shapes(_scene,name,class_list);

        _view->fitInView(_scene->sceneRect(), Qt::KeepAspectRatio);
    }
}

void CanvasDlg::closeEvent(QCloseEvent *event)
{
    auto item=_list->currentItem();
    on_list_currentItemChanged(nullptr,item);

}

void CanvasDlg::on_accept()
{
    auto item=_list->currentItem();
    on_list_currentItemChanged(nullptr,item);
    QDialog::accept();
}

void CanvasDlg::handle_new_shape(QGraphicsItem * shape)
{
    _scene->addItem(shape);
    if(shape->type()==RectShape::Type)
    {

        QListWidgetItem* cur_item = _list_class->currentItem();
        if(cur_item->text()!=NEW_CLASS_NAME)
        {
            int idx=_list_class->currentIndex().row()-1;
            ((RectShape*)shape)->updateLabelText(cur_item->text(),idx);
            ((RectShape*)shape)->updateLabelPosition(shape);
            QColor bkcolor=Exporter::getClassMapColor(idx);
            QColor bkcolor1=  bkcolor;
            QColor bkcolor2=  bkcolor;
            bkcolor1.setAlpha(50);
            bkcolor2.setAlpha(120);
            ((RectShape*)shape)->setupStyle(((RectShape*)shape),bkcolor,
                                    bkcolor1,
                                    bkcolor2);
            return;
        }


        // 1. 准备下拉列表
        QStringList class_list;
        //class_list << "类别A" << "类别B" << "类别C" << "类别D";
        for(int i=0; i<_list_class->count(); i++)
        {
            auto item = _list_class->item(i);
            if(i>0)
                class_list<<item->text();
        }
        if(class_list.size()==0)
        {
            class_list<<"类别1";

        }

        int default_idx = 0; // 默认选中第1项（类别B）
        bool ok;
INPUT_CLS_NAME:
        // 2. 弹出选择输入框
        QString cls_name = QInputDialog::getItem(
            this,                // 父窗口，CanvasDlg* this
            "输入类别",          // 标题
            "请选择或输入类别名称:", // 提示文本
            class_list,
            default_idx,
            true,                // 可手动输入，对应Python True
            &ok
        );


        // 3. 判断用户操作
        if (ok)
        {
            if(cls_name==NEW_CLASS_NAME||cls_name.isEmpty())
            {
                QMessageBox::information(nullptr, "提示", "请输入类别名称");
                goto INPUT_CLS_NAME;
            }
            // 用户点击确定，cls_name 为选中/输入的文本
            qDebug() << "选中类别：" << cls_name;
            int idx=-1;
            bool is_find=false;
            QColor bkcolor;
            for(int i=0; i<_list_class->count(); i++)
            {
                auto item = _list_class->item(i);
                if(item->text()==cls_name)
                {
                    is_find=true;
                    idx=i-1;
                    bkcolor=Exporter::getClassMapColor(idx);
                    break;
                }
            }
            if(!is_find)
            {
                idx=_list_class->count()-1;
                bkcolor=Exporter::getClassMapColor(idx);
                QListWidgetItem *item=new QListWidgetItem(cls_name);
                _list_class->setItemDelegate(new ListColorDelegate(this));
                item->setData(Qt::UserRole + 10, bkcolor);
                item->setData(Qt::UserRole + 11, QColor(Qt::white));
                _list_class->addItem(item);

                classnames_to_db();
            }
            ((RectShape*)shape)->updateLabelText(cls_name,idx);
            ((RectShape*)shape)->updateLabelPosition(shape);

            QColor bkcolor1=  bkcolor;
            QColor bkcolor2=  bkcolor;
            bkcolor1.setAlpha(50);
            bkcolor2.setAlpha(120);
            ((RectShape*)shape)->setupStyle(((RectShape*)shape),bkcolor,
                                    bkcolor1,
                                    bkcolor2);

        }
        else
        {
            _scene->removeItem(shape);

            // 用户取消弹窗
            qDebug() << "取消输入";
        }


    }
}
