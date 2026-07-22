#ifndef ROTATEDRECTSHAPE_H
#define ROTATEDRECTSHAPE_H

#include <QGraphicsObject>
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include "baseshape.h"
#include "obbhandle.h"
#include <QList>

class RotatedRectShape : public QGraphicsObject, public BaseShape
{
    Q_OBJECT
public:
    explicit RotatedRectShape(qreal cx, qreal cy, qreal w, qreal h, qreal angle,
                              const QString& label = "", bool isTemp = false);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    void updateGeometry();
    void handleDragged(OBBHandleType handleType, const QPointF& scenePos);
    QPolygonF polygon() const;

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

public slots:
    void _update_handle_visibility();

public:
    OBBHandleType m_draggingHandle = (OBBHandleType)-1;
    bool m_isResizing = false;
    bool m_hovered = false;
    bool m_isTemp;
    QString m_label;
    qreal m_boxW;
    qreal m_boxH;

    QGraphicsRectItem* m_rectItem;
    QGraphicsLineItem* m_rotateLine;
    OBBHandle* m_hTop;
    OBBHandle* m_hBottom;
    OBBHandle* m_hLeft;
    OBBHandle* m_hRight;
    OBBHandle* m_hRotate;
    QList<QGraphicsItem*> m_handles;
};

#endif // ROTATEDRECTSHAPE_H
