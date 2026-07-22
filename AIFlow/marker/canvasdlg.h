#ifndef CANVASDLG_H
#define CANVASDLG_H
#include <QDialog>
#include "canvasview.h"
#include <QListWidget>
#include <QResizeEvent>
#include <opencv2/opencv.hpp>
#include "canvas.h"
#include <QDialogButtonBox>

#include <QStyledItemDelegate>
#include <QPainter>
#include <QCheckBox>
#include "exporter.h"

class ListColorDelegate : public QStyledItemDelegate
{
public:
    explicit ListColorDelegate(QObject* parent = nullptr)
        : QStyledItemDelegate(parent)
    {}

    void paint(QPainter* painter, const QStyleOptionViewItem& opt, const QModelIndex& idx) const override
    {
        painter->save();
        QRect rect = opt.rect;

        // 读取自定义颜色数据，不存在则用默认
        QColor bg = idx.data(Qt::UserRole +10).value<QColor>();
        QColor fg = idx.data(Qt::UserRole +11).value<QColor>();

        // 选中态优先使用系统选中色
        int offsetX=0;
        if (bg.isValid()&&(opt.state & QStyle::State_Selected))
        {
          //  painter->fillRect(rect, opt.palette.highlight());
           // painter->setPen(opt.palette.highlightedText().color());

            // 填充选中背景 #1E293B
            painter->fillRect(rect, QColor("#1E293B"));
            // 左侧4px绿色竖条 #22C55E
            QRect leftBar(rect.x(), rect.y(), 4, rect.height());
            painter->fillRect(rect, bg);
            painter->fillRect(leftBar, QColor("#22C55E"));
            // 文字绿色
            painter->setPen(QColor("#22C55E"));
            offsetX=2;

        }
        else if (bg.isValid())
        {
            // 有自定义底色，覆盖全局QSS
            painter->fillRect(rect, bg);
            painter->setPen(fg.isValid() ? fg : Qt::white);
        }
        else
        {
            // 无自定义颜色，沿用全局QSS原始样式
            QStyledItemDelegate::paint(painter, opt, idx);
            painter->restore();
            return;
        }

        // 绘制文字
        QString text = idx.data(Qt::DisplayRole).toString();
        painter->drawText(rect.adjusted(6+offsetX,0,-6,0), Qt::AlignVCenter, text);
        painter->restore();
    }
};


class CanvasDlg: public QDialog
{
    Q_OBJECT
public:
    explicit CanvasDlg(QWidget *parent = nullptr);
    QList<QListWidgetItem*> _list_items;
    QString _folderPath;
protected:
    void showEvent(QShowEvent* event) override;
    bool eventFilter(QObject *object, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

public slots:
    void on_list_class_itemDoubleClicked(QListWidgetItem *item);
    void on_list_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);
    void handle_new_shape(QGraphicsItem * shape);
    void on_accept();
private:
    void classnames_to_db();
    std::shared_ptr<Exporter> _exporter;
    CanvasView *_view;
    Canvas *_scene;
    QListWidget *_list;
    QListWidget *_list_class;
    QCheckBox *_checkbox;
    bool _is_init;
    cv::Mat _image;
};

#endif // CANVASDLG_H
