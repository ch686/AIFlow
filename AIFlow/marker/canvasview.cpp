#include "canvasview.h"
#include <QPainter>
#include <QScrollBar>

CanvasView::CanvasView(QWidget *parent)
    : QGraphicsView(parent)
{
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setMouseTracking(true);
    setAlignment(Qt::AlignCenter);
    setDragMode(QGraphicsView::NoDrag);

    _is_panning = false;
}

void CanvasView::wheelEvent(QWheelEvent *event)
{
//    const qreal zoom_in_factor = 1.15;
//    const qreal zoom_out_factor = 1.0 / zoom_in_factor;
//    qreal zoom_factor;

//    if (event->angleDelta().y() > 0)
//    {
//        zoom_factor = zoom_in_factor;
//    }
//    else
//    {
//        zoom_factor = zoom_out_factor;
//    }
//    scale(zoom_factor, zoom_factor);


    const qreal scaleFactor = 1.1;
    qreal factor = (event->angleDelta().y() > 0) ? scaleFactor : 1 / scaleFactor;
    //scale(factor, factor);
    qreal currentScale = transform().m11();
    if ((factor > 1 && currentScale < 5.0) ||
            (factor < 1 && currentScale > 0.1)) {
        scale(factor, factor);
    }

}

void CanvasView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton)
    {
        _is_panning = true;
        _pan_start_pos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QGraphicsView::mousePressEvent(event);
}

void CanvasView::mouseMoveEvent(QMouseEvent *event)
{
    if (_is_panning)
    {
        QPoint delta = event->pos() - _pan_start_pos;
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        _pan_start_pos = event->pos();
        event->accept();
        return;
    }
    QGraphicsView::mouseMoveEvent(event);
}

void CanvasView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton)
    {
        _is_panning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    QGraphicsView::mouseReleaseEvent(event);
}
