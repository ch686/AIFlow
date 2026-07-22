#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "model_image_reader.h"
#include "model_image_show.h"
#include "model_camera_reader.h"
#include "model_onnx_reader.h"
#include "model_net_train.h"
#include "model_net_predict.h"

#include "utils.h"
#include "darktreestyle.h"

#include <QtNodes/NodeDelegateModelRegistry>
#include <QtNodes/DataFlowGraphModel>
#include <QtNodes/DataFlowGraphicsScene>
#include <QtNodes/GraphicsView>

#include <QMessageBox>
#include <QFileDialog>
#include <QCloseEvent>
#include <QHeaderView>
#include <QPushButton>
#include <QtNodes/ConnectionStyle>
#include <QStyle>



static void setStyle2()
{
   QtNodes::ConnectionStyle::setConnectionStyle(
        R"(
  {
    "ConnectionStyle": {
      "FontColor": "#F8FAFC",
      "FontColorFaded": "#94A3B8",
      "ConstructionColor": "gray",
      "NormalColor": "black",
      "SelectedColor": "grey",
      "SelectedHaloColor": "deepskyblue",
      "HoveredColor": "deepskyblue",

      "LineWidth": 3.0,
      "ConstructionLineWidth": 2.0,
      "PointDiameter": 10.0,

      "UseDataDefinedColors": true
    }
  }
  )");

   QtNodes::NodeStyle ns;

   // 背景
   ns.setBackgroundColor(QColor(17,24,39));

   // 文字颜色（公有成员，直接=赋值）
   ns.FontColor = QColor("#22C55E");
   ns.FontColorFaded = QColor(148,163,184);

   // 边框
   ns.NormalBoundaryColor = QColor(51,65,85);
   ns.SelectedBoundaryColor = QColor(34,197,94);

   // 渐变
   ns.GradientColor0 = QColor(30,41,59);
   ns.GradientColor1 = QColor(26,36,54);
   ns.GradientColor2 = QColor(21,30,46);
   ns.GradientColor3 = QColor(16,23,42);

   // 阴影、线条、透明度
   ns.ShadowColor = QColor(10,10,10);
   ns.ShadowEnabled = true;
   ns.PenWidth = 1.2f;
   ns.Opacity = 0.98f;

   // 全局生效
   QtNodes::StyleCollection::setNodeStyle(ns);



}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , _graphModel(nullptr)
    , _scene(nullptr)
    , _nodeTreeWidget(nullptr)
{
    ui->setupUi(this);
    
    setWindowTitle("AIFlow - 节点编辑器");
    //resize(1280, 700);
    setStyle2();

   // ui->toolButton_2->setIcon(QIcon(":/new/prefix1/res/arrow_selector_tool_48dp_E3E3E3_FILL0_wght400_GRAD0_opsz48.svg"));
  //  ui->toolButton_2->setIconSize(QSize(48,48)); // 设置图标大小
    ui->toolButton->setText("移动\n模式");

   // ui->toolButton->setIcon(QIcon(":/new/prefix1/res/back_hand_48dp_E3E3E3_FILL0_wght400_GRAD0_opsz48.svg"));
   // ui->toolButton->setIconSize(QSize(48,48));
    ui->toolButton_2->setText("框选\n模式");

    _is_init_ui=false;
    util::g_main_window=this;
    ui->pushButton_run->hide();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showEvent(QShowEvent* event)
{
    if(!_is_init_ui)
    {
        setupNodeRegistry();
        setupGraphicsView();
        setupNodeListTree();

        connectSignals();

        _is_init_ui=true;
        ui->graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);

#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(winId());
    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(BOOL));
    // 刷新窗口让设置立即生效
    SetWindowPos(hwnd, nullptr,0,0,0,0, SWP_NOMOVE|SWP_NOSIZE|SWP_FRAMECHANGED);
#endif
    }
}


std::vector<std::string> gestureLabels = {
    "grabbing",
    "grip",
    "holy",
    "point",
    "call",
    "three3",
    "timeout",
    "xsign",
    "hand_heart",
    "hand_heart2",
    "little_finger",
    "middle_finger",
    "take_picture",
    "dislike",
    "fist",
    "four",
    "like",
    "mute",
    "ok",
    "one",
    "palm",
    "peace",
    "peace_inverted",
    "rock",
    "stop",
    "stop_inverted",
    "three",
    "three2",
    "two_up",
    "two_up_inverted",
    "three_gun",
    "thumb_index",
    "thumb_index2",
    "no_gesture"
};


