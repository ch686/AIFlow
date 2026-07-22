#ifndef CANVAS_H
#define CANVAS_H

#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsLineItem>
#include <QPen>
#include <QColor>
#include <QPointF>
#include <QRectF>
#include <QPolygonF>
#include <QList>
#include <QKeyEvent>
#include <QMouseEvent>

// 前置声明自定义图形项
class RectShape;
class PolyShape;
class PointShape;
class RotatedRectShape;
class HandleItem;
//class SamClient;

enum class CanvasMode
{
    EDIT = 0,
    RECT = 1,
    POLY = 2,
    POINT = 3,
    RBOX = 4
};

class Canvas : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit Canvas(QObject *parent = nullptr);

    // 模式工具函数
    static QString getModeName(CanvasMode mode);

    // 对外接口
    void loadImage(const QString &path);
    void clearShapes();
    void setMode(CanvasMode mode);
    void setSamEnabled(bool enabled);
    bool isInsideImage(const QPointF &pt) const;
    QPointF clampPoint(const QPointF &pt) const;
    void cancelDrawing();
    int ImgW(){return _img_w;}
    int ImgH(){return _img_h;}
    // SAM回调入口
 //   void handleSamResult(const QPolygonF& polyPts, const QRectF& rectXYWH,
  //                       const QVariantList& rectOBB, float score, bool isClick);

signals:
    void mouseMoved(int x, int y);
    void shapeDrawn(QGraphicsItem* shape);
    void shapeDoubleClicked(QGraphicsItem* shape);
    void stateChanged();

protected:
    // 重写事件
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void updateCrosshair(const QPointF &pt);
    void updateTempPoly(const QPointF &mousePos = QPointF());
    void finishPolyShape();

private:
    int _img_w;
    int _img_h;
    CanvasMode m_mode;
    CanvasMode m_samMode;

    QGraphicsPixmapItem* m_imgItem;
    //SamClient* m_samClient;
    //bool m_samEnabled;

    bool m_drawing;
    QPointF m_startPt;
    QGraphicsItem* m_tempItem;
    QPolygonF m_polyPts;

    //QGraphicsItem* m_samHoverItem;

    QGraphicsLineItem m_hLine;
    QGraphicsLineItem m_vLine;
};

#endif // CANVAS_H
