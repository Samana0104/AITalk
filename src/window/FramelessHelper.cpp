#include "window/FramelessHelper.h"

#include <QGuiApplication>
#include <QMouseEvent>
#include <QWidget>
#include <QWindow>

FramelessHelper::FramelessHelper(QWidget* window, int border)
    : QObject(window)
    , widget(window)
    , border(border)
{
    this->widget->winId(); // QWindow 를 지금 만든다
    this->window = this->widget->windowHandle();
    if (this->window)
        this->window->installEventFilter(this);
}

Qt::Edges FramelessHelper::edgesAt(const QPoint& pos) const
{
    Qt::Edges e;
    if (this->widget->isMaximized() || this->widget->isFullScreen())
        return e;
    if (pos.x() <= this->border) e |= Qt::LeftEdge;
    if (pos.x() >= this->widget->width() - this->border) e |= Qt::RightEdge;
    if (pos.y() <= this->border) e |= Qt::TopEdge;
    if (pos.y() >= this->widget->height() - this->border) e |= Qt::BottomEdge;
    return e;
}

void FramelessHelper::setEdgeCursor(Qt::Edges edges)
{
    if (edges == this->current)
        return;
    this->current = edges;
    Qt::CursorShape shape = Qt::ArrowCursor;
    if (edges == (Qt::LeftEdge | Qt::TopEdge) || edges == (Qt::RightEdge | Qt::BottomEdge))
        shape = Qt::SizeFDiagCursor;
    else if (edges == (Qt::RightEdge | Qt::TopEdge) || edges == (Qt::LeftEdge | Qt::BottomEdge))
        shape = Qt::SizeBDiagCursor;
    else if (edges & (Qt::LeftEdge | Qt::RightEdge))
        shape = Qt::SizeHorCursor;
    else if (edges & (Qt::TopEdge | Qt::BottomEdge))
        shape = Qt::SizeVerCursor;

    if (edges) {
        if (this->overriding)
            QGuiApplication::changeOverrideCursor(shape);
        else
            QGuiApplication::setOverrideCursor(shape);
        this->overriding = true;
    } else if (this->overriding) {
        QGuiApplication::restoreOverrideCursor();
        this->overriding = false;
    }
}

bool FramelessHelper::eventFilter(QObject* watched, QEvent* event)
{
    if (watched != this->window)
        return QObject::eventFilter(watched, event);

    switch (event->type()) {
    case QEvent::MouseMove: {
        auto* me = static_cast<QMouseEvent*>(event);
        if (me->buttons() == Qt::NoButton)
            setEdgeCursor(edgesAt(me->position().toPoint()));
        break;
    }
    case QEvent::MouseButtonPress: {
        auto* me = static_cast<QMouseEvent*>(event);
        const Qt::Edges e = edgesAt(me->position().toPoint());
        if (me->button() == Qt::LeftButton && e) {
            this->window->startSystemResize(e);
            return true;
        }
        break;
    }
    case QEvent::Leave:
        setEdgeCursor({});
        break;
    default:
        break;
    }
    return QObject::eventFilter(watched, event);
}
