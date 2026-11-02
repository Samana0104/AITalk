#include "component/StatusDot.h"

#include <QPainter>

#include "util/DataManager.h"

StatusDot::StatusDot(int size, QWidget* parent)
    : QWidget(parent)
    , m_size(size)
{
    setFixedSize(size, size);
}

void StatusDot::setStatus(const QString& status)
{
    if (status == m_status)
        return;
    m_status = status;
    setToolTip(labelFor(status));
    update();
}

QColor StatusDot::colorFor(const QString& s)
{
    if (s == QLatin1String("reading")) return QColor::fromRgb(0xf59e0b);
    if (s == QLatin1String("typing")) return QColor::fromRgb(0x3b82f6);
    if (s == QLatin1String("offline")) return QColor::fromRgb(0xef4444);
    if (s == QLatin1String("disabled") || s == QLatin1String("missing") || s == QLatin1String("sleeping"))
        return QColor::fromRgb(0xa1a1aa);
    return QColor::fromRgb(0x22c55e);
}

QString StatusDot::labelFor(const QString& s)
{
    if (s == QLatin1String("reading")) return Text("status.reading");
    if (s == QLatin1String("typing")) return Text("status.typing");
    if (s == QLatin1String("offline")) return Text("status.offline");
    if (s == QLatin1String("disabled")) return Text("status.disabled");
    if (s == QLatin1String("missing")) return Text("status.missing");
    if (s == QLatin1String("sleeping")) return Text("status.sleeping");
    return Text("status.idle");
}

void StatusDot::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QColor c = colorFor(m_status);
    const QRectF r(0.5, 0.5, m_size - 1.0, m_size - 1.0);
    if (m_status == QLatin1String("disabled") || m_status == QLatin1String("missing")) {
        p.setPen(QPen(c, 1.2));
        p.setBrush(Qt::NoBrush);
    } else {
        p.setPen(Qt::NoPen);
        p.setBrush(c);
    }
    p.drawEllipse(r);
}
