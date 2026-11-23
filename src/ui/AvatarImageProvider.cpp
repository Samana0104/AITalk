#include "ui/AvatarImageProvider.h"

#include <QFile>
#include <QPainter>
#include <QPainterPath>

#include "core/Member.h"

namespace AITalk
{

AvatarImageProvider::AvatarImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage AvatarImageProvider::requestImage(const QString& id, QSize* size, const QSize& requestedSize)
{
    const int side = requestedSize.isValid() ? qMax(requestedSize.width(), requestedSize.height()) : 64;
    if(size)
    {
        *size = QSize(side, side);
    }
    QImage out(side, side, QImage::Format_ARGB32_Premultiplied);
    out.fill(Qt::transparent);
    QPainter painter(&out);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    QPainterPath clip;
    clip.addRoundedRect(QRectF(0, 0, side, side), side * 0.32, side * 0.32);

    const MemberInfo* info = Members::find(id);
    const QString file = QStringLiteral(":/avatars/%1.png").arg(id);
    const QImage picture = QFile::exists(file) ? QImage(file) : QImage();
    if(!picture.isNull())
    {
        // 원본(600px)을 정사각형에 꽉 차게(cover) 줄인다.
        painter.fillPath(clip, info ? info->avatarBg : QColor::fromRgb(0xe4e4e7));
        painter.setClipPath(clip);
        const QImage scaled = picture.scaled(side, side, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        painter.drawImage(QPointF((side - scaled.width()) / 2.0, (side - scaled.height()) / 2.0), scaled);
        return out;
    }

    painter.fillPath(clip, Members::colorOf(id));
    QFont font;
    font.setBold(true);
    font.setPixelSize(qRound(side * 0.42));
    painter.setFont(font);
    painter.setPen(Qt::white);
    painter.drawText(QRectF(0, 0, side, side), Qt::AlignCenter, Members::displayName(id, {}).left(1).toUpper());
    return out;
}

} // namespace AITalk
