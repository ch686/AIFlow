#include "baseshape.h"

#include <Qt>
#include <QGraphicsDropShadowEffect>

void BaseShape::setupLabel(QGraphicsItem* parentItem)
{
    labelText = new QGraphicsTextItem(parentItem);
    labelText->setDefaultTextColor(QColor(255, 255, 255));
    QFont f("Arial", 10, QFont::Bold);
    labelText->setFont(f);
    labelText->setZValue(1001);

    // 创建阴影效果
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(4);        // 模糊半径，越大越柔和
    shadow->setXOffset(1);           // X 偏移（向右阴影正数）
    shadow->setYOffset(1);           // Y 偏移（向下阴影正数）
    shadow->setColor(QColor(0,0,0,180)); // 黑色半透明阴影

    // 绑定到文字
    labelText->setGraphicsEffect(shadow);

    labelText->hide();
}

void BaseShape::updateLabelPosition(QGraphicsItem* item)
{
    if (!labelText)
        return;
    QRectF bound = item->boundingRect();
    qreal x = bound.center().x();
    qreal y = bound.top() -  labelText->boundingRect().height();;
    qreal textW = labelText->boundingRect().width();
    labelText->setPos(x - textW / 2.0, y);
}

void BaseShape::updateLabelText(const QString& text,int class_id)
{
    if (!labelText) return;
    labelText->setData(DATA_class_id,class_id);
    labelText->setPlainText(text);
    labelText->show();

}

int BaseShape::getLabelText(QString& text)
{
    if (!labelText) return -1;
    text=labelText->toPlainText();
    return labelText->data(DATA_class_id).toInt();
}

void BaseShape::updateLabelVisibility(bool isSelected, bool isHovered)
{
    if (!labelText) return;
    if (isSelected || isHovered)
        labelText->show();
    else
        labelText->hide();
}

