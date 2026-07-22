#ifndef POLYSHAPE_H
#define POLYSHAPE_H

#include <QGraphicsPolygonItem>
#include "baseshape.h"
#include "handleitem.h"
#include <QList>

class PolyShape : public QGraphicsPolygonItem, public BaseShape
{
public:
    explicit PolyShape(const QPolygonF& polygon, const QString& label = "", bool isTemp = false);
    void setPolygon(const QPolygonF& polygon);

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

public :
    void _update_handle_visibility();
    void update_handles();
    void update_from_handles();
    void remove_handle(QGraphicsItem* handle);

private:
    QString m_label;
    bool m_isTemp;
    QList<HandleItem*> m_handles;
    bool m_updatingHandles = false;
    bool m_hovered = false;
    int m_draggingEdgeIdx = -1;
    int m_ghostIdx = -1;
    QPointF m_ghostPos;
    QGraphicsEllipseItem* m_ghostHandle;
};

#endif // POLYSHAPE_H
