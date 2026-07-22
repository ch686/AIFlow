#ifndef HANDLEITEM_H
#define HANDLEITEM_H

#include <QGraphicsEllipseItem>
#include <QPointF>

class HandleItem : public QGraphicsEllipseItem
{

public:
    explicit HandleItem(QGraphicsItem* parent = nullptr,int type=0);

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    QPointF m_mousePressPos;
    bool m_isMoved = false;
    int _type=0;
};

#endif // HANDLEITEM_H
