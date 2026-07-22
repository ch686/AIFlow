#include "pointshape.h"
#include "shapeutil.h"
#include <QCursor>

PointShape::PointShape(const QPointF& center, const QString& label)
    : m_label(label)
{
    qreal r = 4;
    setRect(center.x() - r, center.y() - r, r * 2, r * 2);
    m_pointPen = QPen(QColor(250, 82, 82), 2);
    m_pointNormalBrush = QBrush(QColor(250, 82, 82, 150));
    m_pointHoverBrush = QBrush(QColor(250, 82, 82, 220));
    setPen(m_pointPen);
    setBrush(m_pointNormalBrush);
    setFlags(QGraphicsItem::ItemIsSelectable
             | QGraphicsItem::ItemIsMovable
             | QGraphicsItem::ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
    setData(DATA_is_temp, false);

    setupLabel(this);
    if (!label.isEmpty())
    {
        updateLabelText(label,data(DATA_class_id).toInt());
        updateLabelPosition(this);
    }
}

void PointShape::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    setBrush(m_pointHoverBrush);
    setCursor(Qt::PointingHandCursor);
    updateLabelVisibility(isSelected(), true);
    QGraphicsEllipseItem::hoverEnterEvent(event);
}

void PointShape::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    setBrush(m_pointNormalBrush);
    setCursor(Qt::ArrowCursor);
    updateLabelVisibility(isSelected(), false);
    QGraphicsEllipseItem::hoverLeaveEvent(event);
}

QVariant PointShape::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemPositionChange)
    {
        QPointF clamped = clampItemPosition(this, value.toPointF());
        return QGraphicsEllipseItem::itemChange(change, clamped);
    }
    if (change == ItemSelectedHasChanged)
    {
        updateLabelVisibility(isSelected(), isUnderMouse());
    }
    else if (change == ItemPositionHasChanged)
    {
        updateLabelPosition(this);
    }
    return QGraphicsEllipseItem::itemChange(change, value);
}
