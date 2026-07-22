#ifndef POINTSHAPE_H
#define POINTSHAPE_H

#include <QGraphicsEllipseItem>
#include "baseshape.h"

class PointShape : public QGraphicsEllipseItem, public BaseShape
{
public:
    explicit PointShape(const QPointF& center, const QString& label = "");

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    QString m_label;
    QPen m_pointPen;
    QBrush m_pointNormalBrush;
    QBrush m_pointHoverBrush;
};

#endif // POINTSHAPE_H
