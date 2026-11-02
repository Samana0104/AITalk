#include "component/Toast.h"

#include <QEvent>

Toast::Toast(QWidget* host)
    : QLabel(host)
{
    setObjectName(QStringLiteral("toast"));
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAlignment(Qt::AlignCenter);
    hide();
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &QWidget::hide);
    host->installEventFilter(this);
}

void Toast::showMessage(const QString& text, int ms)
{
    setText(text);
    adjustSize();
    reposition();
    show();
    raise();
    m_timer.start(ms);
}

bool Toast::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == parentWidget() && event->type() == QEvent::Resize && isVisible())
        reposition();
    return QLabel::eventFilter(watched, event);
}

void Toast::reposition()
{
    const QWidget* host = parentWidget();
    move((host->width() - width()) / 2, host->height() - height() - 96);
}
