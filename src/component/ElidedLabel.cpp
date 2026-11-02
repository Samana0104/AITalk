#include "component/ElidedLabel.h"

#include <QMouseEvent>
#include <QPainter>

ElidedLabel::ElidedLabel(const QString& text, QWidget* parent)
    : QFrame(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setText(text);
}

void ElidedLabel::setText(const QString& text)
{
    this->fullText = text;
    setToolTip(text);
    updateGeometry();
    update();
}

void ElidedLabel::setElideMode(Qt::TextElideMode mode)
{
    this->mode = mode;
    update();
}

QSize ElidedLabel::sizeHint() const
{
    const QFontMetrics fm(font());
    return { fm.horizontalAdvance(this->fullText) + 2, fm.height() };
}

QSize ElidedLabel::minimumSizeHint() const
{
    const QFontMetrics fm(font());
    return { fm.horizontalAdvance(QStringLiteral("…")) * 3, fm.height() };
}

void ElidedLabel::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setPen(palette().color(foregroundRole()));
    const QRect r = contentsRect();
    p.drawText(r, Qt::AlignVCenter | Qt::AlignLeft, fontMetrics().elidedText(this->fullText, this->mode, r.width()));
}

void ElidedLabel::mouseDoubleClickEvent(QMouseEvent* event)
{
    emit doubleClicked();
    QFrame::mouseDoubleClickEvent(event);
}
