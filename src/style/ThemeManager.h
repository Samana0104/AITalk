#pragma once

#include <QColor>
#include <QHash>
#include <QIcon>
#include <QObject>
#include <QPixmap>

// 테마(라이트/다크)와 글꼴 크기를 앱 전체에 적용한다.
//  - res/styles/<theme>.json 의 색 토큰과 글꼴 크기 토큰으로 res/styles/base.qss 의 @{토큰} 을 치환
//  - res/icons/*.svg 의 currentColor 를 토큰 색으로 칠해서 아이콘을 만든다
class ThemeManager : public QObject {
    Q_OBJECT
public:
    static ThemeManager& instance();

    void apply(const QString& theme, int basePointSize);
    QString theme() const { return this->themeName; }

    QColor color(const QString& token) const;
    QIcon icon(const QString& name, const QString& token = QStringLiteral("ink-soft")) const;
    QPixmap svgPixmap(const QString& resourcePath, const QSize& size, const QColor& tint, qreal dpr) const;

signals:
    void themeChanged();

private:
    ThemeManager() = default;

    QString themeName;
    QHash<QString, QString> tokens;
};
