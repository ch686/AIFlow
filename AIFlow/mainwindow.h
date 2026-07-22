#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStringListModel>
#include <QTreeWidget>
#include <memory>
#include <vector>
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtNodes/DataFlowGraphModel>
#include <QtNodes/DataFlowGraphicsScene>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class ImageLoaderModel;



class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    bool save();
    bool load();
    void createNode(QString const &nodeType, QPointF scenePos = QPointF(0,0));
    void on_pushButton_run_clicked();
    void OnItemDoubleClicked(QTreeWidgetItem *item, int);
   // void OnNodeDoubleClick(int clickedNodeId);
protected:
    void showEvent(QShowEvent* event) override;
private slots:
    void on_toolButton_clicked();

    void on_toolButton_2_clicked();

private:
    int _nodeId;
    void setupNodeRegistry();
    void setupNodeListTree();
    void setupGraphicsView();
    void connectSignals();
    //void connectNodeDoubleClick(int nodeId);
    
    // 跟踪所有 ImageLoaderModel 的 NodeId
private:
    Ui::MainWindow *ui;
    
    // 节点注册表
    std::shared_ptr<QtNodes::NodeDelegateModelRegistry> _registry;
    // 数据流图模型
    QtNodes::DataFlowGraphModel *_graphModel;
    
    // 数据流图形场景
    QtNodes::DataFlowGraphicsScene *_scene;
    
    // 节点列表树形控件
    QTreeWidget *_nodeTreeWidget;
    bool _is_init_ui;


};
#endif // MAINWINDOW_H
