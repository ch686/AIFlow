#include "canvas.h"
#include "rectshape.h"
#include "polyshape.h"
#include "pointshape.h"
#include "rotatedrectshape.h"
#include "handleitem.h"
#include <QGraphicsSceneMouseEvent>
#include <QtMath>
#include <QDebug>
#include <QImageReader>

Canvas::Canvas(QObject *parent)
    : QGraphicsScene(parent),
      m_mode(CanvasMode::RECT),
      m_samMode(CanvasMode::POINT),
      m_imgItem(nullptr),
//      m_samClient(nullptr),
//      m_samEnabled(false),
      m_drawing(false),
      m_startPt(),
      m_tempItem(nullptr)
 //     m_samHoverItem(nullptr)
{
    // 十字线画笔
    QPen crosshairPen(QColor(255, 255, 255, 200), 1, Qt::DashLine);
    m_hLine.setPen(crosshairPen);
    m_vLine.setPen(crosshairPen);
    m_hLine.setZValue(9999);
    m_vLine.setZValue(9999);
    m_hLine.hide();
    m_vLine.hide();

    addItem(&m_hLine);
    addItem(&m_vLine);
}

QString Canvas::getModeName(CanvasMode mode)
{
    switch (mode)
    {
    case CanvasMode::RECT: return "矩形";
    case CanvasMode::POLY: return "多边形";
    case CanvasMode::POINT: return "点";
    case CanvasMode::RBOX: return "旋转框";
    default: return "未知";
    }
}

static QString getRealImageFormat(const QString& filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly))
        return "";

    QByteArray head = f.read(4);
    f.close();

    // JPG: FF D8
    if (head.size() >= 2 && head[0] == (char)0xFF && head[1] == (char)0xD8)
        return "jpg";

    // PNG: 89 50 4E 47
    if (head.size() >= 4 && head[0] == (char)0x89 && head[1] == 'P' && head[2] == 'N' && head[3] == 'G')
        return "png";

    // BMP: 前两字节 "BM"
    if (head.size() >= 2 && head[0] == 'B' && head[1] == 'M')
        return "bmp";

    return "";
}



void Canvas::loadImage(const QString &path)
{
    clearShapes();
    QString fmt=getRealImageFormat(path);
    QImageReader reader(path);
    reader.setFormat(fmt.toLatin1()); // 强制按jpg解析内部数据
    QImage img = reader.read();
    _img_w=img.width();
    _img_h=img.height();

    QPixmap pixmap = QPixmap::fromImage(img);
    if (pixmap.isNull())
    {
        qDebug() << "加载失败路径：" << path;
        // 校验是否支持PNG格式
        QList<QByteArray> supportFormats = QImageReader::supportedImageFormats();
        qDebug() << "当前支持的图片格式：" << supportFormats;
        return;
    }
    if (m_imgItem)
        removeItem(m_imgItem);

    m_imgItem = new QGraphicsPixmapItem(pixmap);
    addItem(m_imgItem);
    setSceneRect(pixmap.rect());
    m_hLine.show();
    m_vLine.show();
}

void Canvas::clearShapes()
{
    const QList<QGraphicsItem*> allItems = items();
    for (QGraphicsItem* item : allItems)
    {
        if (dynamic_cast<RectShape*>(item)
                || dynamic_cast<PolyShape*>(item)
                || dynamic_cast<PointShape*>(item)
                || dynamic_cast<RotatedRectShape*>(item))
        {
            removeItem(item);
        }
    }
}

void Canvas::setMode(CanvasMode mode)
{
    m_mode = mode;
    cancelDrawing();

    const QList<QGraphicsItem*> selItems = selectedItems();
    for (QGraphicsItem* it : selItems)
        it->setSelected(false);
}

void Canvas::setSamEnabled(bool enabled)
{
//    m_samEnabled = enabled;
//    if (!enabled && m_samHoverItem)
//    {
//        removeItem(m_samHoverItem);
//        m_samHoverItem = nullptr;
//    }
}

bool Canvas::isInsideImage(const QPointF &pt) const
{
    if (!m_imgItem) return false;
    return sceneRect().contains(pt);
}

