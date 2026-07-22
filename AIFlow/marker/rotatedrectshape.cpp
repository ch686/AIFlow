#include "rotatedrectshape.h"

#include "shapeutil.h"
#include <QtMath>

RotatedRectShape::RotatedRectShape(qreal cx, qreal cy, qreal w, qreal h, qreal angle,
                                   const QString& label, bool isTemp)
    : QGraphicsObject(), m_isTemp(isTemp), m_label(label), m_boxW(w), m_boxH(h)
{
    setPos(cx, cy);
    setRotation(angle);
    setFlags(QGraphicsItem::ItemIsSelectable
             | QGraphicsItem::ItemIsMovable
             | QGraphicsItem::ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
    setProperty("is_temp", isTemp);

    m_rectItem = new QGraphicsRectItem(this);
    m_rotateLine = new QGraphicsLineItem(this);
    m_rotateLine->setPen(QPen(QColor(28, 126, 214), 1.5, Qt::DashLine));

    m_hTop = new OBBHandle(OBBHandleType::Top, this);
    m_hBottom = new OBBHandle(OBBHandleType::Bottom, this);
    m_hLeft = new OBBHandle(OBBHandleType::Left, this);
    m_hRight = new OBBHandle(OBBHandleType::Right, this);
    m_hRotate = new OBBHandle(OBBHandleType::Rotate, this);

    m_handles << m_hTop << m_hBottom << m_hLeft << m_hRight << m_hRotate << m_rotateLine;
    updateGeometry();
    _update_handle_visibility();

    if (!isTemp)
    {
        setupLabel(this);
        if (!label.isEmpty()) updateLabelText(label,data(DATA_class_id).toInt());
    }
    if (isTemp)
        for (auto h : m_handles) h->hide();
}

QRectF RotatedRectShape::boundingRect() const
{
    qreal r = m_boxH / 2.0 + 35;
    return QRectF(-m_boxW / 2.0 - 10, -r, m_boxW + 20, r + m_boxH / 2.0 + 10);
}

void RotatedRectShape::paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*)
{
    // 子item自行绘制，本体无需绘制
}

void RotatedRectShape::updateGeometry()
{
    qreal w = m_boxW;
    qreal h = m_boxH;
    m_rectItem->setRect(-w / 2.0, -h / 2.0, w, h);

    m_hTop->setPos(0, -h / 2.0);
    m_hBottom->setPos(0, h / 2.0);
    m_hLeft->setPos(-w / 2.0, 0);
    m_hRight->setPos(w / 2.0, 0);
    m_hRotate->setPos(0, -h / 2.0 - 30);
    m_rotateLine->setLine(0, -h / 2.0, 0, -h / 2.0 - 30);

    prepareGeometryChange();
    updateLabelPosition(this);
}

void RotatedRectShape::handleDragged(OBBHandleType handleType, const QPointF& scenePos)
{
    m_isResizing = true;
    QPointF localP = mapFromScene(scenePos);
    qreal minSize = 5.0;
    switch (handleType)
    {
    case OBBHandleType::Top:
    {
        qreal dy = localP.y() - (-m_boxH / 2.0);
        if (m_boxH - dy < minSize) dy = m_boxH - minSize;
        m_boxH -= dy;
        QPointF off = mapToScene(QPointF(0, dy / 2.0)) - mapToScene(QPointF(0, 0));
        setPos(pos() + off);
        break;
    }
    case OBBHandleType::Bottom:
    {
        qreal dy = localP.y() - (m_boxH / 2.0);
        if (m_boxH + dy < minSize) dy = -(m_boxH - minSize);
        m_boxH += dy;
        QPointF off = mapToScene(QPointF(0, dy / 2.0)) - mapToScene(QPointF(0, 0));
        setPos(pos() + off);
        break;
    }
    case OBBHandleType::Left:
    {
        qreal dx = localP.x() - (-m_boxW / 2.0);
        if (m_boxW - dx < minSize) dx = m_boxW - minSize;
        m_boxW -= dx;
        QPointF off = mapToScene(QPointF(dx / 2.0, 0)) - mapToScene(QPointF(0, 0));
        setPos(pos() + off);
        break;
    }
    case OBBHandleType::Right:
    {
        qreal dx = localP.x() - (m_boxW / 2.0);
        if (m_boxW + dx < minSize) dx = -(m_boxW - minSize);
        m_boxW += dx;
        QPointF off = mapToScene(QPointF(dx / 2.0, 0)) - mapToScene(QPointF(0, 0));
        setPos(pos() + off);
        break;
    }
    case OBBHandleType::Rotate:
    {
        QPointF centerScene = mapToScene(QPointF(0,0));
        qreal dx = scenePos.x() - centerScene.x();
        qreal dy = scenePos.y() - centerScene.y();
        qreal rad = qAtan2(dy, dx);
        qreal deg = qRadiansToDegrees(rad) + 90;
        setRotation(deg);
        break;
    }
    }
    updateGeometry();
    m_isResizing = false;
}

QPolygonF RotatedRectShape::polygon() const
{
    qreal w = m_boxW;
    qreal h = m_boxH;
    QPolygonF poly;
    poly << mapToScene(QPointF(-w/2, -h/2));
    poly << mapToScene(QPointF(w/2, -h/2));
    poly << mapToScene(QPointF(w/2, h/2));
    poly << mapToScene(QPointF(-w/2, h/2));
    return poly;
}

void RotatedRectShape::hoverEnterEvent(QGraphicsSceneHoverEvent*)
{
    m_hovered = true;
    m_rectItem->setBrush(QBrush(QColor(28,126,214,120)));
    _update_handle_visibility();
}

void RotatedRectShape::hoverLeaveEvent(QGraphicsSceneHoverEvent*)
{
    m_hovered = false;
    m_rectItem->setBrush(QBrush(QColor(28,126,214,50)));
    _update_handle_visibility();
}

void RotatedRectShape::_update_handle_visibility()
{
    if (m_isTemp) return;
    bool vis = isSelected() || m_hovered;
    for (auto h : m_handles) h->setVisible(vis);
}

QVariant RotatedRectShape::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemPositionChange && !property("is_temp").toBool() && !m_isResizing)
    {
        QGraphicsScene* sc = scene();
        if (sc)
        {
            QRectF sr = sc->sceneRect();
            QPointF p = value.toPointF();
            p.setX(std::clamp(p.x(), sr.left(), sr.right()));
            p.setY(std::clamp(p.y(), sr.top(), sr.bottom()));
            return p;
        }
    }
    if (change == ItemSelectedHasChanged)
        _update_handle_visibility();
    else if (change == ItemPositionHasChanged)
        updateLabelPosition(this);
    return QGraphicsObject::itemChange(change, value);
}