void MainWindow::setupNodeRegistry()
{
    _registry = std::make_shared<QtNodes::NodeDelegateModelRegistry>();

    _registry->registerModel<ModelCameraReader>("图像模块");
    _registry->registerModel<ModelImageReader>("图像模块");
    _registry->registerModel<ModelImageShow>("图像模块");
#ifdef USE_CVDNN
    _registry->registerModel<ModelOnnxReader>([this]() {
         return std::make_unique<ModelOnnxReader>("手势识别","best_hagridv2.onnx",gestureLabels);
     },"预训练模型");
    _registry->registerModel<ModelOnnxReader>([this]() {
        return std::make_unique<ModelOnnxReader>("人脸识别","best_face.onnx",std::vector<std::string>{"face"});
     },"预训练模型");
#else
    _registry->registerModel<ModelOnnxReader>([this]() {
         return std::make_unique<ModelOnnxReader>("手势识别","best_hagridv2.torchscript",gestureLabels);
     },"预训练模型");
    _registry->registerModel<ModelOnnxReader>([this]() {
        return std::make_unique<ModelOnnxReader>("人脸识别","best_face.torchscript",std::vector<std::string>{"face"});
     },"预训练模型");
#endif
    _registry->registerModel<ModelNetTrain>("机器学习模块");
    _registry->registerModel<ModelNetPredict>("机器学习模块");


}

void MainWindow::setupNodeListTree()
{
    // 创建树形控件
    _nodeTreeWidget = ui->treeWidget;//new QTreeWidget();
//    _nodeTreeWidget->setAlternatingRowColors(false);
//    _nodeTreeWidget->setHeaderLabel("可用节点");
//    _nodeTreeWidget->setHeaderHidden(false);
//    _nodeTreeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
//    _nodeTreeWidget->setExpandsOnDoubleClick(false);
//    _nodeTreeWidget->setRootIsDecorated(true);

   _nodeTreeWidget->setHeaderLabel("可用节点");
   _nodeTreeWidget->setHeaderHidden(false);

   _nodeTreeWidget->setRootIsDecorated(true);
   _nodeTreeWidget->setItemsExpandable(true);
   _nodeTreeWidget->setExpandsOnDoubleClick(true);

   _nodeTreeWidget->setAlternatingRowColors(false);
   _nodeTreeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
   auto *treeStyle = new DarkTreeStyle;
   _nodeTreeWidget->setStyle(treeStyle);

   // 允许拖动内部条目
   _nodeTreeWidget->setDragEnabled(true);
   // 拖动复制，不移动原条目
   _nodeTreeWidget->setDragDropMode(QAbstractItemView::DragOnly);
   // 拖动时复制item数据
   _nodeTreeWidget->setDefaultDropAction(Qt::CopyAction);

//   qDebug() << qApp->style()->objectName();
//   qDebug() <<QApplication::style()->objectName();
    // 替换 listView 的 widget
    //ui->splitter->replaceWidget(0, _nodeTreeWidget);
    //ui->listView->hide(); // 隐藏原来的 listView
    
    // 从注册表获取所有节点类型并按分类组织
    auto registry = _registry;
    
    // 先创建分类
    std::unordered_map<QString, QTreeWidgetItem*> categoryItems;
    for (auto const &cat : registry->categories())
    {
        auto *item = new QTreeWidgetItem(_nodeTreeWidget);
        item->setText(0, cat);
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        categoryItems[cat] = item;
    }
    
    // 添加节点到对应分类
    for (auto const &assoc : registry->registeredModelsCategoryAssociation())
    {
        auto it = categoryItems.find(assoc.second);
        if (it != categoryItems.end())
        {
            auto *item = new QTreeWidgetItem(it->second);
            item->setText(0, assoc.first);
            item->setData(0, Qt::UserRole, assoc.first); // 存储节点类型
        }
    }
    
    _nodeTreeWidget->expandAll();
    
    // 双击创建节点
    connect(_nodeTreeWidget, &QTreeWidget::itemDoubleClicked, this, &MainWindow::OnItemDoubleClicked);

     ui->menu->addAction(ui->dockWidget->toggleViewAction());
}

void MainWindow::OnItemDoubleClicked(QTreeWidgetItem *item, int)
{
    if (!(item->flags() & Qt::ItemIsSelectable))
        return;

    QString nodeType = item->data(0, Qt::UserRole).toString();
    if (nodeType.isEmpty())
        return;

    createNode(nodeType);
}


#include <QtNodes/AbstractGraphModel>
#include <QMessageBox>

