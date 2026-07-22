#include "polyshape.h"

#include "shapeutil.h"
#include <QPolygonF>
#include <QGraphicsSceneHoverEvent>
#include <QCursor>

PolyShape::PolyShape(const QPolygonF& polygon, const QString& label, bool isTemp)
    : QGraphicsPolygonItem(polygon), m_label(label), m_isTemp(isTemp)
{
    m_ghostHandle = new QGraphicsEllipseItem(-3.5, -3.5, 7, 7, this);
    m_ghostHandle->setBrush(QBrush(QColor(255, 255, 255, 180)));
    m_ghostHandle->setPen(QPen(QColor(28, 126, 214, 150), 1.5));
    m_ghostHandle->setZValue(999);
    m_ghostHandle->setAcceptedMouseButtons(Qt::NoButton);
    m_ghostHandle->hide();

    setData(DATA_is_temp, m_isTemp);
    if (m_isTemp)
    {
        setPen(QPen(QColor(28, 126, 214), 2, Qt::DashLine));
        setBrush(QBrush(QColor(28, 126, 214, 50)));
    }
    else
    {
        setupStyle(this);
        setupLabel(this);
        if (!label.isEmpty())
        {
            updateLabelText(label,data(DATA_class_id).toInt());
            updateLabelPosition(this);
        }
    }
    update_handles();
}

void PolyShape::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    if (!m_isTemp) applyHoverEnter(this);
    m_hovered = true;
    _update_handle_visibility();
    if (!m_isTemp) updateLabelVisibility(isSelected(), m_hovered);
    QGraphicsPolygonItem::hoverEnterEvent(event);
}

void PolyShape::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    if (m_isTemp || m_draggingEdgeIdx != -1)
    {
        QGraphicsPolygonItem::hoverMoveEvent(event);
        return;
    }
    QPointF pt = event->pos();
    QPolygonF poly = polygon();
    qreal minDist = 1e9;
    int insertIdx = -1;
    QPointF closestPt;

    for (int i = 0; i < poly.count(); i++)
    {
        QPointF p1 = poly[i];
        QPointF p2 = poly[(i + 1) % poly.count()];
        DistResult dr = pointToSegmentDist(pt, p1, p2);
        if (dr.dist < minDist)
        {
            minDist = dr.dist;
            insertIdx = i + 1;
            closestPt = dr.projPt;
        }
    }
    if (minDist < 8)
    {
        m_ghostIdx = insertIdx;
        m_ghostPos = closestPt;
        m_ghostHandle->setPos(closestPt);
        m_ghostHandle->show();
        setCursor(Qt::CrossCursor);
    }
    else
    {
        m_ghostIdx = -1;
        m_ghostHandle->hide();
        setCursor(Qt::PointingHandCursor);
    }
    QGraphicsPolygonItem::hoverMoveEvent(event);
}

void PolyShape::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    if (!m_isTemp) applyHoverLeave(this);
    m_hovered = false;
    m_ghostIdx = -1;
    m_ghostHandle->hide();

    bool anyHandleHover = false;
    for (auto h : m_handles)
        if (h->isUnderMouse()) anyHandleHover = true;
    if (!anyHandleHover)
        _update_handle_visibility();

    if (!m_isTemp) updateLabelVisibility(isSelected(), m_hovered);
    QGraphicsPolygonItem::hoverLeaveEvent(event);
}

void PolyShape::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_ghostIdx != -1)
    {
        m_draggingEdgeIdx = m_ghostIdx;
        QPolygonF poly = polygon();
        poly.insert(m_ghostIdx, m_ghostPos);
        setPolygon(poly);
        m_ghostHandle->hide();
        m_ghostIdx = -1;
        setFlag(QGraphicsItem::ItemIsMovable, false);
        event->accept();
        return;
    }
    QGraphicsPolygonItem::mousePressEvent(event);
}

void PolyShape::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (m_draggingEdgeIdx != -1)
    {
        QPolygonF poly = polygon();
        QPointF sceneP = mapToScene(event->pos());
        QGraphicsScene* sc = scene();
        QPointF clampedP = sceneP;
        if (sc)
        {
            QRectF sr = sc->sceneRect();
            clampedP.setX(std::clamp(sceneP.x(), sr.left(), sr.right()));
            clampedP.setY(std::clamp(sceneP.y(), sr.top(), sr.bottom()));
        }
        QPointF localClamp = mapFromScene(clampedP);
        poly[m_draggingEdgeIdx] = localClamp;
        setPolygon(poly);
        event->accept();
        return;
    }
    QGraphicsPolygonItem::mouseMoveEvent(event);
}

void PolyShape::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (m_draggingEdgeIdx != -1)
    {
        m_draggingEdgeIdx = -1;
        setFlag(QGraphicsItem::ItemIsMovable, true);
        event->accept();
        return;
    }
    QGraphicsPolygonItem::mouseReleaseEvent(event);
}

void PolyShape::remove_handle(QGraphicsItem* handle)
{
    if (m_handles.size() <= 3) return;
    HandleItem* h = static_cast<HandleItem*>(handle);
    int idx = m_handles.indexOf(h);
    QPolygonF poly = polygon();
    poly.remove(idx);
    setPolygon(poly);
}

void PolyShape::_update_handle_visibility()
{
    if (m_isTemp) return;
    bool vis = isSelected() || m_hovered;
    for (auto h : m_handles)
        h->setVisible(vis);
}

void PolyShape::update_handles()
{
    QPolygonF poly = polygon();
    while (m_handles.size() < poly.count())
    {
        HandleItem* h = new HandleItem(this);
        if (m_isTemp)
        {
            h->setFlag(QGraphicsItem::ItemIsMovable, false);
            h->setFlag(QGraphicsItem::ItemSendsGeometryChanges, false);
            h->hide();
        }
        m_handles.append(h);
    }
    while (m_handles.size() > poly.count())
    {
        HandleItem* h = m_handles.takeLast();
        h->setParentItem(nullptr);
        if (scene()) scene()->removeItem(h);
    }
    m_updatingHandles = true;
    for (int i = 0; i < m_handles.size(); i++)
        m_handles[i]->setPos(poly[i]);
    _update_handle_visibility();
    m_updatingHandles = false;
}

void PolyShape::setPolygon(const QPolygonF& polygon)
{
    QGraphicsPolygonItem::setPolygon(polygon);
    update_handles();
    if (!m_isTemp) updateLabelPosition(this);
}

void PolyShape::update_from_handles()
{
    if (m_isTemp || m_updatingHandles) return;
    QPolygonF poly;
    for (auto h : m_handles)
        poly.append(h->pos());
    QGraphicsPolygonItem::setPolygon(poly);
    updateLabelPosition(this);
}

QVariant PolyShape::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemPositionChange && !data(DATA_is_temp).toBool())
    {
        QPointF clamped = clampItemPosition(this, value.toPointF());
        return QGraphicsPolygonItem::itemChange(change, clamped);
    }
    if (!m_isTemp)
    {
        if (change == ItemSelectedHasChanged)
        {
            _update_handle_visibility();
            updateLabelVisibility(isSelected(), m_hovered);
        }
        else if (change == ItemPositionHasChanged)
        {
            updateLabelPosition(this);
        }
    }
    return QGraphicsPolygonItem::itemChange(change, value);
}