QPointF Canvas::clampPoint(const QPointF &pt) const
{
    QRectF rect = sceneRect();
    qreal x = qMax(rect.left(), qMin(pt.x(), rect.right()));
    qreal y = qMax(rect.top(), qMin(pt.y(), rect.bottom()));
    return QPointF(x, y);
}

void Canvas::updateCrosshair(const QPointF &pt)
{
    if (!m_imgItem) return;
    QRectF rect = sceneRect();
    qreal x = qMax(rect.left(), qMin(pt.x(), rect.right()));
    qreal y = qMax(rect.top(), qMin(pt.y(), rect.bottom()));

    m_hLine.setLine(rect.left(), y, rect.right(), y);
    m_vLine.setLine(x, rect.top(), x, rect.bottom());
    emit mouseMoved(static_cast<int>(x), static_cast<int>(y));
}

void Canvas::cancelDrawing()
{
    m_drawing = false;
    m_polyPts.clear();

    if (m_tempItem)
    {
        removeItem(m_tempItem);
        m_tempItem = nullptr;
    }
//    if (m_samHoverItem)
//    {
//        removeItem(m_samHoverItem);
//        m_samHoverItem = nullptr;
//    }
}

void Canvas::updateTempPoly(const QPointF &mousePos)
{
    QPolygonF displayPts = m_polyPts;
    if (!mousePos.isNull())
        displayPts.append(mousePos);

    if (displayPts.size() < 2)
    {
        if (m_tempItem)
        {
            removeItem(m_tempItem);
            m_tempItem = nullptr;
        }
        return;
    }

    QPolygonF poly=displayPts;
//    for (const QPointF& p : displayPts)
//    {
//        poly << p;
//    }
    PolyShape* polyItem = dynamic_cast<PolyShape*>(m_tempItem);
    if (polyItem)
    {
        polyItem->setPolygon(poly);
    }
    else
    {
        if (m_tempItem)
            removeItem(m_tempItem);
        m_tempItem = new PolyShape(poly,"", true);
        addItem(m_tempItem);
    }
}

void Canvas::finishPolyShape()
{
    PolyShape* shape = new PolyShape(QPolygonF(m_polyPts));
    m_polyPts.clear();

    if (m_tempItem)
    {
        removeItem(m_tempItem);
        m_tempItem = nullptr;
    }
    emit shapeDrawn(shape);
}

//void Canvas::handleSamResult(const QPolygonF &polyPts, const QRectF &rectXYWH,
//                             const QVariantList &rectOBB, float score, bool isClick)
//{
//    Q_UNUSED(score);
//    if (!m_samEnabled || !(m_mode == CanvasMode::RECT || m_mode == CanvasMode::POLY || m_mode == CanvasMode::RBOX))
//        return;

//    if (m_samHoverItem)
//    {
//        removeItem(m_samHoverItem);
//        m_samHoverItem = nullptr;
//    }

//    if (polyPts.isEmpty() && rectXYWH.isEmpty())
//        return;

//    // 矩形SAM
//    if (m_mode == CanvasMode::RECT)
//    {
//        if (isClick)
//        {
//            RectShape* shape = new RectShape(rectXYWH);
//            emit shapeDrawn(shape);
//        }
//        else
//        {
//            QGraphicsRectItem* rectItem = new QGraphicsRectItem(rectXYWH);
//            rectItem->setPen(QPen(QColor(0,255,0),2,Qt::DashLine));
//            rectItem->setBrush(QBrush(QColor(0,255,0,50)));
//            m_samHoverItem = rectItem;
//            addItem(m_samHoverItem);
//        }
//    }
//    // 多边形SAM
//    else if (m_mode == CanvasMode::POLY)
//    {
//        QPolygonF qPoly(polyPts);
//        if (isClick)
//        {
//            PolyShape* shape = new PolyShape(qPoly);
//            emit shapeDrawn(shape);
//        }
//        else
//        {
//            PolyShape* polyItem = new PolyShape(qPoly,"", true);
//            polyItem->setPen(QPen(QColor(0,255,0),2,Qt::DashLine));
//            polyItem->setBrush(QBrush(QColor(0,255,0,50)));
//            m_samHoverItem = polyItem;
//            addItem(m_samHoverItem);
//        }
//    }
//    // 旋转框SAM OBB
//    else if (m_mode == CanvasMode::RBOX)
//    {
//        if (rectOBB.size() <5) return;
//        qreal cx = rectOBB[0].toDouble();
//        qreal cy = rectOBB[1].toDouble();
//        qreal w = rectOBB[2].toDouble();
//        qreal h = rectOBB[3].toDouble();
//        qreal angle = rectOBB[4].toDouble();

