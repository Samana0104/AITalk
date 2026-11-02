#include "style/ThemeManager.h"

#include <QApplication>
#include <QFile>
#include <QFont>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QPalette>
#include <QRegularExpression>
#include <QStyleHints>
#include <QSvgRenderer>

namespace {
QByteArray readResource(const QString& path)
{
    QFile f(path);
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}
}

ThemeManager& ThemeManager::instance()
{
    static ThemeManager* inst = new ThemeManager(); // 앱이 끝날 때까지 산다
    return *inst;
}

void ThemeManager::apply(const QString& theme, int basePointSize)
{
    this->themeName = theme == QLatin1String("dark") ? QStringLiteral("dark") : QStringLiteral("light");
    this->tokens.clear();
    const QJsonObject tokens = QJsonDocument::fromJson(readResource(QStringLiteral(":/styles/%1.json").arg(this->themeName))).object();
    for (auto it = tokens.begin(); it != tokens.end(); ++it)
        this->tokens.insert(it.key(), it.value().toString());

    const int base = qBound(8, basePointSize, 18);
    this->tokens.insert(QStringLiteral("font-base"), QString::number(base));
    this->tokens.insert(QStringLiteral("font-sm"), QString::number(base - 1));
    this->tokens.insert(QStringLiteral("font-xs"), QString::number(base - 2));
    this->tokens.insert(QStringLiteral("font-lg"), QString::number(base + 2));

    QFont font = QApplication::font();
    font.setFamilies({ QStringLiteral("Segoe UI Variable Text"), QStringLiteral("Segoe UI"),
                       QStringLiteral("Malgun Gothic"), QStringLiteral("Apple SD Gothic Neo"), QStringLiteral("Noto Sans KR") });
    font.setPointSize(base);
    QApplication::setFont(font);

    QPalette pal = QApplication::palette();
    pal.setColor(QPalette::Window, color(QStringLiteral("page")));
    pal.setColor(QPalette::WindowText, color(QStringLiteral("ink")));
    pal.setColor(QPalette::Base, color(QStringLiteral("surface")));
    pal.setColor(QPalette::Text, color(QStringLiteral("ink")));
    pal.setColor(QPalette::Button, color(QStringLiteral("surface")));
    pal.setColor(QPalette::ButtonText, color(QStringLiteral("ink")));
    pal.setColor(QPalette::Highlight, color(QStringLiteral("accent")));
    pal.setColor(QPalette::HighlightedText, color(QStringLiteral("accent-ink")));
    pal.setColor(QPalette::Link, color(QStringLiteral("link")));
    pal.setColor(QPalette::PlaceholderText, color(QStringLiteral("muted")));
    QApplication::setPalette(pal);
    QGuiApplication::styleHints()->setColorScheme(this->themeName == QLatin1String("dark") ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light);

    QString qss = QString::fromUtf8(readResource(QStringLiteral(":/styles/base.qss")));
    static const QRegularExpression token(QStringLiteral(R"(@\{([a-z0-9-]+)\})"));
    QString out;
    qsizetype last = 0;
    for (auto it = token.globalMatch(qss); it.hasNext();) {
        const QRegularExpressionMatch m = it.next();
        out += qss.mid(last, m.capturedStart() - last);
        out += this->tokens.value(m.captured(1), QStringLiteral("#ff00ff")); // 빠진 토큰은 눈에 띄게
        last = m.capturedEnd();
    }
    out += qss.mid(last);
    qApp->setStyleSheet(out);

    emit themeChanged();
}

QColor ThemeManager::color(const QString& token) const
{
    return QColor(this->tokens.value(token, QStringLiteral("#ff00ff")));
}

QPixmap ThemeManager::svgPixmap(const QString& resourcePath, const QSize& size, const QColor& tint, qreal dpr) const
{
    QByteArray svg = readResource(resourcePath);
    svg.replace("currentColor", tint.name(QColor::HexRgb).toLatin1());
    QSvgRenderer renderer(svg);
    QPixmap pm(size * dpr);
    pm.fill(Qt::transparent);
    pm.setDevicePixelRatio(dpr);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    renderer.render(&p, QRectF(QPointF(0, 0), QSizeF(size)));
    return pm;
}

QIcon ThemeManager::icon(const QString& name, const QString& token) const
{
    const QString path = QStringLiteral(":/icons/%1.svg").arg(name);
    QIcon ic;
    for (int s : { 16, 20, 24, 32 }) {
        ic.addPixmap(svgPixmap(path, QSize(s, s), color(token), 2.0), QIcon::Normal);
        ic.addPixmap(svgPixmap(path, QSize(s, s), color(QStringLiteral("muted")), 2.0), QIcon::Disabled);
    }
    return ic;
}
