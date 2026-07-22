#ifndef RECTSHAPE_H
#define RECTSHAPE_H

#include <QGraphicsRectItem>
#include "baseshape.h"
#include "handleitem.h"

class RectShape : public QGraphicsRectItem, public BaseShape
{
public:
    explicit RectShape(const QRectF& rect, const QString& label = "");

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

public :
    void _update_handle_visibility();
    void update_handles_pos();
    void update_from_handle(QGraphicsItem* draggedHandle);

private:
    QString m_label;
    bool m_updatingHandles = false;
    bool m_hovered = false;

    HandleItem* m_ltHandle;
    HandleItem* m_rtHandle;
    HandleItem* m_lbHandle;
    HandleItem* m_rbHandle;
};

#endif // RECTSHAPE_H
