#include "obbhandle.h"

#include "rotatedrectshape.h"
#include <QtMath>
#include <QCursor>
#include <QGraphicsSceneMouseEvent>

OBBHandle::OBBHandle(OBBHandleType type, RotatedRectShape* parent)
    : QGraphicsItem(parent), m_type(type), m_parentRBox(parent)
{
    setAcceptHoverEvents(true);
    setZValue(100);
    switch (m_type)
    {
    case OBBHandleType::Top:
    case OBBHandleType::Bottom:
        m_w = 16; m_h = 6; break;
    case OBBHandleType::Left:
    case OBBHandleType::Right:
        m_w = 6; m_h = 16; break;
    case OBBHandleType::Rotate:
        m_w = 10; m_h = 10; break;
    }
}

QRectF OBBHandle::boundingRect() const
{
    return QRectF(-m_w / 2.0, -m_h / 2.0, m_w, m_h);
}

void OBBHandle::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setBrush(Qt::white);
    painter->setPen(QPen(QColor(28, 126, 214), 2));
    QRectF br = boundingRect();
    if (m_type == OBBHandleType::Rotate)
        painter->drawEllipse(br);
    else
    {
        qreal r = qMin(m_w, m_h) / 2.0;
        painter->drawRoundedRect(br, r, r);
    }
}

void OBBHandle::hoverEnterEvent(QGraphicsSceneHoverEvent*)
{
    if (m_type == OBBHandleType::Rotate)
        setCursor(QCursor(Qt::OpenHandCursor));
    else
        setCursor(Qt::CrossCursor);
}

void OBBHandle::hoverLeaveEvent(QGraphicsSceneHoverEvent*)
{
    setCursor(Qt::ArrowCursor);
}

void OBBHandle::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (m_type == OBBHandleType::Rotate)
            setCursor(Qt::ClosedHandCursor);
        m_parentRBox->m_draggingHandle = m_type;
        m_parentRBox->setFlag(QGraphicsItem::ItemIsMovable, false);
        event->accept();
    }
    else
        QGraphicsItem::mousePressEvent(event);
}

void OBBHandle::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (m_parentRBox->m_draggingHandle == m_type)
    {
        m_parentRBox->handleDragged(m_type, event->scenePos());
        event->accept();
    }
    else
        QGraphicsItem::mouseMoveEvent(event);
}

void OBBHandle::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_parentRBox->m_draggingHandle = (OBBHandleType)-1;
        m_parentRBox->setFlag(QGraphicsItem::ItemIsMovable, true);
        if (m_type == OBBHandleType::Rotate)
            setCursor(Qt::OpenHandCursor);
        event->accept();
    }
    else
        QGraphicsItem::mouseMoveEvent(event);
}
