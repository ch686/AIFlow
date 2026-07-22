#include "darktreestyle.h"

#include <QPainter>
#include <QStyleOption>
#include <QPolygon>

void DarkTreeStyle::drawPrimitive(PrimitiveElement element,
                                  const QStyleOption *option,
                                  QPainter *painter,
                                  const QWidget *widget) const
{
    if (element == PE_IndicatorBranch) {
        // 无子节点 → 不绘制
        if (!(option->state & State_Children))
            return;

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(Qt::NoPen);

        // 展开 → 绿色，折叠 → 灰色
        QColor color = (option->state & State_Open)
            ? QColor("#22C55E")
            : QColor("#94A3B8");
        painter->setBrush(color);

        QRect rect = option->rect;
        int cx = rect.center().x();
        int cy = rect.center().y();
        int s  = qMin(rect.width(), rect.height()) / 3;

        QPolygon triangle;
        if (option->state & State_Open) {
            // ▼ 下箭头（已展开）
            triangle << QPoint(cx - s, cy - s / 2)
                     << QPoint(cx + s, cy - s / 2)
                     << QPoint(cx,     cy + s);
        } else {
            // ▶ 右箭头（已折叠）
            triangle << QPoint(cx - s / 2, cy - s)
                     << QPoint(cx + s,     cy)
                     << QPoint(cx - s / 2, cy + s);
        }

        painter->drawPolygon(triangle);
        painter->restore();
        return;
    }

    // 其余元素走默认绘制
    QProxyStyle::drawPrimitive(element, option, painter, widget);
}