void MainWindow::createNode(QString const &nodeType, QPointF scenePos)
{
    if (nodeType.isEmpty())
        return;

    // ========== 新增：判断该类型节点是否已存在 ==========
    bool nodeAlreadyExists = false;
    // 遍历所有节点ID
    if(nodeType=="摄像头"||nodeType=="图像列表")
    {
        for (QtNodes::NodeId id : _graphModel->allNodeIds())
        {
            // 获取节点注册名称（和addNode传入的nodeType一致）
            QString existType = _graphModel->nodeData(id, QtNodes::NodeRole::Type).toString();
            if (existType == nodeType)
            {
                nodeAlreadyExists = true;
                break;
            }
        }
    }

    // 已存在则直接返回，禁止创建
    if (nodeAlreadyExists)
    {
        QMessageBox::information(this, "提示",
            QString("节点「%1」仅允许添加一个，画布已存在！").arg(nodeType));
        return;
    }
    // =================================================

    // 创建节点
    QtNodes::NodeId nodeId = _graphModel->addNode(nodeType);
    if (nodeId != QtNodes::InvalidNodeId)
    {
        qDebug() << "创建节点:" << nodeType << "ID:" << nodeId;

        // 如果是 ImageLoaderModel 类型，记录 NodeId 并连接双击信号
        if (nodeType == QStringLiteral("加载图像"))
        {
           // _imageLoaderIds.push_back(nodeId);
            //connectNodeDoubleClick(nodeId);
        }
        // 拖拽传入坐标则移动节点到鼠标位置
        if (!scenePos.isNull())
        {
            QVariant posVar = QVariant::fromValue(scenePos);
            _graphModel->setNodeData(nodeId, QtNodes::NodeRole::Position, posVar);
        }
    }
    else
    {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("无法创建节点：") + nodeType);
    }
}


//void MainWindow::OnNodeDoubleClick(int clickedNodeId)
//{
//    if (clickedNodeId == _nodeId)
//    {
//        // 获取节点模型
//        auto *model = _graphModel->delegateModel<ModelImageReader>(clickedNodeId);
//        if (model)
//        {
//            model->openFileDialog();
//        }
//    }
//}

//void MainWindow::connectNodeDoubleClick(int nodeId)
//{
//    // 连接场景的 nodeDoubleClicked 信号
//    _nodeId=nodeId;
//    connect(_scene, &QtNodes::DataFlowGraphicsScene::nodeDoubleClicked,
//            this,&MainWindow::OnNodeDoubleClick);
//}

void MainWindow::setupGraphicsView()
{
    _graphModel = new QtNodes::DataFlowGraphModel(_registry);

    // 创建数据流图形场景
    _scene = new QtNodes::DataFlowGraphicsScene(*_graphModel, this);
    _scene->setGroupingEnabled(false);
    // 获取 graphicsView 并设置场景
    QtNodes::GraphicsView *view=ui->graphicsView;
//    QtNodes::GraphicsView *view = qobject_cast<QtNodes::GraphicsView*>(ui->graphicsView);
//    if (!view)
//    {
//        QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("graphicsView 类型不正确"));
//        return;
//    }
    
    view->setScene(_scene);
    view->setRenderHint(QPainter::Antialiasing);
    view->setDragMode(QGraphicsView::RubberBandDrag);
    view->setAcceptDrops(true);
    
    // 居中显示
    view->centerScene();
}

void MainWindow::connectSignals()
{
    // 连接场景加载完成的信号
    connect(_scene, &QtNodes::DataFlowGraphicsScene::sceneLoaded, this, [](){
        qDebug() << "场景加载完成";
    });
}

bool MainWindow::save()
{
    if (_scene)
    {
        return _scene->save();
    }
    return false;
}

bool MainWindow::load()
{
    if (_scene)
    {
        return _scene->load();
    }
    return false;
}

void MainWindow::on_pushButton_run_clicked()
{
//    qDebug() << "[MainWindow] Run button clicked, propagating data from"
//             << _imageLoaderIds.size() << "ImageLoaderModel(s)";
    
//    // 通知所有 ImageLoaderModel 发送缓存的数据
//    int idx = 0;
//    for (int nodeId : _imageLoaderIds)
//    {
//        qDebug() << "[MainWindow] Loader" << idx << "NodeID:" << nodeId;
        
//        // 从 graphModel 获取模型
//        auto *loader = _graphModel->delegateModel<ImageLoaderModel>(nodeId);
//        if (loader)
//        {
//            qDebug() << "[MainWindow] 调用 propagateData...";
//            loader->propagateData();
//            qDebug() << "[MainWindow] propagateData 返回";
//        }
//        else
//        {
//            qDebug() << "[MainWindow] Loader" << nodeId << "获取失败，跳过";
//        }
//        idx++;
//    }
//    qDebug() << "[MainWindow] Run 完成";
}

void MainWindow::on_toolButton_clicked()
{
    ui->graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
}


void MainWindow::on_toolButton_2_clicked()
{
    ui->graphicsView->setDragMode(QGraphicsView::RubberBandDrag);
}

