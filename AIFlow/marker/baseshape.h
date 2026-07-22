#ifndef BASESHAPE_H
#define BASESHAPE_H

#include <QPen>
#include <QBrush>
#include <QGraphicsTextItem>
#include <QFont>
#include <QGraphicsItem>
#include <QColor>
#include <QCursor>
#include <QGraphicsRectItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsEllipseItem>
#define DATA_hovered (Qt::UserRole+1)
#define DATA_updating_handles (Qt::UserRole+2)
#define DATA_is_temp (Qt::UserRole+3)
#define DATA_class_id (Qt::UserRole+4)

class BaseShape
{
public:
    QPen normalPen;
    QBrush normalBrush;
    QBrush hoverBrush;
    QGraphicsTextItem* labelText = nullptr;


    template<class T>
    void setupStyle(T* item,QColor pen_clr=QColor(28, 126, 214),
                            QColor nb_clr=QColor(28, 126, 214, 50),
                            QColor hb_clr=QColor(28, 126, 214, 120))
    {
        normalPen = QPen(pen_clr, 2);
        normalBrush = QBrush(nb_clr);
        hoverBrush = QBrush(hb_clr);

        item->setPen(normalPen);
        item->setBrush(normalBrush);
        item->setFlags(QGraphicsItem::ItemIsSelectable
                       | QGraphicsItem::ItemIsMovable
                       | QGraphicsItem::ItemSendsGeometryChanges);
        item->setAcceptHoverEvents(true);
    }

    template<class T>
    void applyHoverEnter(T* item)
    {
        bool isTemp = item->data(DATA_is_temp).toBool();
        if (isTemp) return;
        item->setBrush(hoverBrush);
        item->setCursor(Qt::OpenHandCursor);
    }

    template<class T>
    void applyHoverLeave(T* item)
    {
        bool isTemp = item->data(DATA_is_temp).toBool();
        if (isTemp) return;
        item->setBrush(normalBrush);
        item->setCursor(Qt::ArrowCursor);
    }

    void setupLabel(QGraphicsItem* parentItem);
    void updateLabelPosition(QGraphicsItem* item);
    void updateLabelText(const QString& text,int class_id);
    int  getLabelText(QString& text);
    void updateLabelVisibility(bool isSelected, bool isHovered);
};

#endif // BASESHAPE_H
