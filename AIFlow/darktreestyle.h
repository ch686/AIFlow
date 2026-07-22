#pragma once

#include <QProxyStyle>

class DarkTreeStyle : public QProxyStyle
{
public:
    using QProxyStyle::QProxyStyle;

    void drawPrimitive(PrimitiveElement element,
                       const QStyleOption *option,
                       QPainter *painter,
                       const QWidget *widget = nullptr) const override;
};
