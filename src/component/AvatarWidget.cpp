#include "component/AvatarWidget.h"

#include <QFile>
#include <QHash>
#include <QPainter>
#include <QPainterPath>

#include "core/Member.h"

namespace {

QPainterPath roundedSquare(qreal size)
{
    QPainterPath path;
    path.addRoundedRect(QRectF(0, 0, size, size), size * 0.32, size * 0.32);
    return path;
}

// 원본(600px)을 정사각형에 꽉 차게(cover) 줄인다.
QPixmap renderAvatar(const QString& memberId, const QString& label, const QColor& fallback, int size, qreal dpr)
{
    QPixmap pm(QSize(size, size) * dpr);
    pm.fill(Qt::transparent);
    pm.setDevicePixelRatio(dpr);
    QPainter p(&pm);
    p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    const QPainterPath clip = roundedSquare(size);

    const MemberInfo* info = Members::find(memberId);
    const QString file = QStringLiteral(":/avatars/%1.png").arg(memberId);
    const QImage img = QFile::exists(file) ? QImage(file) : QImage();
    if (!img.isNull()) {
        p.fillPath(clip, info ? info->avatarBg : QColor::fromRgb(0xe4e4e7));
        p.setClipPath(clip);
        const QImage scaled = img.scaled(QSize(size, size) * dpr, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        const QRectF src((scaled.width() - size * dpr) / 2.0, (scaled.height() - size * dpr) / 2.0, size * dpr, size * dpr);
        p.drawImage(QRectF(0, 0, size, size), scaled, src);
        return pm;
    }

    p.fillPath(clip, fallback);
    QFont f;
    f.setBold(true);
    f.setPixelSize(qRound(size * 0.42));
    p.setFont(f);
    p.setPen(Qt::white);
    p.drawText(QRectF(0, 0, size, size), Qt::AlignCenter, label.left(1).toUpper());
    return pm;
}

}

AvatarWidget::AvatarWidget(int size, QWidget* parent)
    : QWidget(parent)
    , m_size(size)
    , m_color(Qt::gray)
{
    setFixedSize(size, size);
}

void AvatarWidget::setIdentity(const QString& id, const QString& label, const QColor& color)
{
    m_id = id;
    m_label = label;
    m_color = color;
    update();
}

QSize AvatarWidget::sizeHint() const
{
    return { m_size, m_size };
}

QPixmap AvatarWidget::pixmapFor(const QString& memberId, int size, qreal dpr)
{
    static QHash<QString, QPixmap> cache;
    const QString key = QStringLiteral("%1@%2x%3").arg(memberId).arg(size).arg(dpr);
    auto it = cache.constFind(key);
    if (it != cache.cend())
        return it.value();
    const QPixmap pm = renderAvatar(memberId, Members::displayName(memberId, {}), Members::colorOf(memberId), size, dpr);
    cache.insert(key, pm);
    return pm;
}

void AvatarWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    if (Members::isAgent(m_id))
        p.drawPixmap(0, 0, pixmapFor(m_id, m_size, devicePixelRatioF()));
    else
        p.drawPixmap(0, 0, renderAvatar(m_id, m_label, m_color, m_size, devicePixelRatioF()));
}