//        if (isClick)
//        {
//            RotatedRectShape* shape = new RotatedRectShape(cx, cy, w, h, angle);
//            emit shapeDrawn(shape);
//        }
//        else
//        {
//            RotatedRectShape* rboxItem = new RotatedRectShape(cx, cy, w, h, angle,"", true);
//            m_samHoverItem = rboxItem;
//            addItem(m_samHoverItem);
//        }
//    }
//}

void Canvas::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QPointF pt = event->scenePos();
    updateCrosshair(pt);
    QGraphicsScene::mouseMoveEvent(event);
    QPointF clampedPt = clampPoint(pt);

    // SAM悬停推理
//    if (m_samEnabled && isInsideImage(pt)
//            && m_samMode == CanvasMode::POINT
//            && (m_mode == CanvasMode::RECT || m_mode == CanvasMode::POLY || m_mode == CanvasMode::RBOX))
//    {
//        if (m_samClient)
//        {
//            QList<QPointF> pts;
//            pts << clampedPt;
//            m_samClient->requestInference(pts, "point", false);
//        }
//        return;
//    }
//    else if (m_samHoverItem)
//    {
//        removeItem(m_samHoverItem);
//        m_samHoverItem = nullptr;
//    }

    // 矩形/旋转框临时绘制
    if (m_drawing && !m_startPt.isNull())
    {
        QRectF rect(
            qMin(m_startPt.x(), clampedPt.x()),
            qMin(m_startPt.y(), clampedPt.y()),
            qAbs(clampedPt.x() - m_startPt.x()),
            qAbs(clampedPt.y() - m_startPt.y())
        );
        if (m_tempItem)
            removeItem(m_tempItem);

        if (m_mode == CanvasMode::RECT)// || (m_samEnabled && m_samMode == CanvasMode::RECT))
        {
            QGraphicsRectItem* rectItem = new QGraphicsRectItem(rect);
            rectItem->setPen(QPen(QColor(28,126,214),2,Qt::DashLine));
            rectItem->setData(DATA_is_temp, true);
            m_tempItem = rectItem;
            addItem(m_tempItem);
        }
        else if (m_mode == CanvasMode::RBOX)
        {
            qreal cx = rect.center().x();
            qreal cy = rect.center().y();
            qreal w = qMax(1.0, rect.width());
            qreal h = qMax(1.0, rect.height());
            RotatedRectShape* rbox = new RotatedRectShape(cx, cy, w, h, 0, "",true);
            m_tempItem = rbox;
            addItem(m_tempItem);
        }
    }
    // 多边形拖拽预览
    else if (m_mode == CanvasMode::POLY  && !m_polyPts.isEmpty())//&& !m_samEnabled
    {
        updateTempPoly(clampedPt);
    }
}

