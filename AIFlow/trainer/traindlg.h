#pragma once
#include <QDialog>
#include <QProgressBar>
#include <QLabel>
#include <QDialogButtonBox>
#include <QThread>

#include "qcustomplot.h"
#include"Detector.h"
#include "exporter.h"

class TrainDlg: public QDialog
{
    Q_OBJECT
public:
    explicit TrainDlg(QWidget *parent = nullptr);
    QString _data_folder;
    std::shared_ptr<Exporter> _exporter;
    int _num_epochs;
    int _batch_size;
    float _learning_rate;
    float _best_loss=-1;
    bool _is_pre=false;
protected:
    void showEvent(QShowEvent* event) override;
    bool eventFilter(QObject *object, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
public slots:
    //void on_list_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);
    // 外部调用，追加一组 epoch-loss 数据并刷新曲线
    void appendLossData(bool is_train,int epoch, double loss);
    // 清空曲线数据
    void clearPlotData();
    void OnAbstractButton(QAbstractButton* btn);

    void OnTrainProgress(bool is_train,int cur_epoch, int total_epoch, int batch_cnt,int total_batch);
    void OnTrainLoss(bool is_train,int cur_epoch, int total_epoch,float loss);
    void onTrainThreadFinished();

private:
    bool OnClose();
    void StartTrain();
    QThread * m_trainThread = nullptr;
    QCustomPlot* m_plot;
    QVector<double> m_epochData;   // x轴 epoch
    QVector<double> m_trainLossData;// y轴 loss
    QVector<double> m_valLossData;
    bool _is_init;
    QLabel* _label;
    QLabel* _label2;
    QProgressBar* _progress1;
    QProgressBar* _progress2;
    QDialogButtonBox *_buttonBox;
    Detector *_detector;
    bool m_autoRescaleY;
};

