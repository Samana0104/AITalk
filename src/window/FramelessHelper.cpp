#include "window/FramelessHelper.h"

#include <QGuiApplication>
#include <QMouseEvent>
#include <QWidget>
#include <QWindow>

FramelessHelper::FramelessHelper(QWidget* window, int border)
    : QObject(window)
    , m_widget(window)
    , m_border(border)
{
    m_widget->winId(); // QWindow 를 지금 만든다
    m_window = m_widget->windowHandle();
    if (m_window)
        m_window->installEventFilter(this);
}

Qt::Edges FramelessHelper::edgesAt(const QPoint& pos) const
{
    Qt::Edges e;
    if (m_widget->isMaximized() || m_widget->isFullScreen())
        return e;
    if (pos.x() <= m_border) e |= Qt::LeftEdge;
    if (pos.x() >= m_widget->width() - m_border) e |= Qt::RightEdge;
    if (pos.y() <= m_border) e |= Qt::TopEdge;
    if (pos.y() >= m_widget->height() - m_border) e |= Qt::BottomEdge;
    return e;
}

void FramelessHelper::setEdgeCursor(Qt::Edges edges)
{
    if (edges == m_current)
        return;
    m_current = edges;
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
        if (m_overriding)
            QGuiApplication::changeOverrideCursor(shape);
        else
            QGuiApplication::setOverrideCursor(shape);
        m_overriding = true;
    } else if (m_overriding) {
        QGuiApplication::restoreOverrideCursor();
        m_overriding = false;
    }
}

bool FramelessHelper::eventFilter(QObject* watched, QEvent* event)
{
    if (watched != m_window)
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
            m_window->startSystemResize(e);
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