void Canvas::mousePressEvent(QGraphicsSceneMouseEvent *event)
{


    QPointF pt = event->scenePos();
    QPointF clampedPt = clampPoint(pt);

//    // SAM左键点选推理
//    if (m_samEnabled && event->button() == Qt::LeftButton
//            && m_samMode == CanvasMode::POINT
//            && (m_mode == CanvasMode::RECT || m_mode == CanvasMode::POLY || m_mode == CanvasMode::RBOX))
//    {
//        if (isInsideImage(pt) && m_samClient)
//        {
//            m_samClient->requestInference(clampedPt.x(), clampedPt.y(), true);
//        }
//        return;
//    }

    // 拾取选中图形/控制点
    QList<QGraphicsItem*> hitItems = items(clampedPt);
    QGraphicsItem* clickedItem = nullptr;
    for (QGraphicsItem* it : hitItems)
    {
        HandleItem* handle = dynamic_cast<HandleItem*>(it);
        if (handle && handle->isVisible())
        {
            bool isTemp = handle->parentItem()->data(DATA_is_temp).toBool();
            if (!isTemp)
            {
                clickedItem = handle;
                break;
            }
        }
    }
    if (!clickedItem)
    {
        for (QGraphicsItem* it : hitItems)
        {
            if (dynamic_cast<RectShape*>(it) || dynamic_cast<PolyShape*>(it)
                    || dynamic_cast<PointShape*>(it) || dynamic_cast<RotatedRectShape*>(it))
            {
                bool isTemp = it->data(DATA_is_temp).toBool();
                if (!isTemp)
                {
                    clickedItem = it;
                    break;
                }
            }
        }
    }

    // 选中已有图形，进入编辑模式
    if (clickedItem)// && !m_samEnabled)
    {
        QGraphicsScene::mousePressEvent(event);
        if (event->button() == Qt::RightButton)
        {
            m_startPt = clampedPt;
        }
        else if (event->button() == Qt::LeftButton)
        {
            if (!(event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier)))
            {
                const QList<QGraphicsItem*> sel = selectedItems();
                for (QGraphicsItem* s : sel)
                {
                    if (s != clickedItem && s != clickedItem->parentItem())
                        s->setSelected(false);
                }
            }
            if (dynamic_cast<HandleItem*>(clickedItem))
                clickedItem->parentItem()->setSelected(true);
            else
                clickedItem->setSelected(true);
        }
        return;
    }

    // 绘图逻辑
    if (!isInsideImage(pt) && !m_drawing)
        return;

    if (event->button() == Qt::LeftButton)
    {
        if ((m_mode == CanvasMode::RECT || m_mode == CanvasMode::RBOX)
                || ((m_mode == CanvasMode::RECT || m_mode == CanvasMode::RBOX || m_mode == CanvasMode::POLY)
                    && m_samMode == CanvasMode::RECT))// && m_samEnabled))
        {
            m_drawing = true;
            m_startPt = clampedPt;
        }
        else if (m_mode == CanvasMode::POLY)
        {
            // 闭合判断
            if (m_polyPts.size() > 2)
            {
                QPointF first = m_polyPts[0];
                qreal dist = qSqrt(qPow(clampedPt.x()-first.x(),2) + qPow(clampedPt.y()-first.y(),2));
                if (dist < 10)
                {
                    finishPolyShape();
                    return;
                }
            }
            m_polyPts.append(clampedPt);
            updateTempPoly();
        }
        else if (m_mode == CanvasMode::POINT)
        {
            PointShape* shape = new PointShape(clampedPt);
            emit shapeDrawn(shape);
        }
    }
    else if (event->button() == Qt::RightButton)
    {
        if (m_mode == CanvasMode::POLY && m_polyPts.size() > 2)
            finishPolyShape();
    }
}

