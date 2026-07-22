#ifndef OBBHANDLE_H
#define OBBHANDLE_H

#include <QGraphicsItem>
#include <QPainter>

enum class OBBHandleType
{
    Top, Bottom, Left, Right, Rotate
};

class RotatedRectShape;
class OBBHandle : public QGraphicsItem
{
public:
    explicit OBBHandle(OBBHandleType type, RotatedRectShape* parent);
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    OBBHandleType m_type;
    qreal m_w = 0, m_h = 0;
    RotatedRectShape* m_parentRBox;
};

#endif // OBBHANDLE_H
