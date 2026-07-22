#include "handleitem.h"
#include "shapeutil.h"
#include <QCursor>
#include <QGraphicsSceneMouseEvent>
#include "rectshape.h"
#include "polyshape.h"
#include "rotatedrectshape.h"

HandleItem::HandleItem(QGraphicsItem* parent,int type)
    : QGraphicsEllipseItem(parent)
{
    qreal r = 3.5;
    setRect(-r, -r, r * 2, r * 2);
    setBrush(QBrush(Qt::white));
    setPen(QPen(QColor(28, 126, 214), 1.5));
    setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
    setZValue(1000);
    _type=type;
    hide();
}

void HandleItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    if(_type==0)
        setCursor(QCursor(Qt::ClosedHandCursor));
    else if(_type==1)
        setCursor(QCursor(Qt::SizeFDiagCursor));
    else if(_type==2)
        setCursor(QCursor(Qt::SizeBDiagCursor));

    QGraphicsItem* p = parentItem();
    if (p->data(DATA_hovered).isValid())
    {
        p->setData(DATA_hovered, true);
        if(p->type()==QGraphicsRectItem::Type)
            ((RectShape*)p)->_update_handle_visibility();
        if(p->type()==QGraphicsPolygonItem::Type)
            ((PolyShape*)p)->_update_handle_visibility();
        if(p->type()==QGraphicsObject::Type)
            ((RotatedRectShape*)p)->_update_handle_visibility();
        //metaObject()->invokeMethod(p, "_update_handle_visibility");
    }
    QGraphicsEllipseItem::hoverEnterEvent(event);
}


void HandleItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    setCursor(Qt::ArrowCursor);
    QGraphicsEllipseItem::hoverLeaveEvent(event);
}

void HandleItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    m_mousePressPos = event->pos();
    m_isMoved = false;
    QGraphicsItem* p = parentItem();
    if (p)
    {
        p->setSelected(true);
        p->setFlag(QGraphicsItem::ItemIsMovable, false);
    }
    QGraphicsEllipseItem::mousePressEvent(event);
}

void HandleItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (!m_mousePressPos.isNull() && (event->pos() - m_mousePressPos).manhattanLength() > 2)
        m_isMoved = true;
    QGraphicsEllipseItem::mouseMoveEvent(event);
}

void HandleItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsEllipseItem::mouseReleaseEvent(event);
    QGraphicsItem* p = parentItem();
    if (p)
        p->setFlag(QGraphicsItem::ItemIsMovable, true);

    if (!m_isMoved && event->button() == Qt::LeftButton)
    {
       // if (p->metaObject()->indexOfMethod("remove_handle(QGraphicsItem*)") != -1)
       //     metaObject()->invokeMethod(p, "remove_handle", Q_ARG(QGraphicsItem*, this));
        if(p->type()==QGraphicsPolygonItem::Type)
            ((PolyShape*)p)->remove_handle(this);
    }
}

QVariant HandleItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemPositionChange && scene() && parentItem())
    {
        QGraphicsItem* p = parentItem();
        QPointF localNewPos = value.toPointF();
        QPointF scenePos = p->mapToScene(localNewPos);
        QRectF sceneRect = scene()->sceneRect();
        QPointF clamped(
            std::clamp(scenePos.x(), sceneRect.left(), sceneRect.right()),
            std::clamp(scenePos.y(), sceneRect.top(), sceneRect.bottom())
        );
        QPointF localClamped = p->mapFromScene(clamped);
        return QGraphicsEllipseItem::itemChange(change, localClamped);
    }
    if (change == ItemPositionHasChanged && scene())
    {
        QGraphicsItem* p = parentItem();
        bool updating = p->data(DATA_updating_handles).toBool();
        if (!updating)
        {
//            if (p->metaObject()->indexOfMethod("update_from_handle(QGraphicsItem*)") != -1)
//                metaObject()->invokeMethod(p, "update_from_handle", Q_ARG(QGraphicsItem*, this));
//            else if (p->metaObject()->indexOfMethod("update_from_handles()") != -1)
//                metaObject()->invokeMethod(p, "update_from_handles");
            if(p->type()==QGraphicsRectItem::Type)
                ((RectShape*)p)->update_from_handle(this);
            else if(p->type()==QGraphicsPolygonItem::Type)
                ((PolyShape*)p)->update_from_handles();

        }
    }
    return QGraphicsEllipseItem::itemChange(change, value);
}
