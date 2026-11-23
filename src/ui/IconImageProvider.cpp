#include "ui/IconImageProvider.h"

#include "manager/ThemeManager.h"

IconImageProvider::IconImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Pixmap)
{
}

QPixmap IconImageProvider::requestPixmap(const QString& id, QSize* size, const QSize& requestedSize)
{
    const QString name = id.section(QLatin1Char('/'), 0, 0);
    const QColor tint(QLatin1Char('#') + id.section(QLatin1Char('/'), 1, 1));
    const QSize target = requestedSize.isValid() ? requestedSize : QSize(24, 24);
    if(size)
    {
        *size = target;
    }
    return ThemeManager::instance().svgPixmap(QStringLiteral(":/icons/%1.svg").arg(name), target,
                                              tint.isValid() ? tint : QColor(Qt::black), 1.0);
}
