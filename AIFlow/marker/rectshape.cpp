#include "rectshape.h"
#include "shapeutil.h"

RectShape::RectShape(const QRectF& rect, const QString& label)
    : QGraphicsRectItem(rect), m_label(label)
{
    setupStyle(this);
    setData(DATA_is_temp, false);
    setupLabel(this);
    if (!label.isEmpty())
    {
        updateLabelText(label,data(DATA_class_id).toInt());
        updateLabelPosition(this);
    }

    m_ltHandle = new HandleItem(this,1);
    m_rtHandle = new HandleItem(this,2);
    m_lbHandle = new HandleItem(this,2);
    m_rbHandle = new HandleItem(this,1);
    update_handles_pos();
}

void RectShape::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    applyHoverEnter(this);
    m_hovered = true;
    _update_handle_visibility();
    updateLabelVisibility(isSelected(), m_hovered);
    QGraphicsRectItem::hoverEnterEvent(event);
}

void RectShape::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    applyHoverLeave(this);
    m_hovered = false;
    _update_handle_visibility();
    updateLabelVisibility(isSelected(), m_hovered);
    QGraphicsRectItem::hoverLeaveEvent(event);
}

void RectShape::_update_handle_visibility()
{
    bool vis = isSelected() || m_hovered;
    m_ltHandle->setVisible(vis);
    m_rtHandle->setVisible(vis);
    m_lbHandle->setVisible(vis);
    m_rbHandle->setVisible(vis);
}

void RectShape::update_handles_pos()
{
    m_updatingHandles = true;
    QRectF r = rect();
    m_ltHandle->setPos(r.topLeft());
    m_rtHandle->setPos(r.topRight());
    m_lbHandle->setPos(r.bottomLeft());
    m_rbHandle->setPos(r.bottomRight());
    m_updatingHandles = false;
}

void RectShape::update_from_handle(QGraphicsItem* draggedHandle)
{
    if (m_updatingHandles) return;
    m_updatingHandles = true;

    HandleItem* h = static_cast<HandleItem*>(draggedHandle);
    QPointF hp = h->pos();

    if (h == m_ltHandle)
    {
        m_rtHandle->setPos(m_rtHandle->pos().x(), hp.y());
        m_lbHandle->setPos(hp.x(), m_lbHandle->pos().y());
    }
    else if (h == m_rtHandle)
    {
        m_ltHandle->setPos(m_ltHandle->pos().x(), hp.y());
        m_rbHandle->setPos(hp.x(), m_rbHandle->pos().y());
    }
    else if (h == m_lbHandle)
    {
        m_rbHandle->setPos(m_rbHandle->pos().x(), hp.y());
        m_ltHandle->setPos(hp.x(), m_ltHandle->pos().y());
    }
    else if (h == m_rbHandle)
    {
        m_lbHandle->setPos(m_lbHandle->pos().x(), hp.y());
        m_rtHandle->setPos(hp.x(), m_rtHandle->pos().y());
    }

    qreal minX = std::min(m_ltHandle->pos().x(), m_rbHandle->pos().x());
    qreal maxX = std::max(m_ltHandle->pos().x(), m_rbHandle->pos().x());
    qreal minY = std::min(m_ltHandle->pos().y(), m_rbHandle->pos().y());
    qreal maxY = std::max(m_ltHandle->pos().y(), m_rbHandle->pos().y());
    setRect(QRectF(minX, minY, maxX - minX, maxY - minY));
    updateLabelPosition(this);
    m_updatingHandles = false;
}

QVariant RectShape::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemPositionChange && !data(DATA_is_temp).toBool())
    {
        QPointF clamped = clampItemPosition(this, value.toPointF());
        return QGraphicsRectItem::itemChange(change, clamped);
    }
    if (change == ItemSelectedHasChanged)
    {
        _update_handle_visibility();
        updateLabelVisibility(isSelected(), m_hovered);
    }
    else if (change == ItemPositionHasChanged)
    {
        updateLabelPosition(this);
    }
    return QGraphicsRectItem::itemChange(change, value);
}
