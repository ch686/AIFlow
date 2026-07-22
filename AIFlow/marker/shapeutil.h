#ifndef SHAPEUTIL_H
#define SHAPEUTIL_H

#include <QPointF>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QRectF>
#include <cmath>
#include <algorithm>


struct DistResult
{
    qreal dist;
    QPointF projPt;
};

// 点到线段距离
inline DistResult pointToSegmentDist(const QPointF& p, const QPointF& a, const QPointF& b)
{
    DistResult res;
    qreal px = p.x();
    qreal py = p.y();
    qreal ax = a.x();
    qreal ay = a.y();
    qreal bx = b.x();
    qreal by = b.y();

    qreal abDx = bx - ax;
    qreal abDy = by - ay;
    qreal abSq = abDx * abDx + abDy * abDy;

    if (abSq < 1e-9)
    {
        res.dist = std::hypot(px - ax, py - ay);
        res.projPt = a;
        return res;
    }

    qreal apDx = px - ax;
    qreal apDy = py - ay;
    qreal t = (apDx * abDx + apDy * abDy) / abSq;
    t = std::clamp(t, 0.0, 1.0);

    qreal projX = ax + t * abDx;
    qreal projY = ay + t * abDy;
    res.projPt = QPointF(projX, projY);
    res.dist = std::hypot(px - projX, py - projY);
    return res;
}

// 限制图形移动不超出画布
inline QPointF clampItemPosition(QGraphicsItem* item, const QPointF& proposedPos)
{
    QGraphicsScene* scene = item->scene();
    if (!scene)
        return proposedPos;
    QRectF sceneRect = scene->sceneRect();
    QRectF shapeRect = item->boundingRect();

    qreal minX = sceneRect.left() - shapeRect.left();
    qreal maxX = sceneRect.right() - shapeRect.right();
    qreal minY = sceneRect.top() - shapeRect.top();
    qreal maxY = sceneRect.bottom() - shapeRect.bottom();

    qreal nx = proposedPos.x();
    qreal ny = proposedPos.y();
    if (minX <= maxX)
        nx = std::clamp(nx, minX, maxX);
    if (minY <= maxY)
        ny = std::clamp(ny, minY, maxY);
    return QPointF(nx, ny);
}

#endif // SHAPEUTIL_H