void Canvas::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsScene::mouseReleaseEvent(event);
  //  if (m_samEnabled && m_samMode != CanvasMode::RECT)
   //     return;
    if (event->button() == Qt::RightButton)
    {
        QPointF pt = event->scenePos();
        QPointF clampedPt = clampPoint(pt);
        qreal dx = clampedPt.x() - m_startPt.x();
        qreal dy = clampedPt.y() - m_startPt.y();
        qreal dist=std::sqrt(dx*dx + dy*dy);
        if(dist<10)
        {
            const QList<QGraphicsItem*> sel = selectedItems();
            for (QGraphicsItem* it : sel)
            {
                if(((RectShape*)it)->rect().contains(pt))
                    removeItem(it);
            }
        }
    }
    else if (event->button() == Qt::LeftButton && m_drawing)
    {
        m_drawing = false;
        if (m_tempItem)
        {
            QPointF pt = clampPoint(event->scenePos());
            QRectF rect(
                qMin(m_startPt.x(), pt.x()),
                qMin(m_startPt.y(), pt.y()),
                qAbs(pt.x() - m_startPt.x()),
                qAbs(pt.y() - m_startPt.y())
            );
            removeItem(m_tempItem);
            m_tempItem = nullptr;

            if (rect.width() > 5 && rect.height() >5)
            {
                // SAM框选推理
//                if (m_samEnabled && m_samMode == CanvasMode::RECT && m_samClient)
//                {
//                    QList<QPointF> boxPts;
//                    boxPts << QPointF(rect.left(), rect.top());
//                    boxPts << QPointF(rect.right(), rect.bottom());
//                    m_samClient->requestInference(boxPts, "rect", false);
//                }
//                else
                if (m_mode == CanvasMode::RECT)
                {
                    RectShape* shape = new RectShape(rect);
                    emit shapeDrawn(shape);
                }
                else if (m_mode == CanvasMode::RBOX)
                {
                    qreal cx = rect.center().x();
                    qreal cy = rect.center().y();
                    RotatedRectShape* shape = new RotatedRectShape(cx, cy, rect.width(), rect.height(), 0);
                    emit shapeDrawn(shape);
                }
            }
        }
        emit stateChanged();
    }
}

void Canvas::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    QPointF pt = event->scenePos();
    if (!isInsideImage(pt)) return;

    QList<QGraphicsItem*> hitItems = items(pt);
    for (QGraphicsItem* it : hitItems)
    {
        bool isTemp = it->data(DATA_is_temp).toBool();
        if (isTemp) continue;

        if (dynamic_cast<HandleItem*>(it) || dynamic_cast<RectShape*>(it)
                || dynamic_cast<PolyShape*>(it) || dynamic_cast<PointShape*>(it)
                || dynamic_cast<RotatedRectShape*>(it))
        {
            QGraphicsItem* target = dynamic_cast<HandleItem*>(it) ? it->parentItem() : it;
            emit shapeDoubleClicked(target);
            return;
        }
    }

    if (event->button() == Qt::LeftButton && m_mode == CanvasMode::POLY  && m_polyPts.size()>2)//&& !m_samEnabled
    {
        finishPolyShape();
    }
    else
    {
        QGraphicsScene::mouseDoubleClickEvent(event);
    }
}

void Canvas::keyPressEvent(QKeyEvent *event)
{
    int key = event->key();
    Qt::KeyboardModifiers mod = event->modifiers();

    if (key == Qt::Key_Backspace || key == Qt::Key_Delete)
    {
        const QList<QGraphicsItem*> sel = selectedItems();
        for (QGraphicsItem* it : sel)
            removeItem(it);
        emit stateChanged();
    }
    else if (key == Qt::Key_Z && mod == Qt::ControlModifier)
    {
        if (m_mode == CanvasMode::POLY  && !m_polyPts.isEmpty())//&& !m_samEnabled
        {
            m_polyPts.pop_back();
            updateTempPoly();
        }
    }
    else if (key == Qt::Key_Return || key == Qt::Key_Enter)
    {
        if (m_mode == CanvasMode::POLY && m_polyPts.size()>2)
            finishPolyShape();
    }
    else if (key == Qt::Key_Escape)
    {
        cancelDrawing();
    }
    else if (key == Qt::Key_Z || key == Qt::Key_X || key == Qt::Key_C || key == Qt::Key_V)
    {
        const QList<QGraphicsItem*> sel = selectedItems();
        if (!sel.isEmpty())
        {
            if(sel[0]->type()==QGraphicsObject::Type)
            {
                RotatedRectShape* rbox = dynamic_cast<RotatedRectShape*>(sel[0]);
                if (rbox)
                {
                    qreal delta = 0;
                    if (key == Qt::Key_Z) delta = -5;
                    else if (key == Qt::Key_X) delta = -1;
                    else if (key == Qt::Key_C) delta = 1;
                    else if (key == Qt::Key_V) delta =5;

                    if (delta != 0)
                    {
                        rbox->setRotation(rbox->rotation() + delta);
                        emit stateChanged();
                    }
                }
            }
        }
    }

    QGraphicsScene::keyPressEvent(event);
}
