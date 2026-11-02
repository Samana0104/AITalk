#include "component/IconButton.h"

#include "style/ThemeManager.h"

IconButton::IconButton(const QString& icon, const QString& tooltip, QWidget* parent)
    : QToolButton(parent)
    , m_icon(icon)
{
    setObjectName(QStringLiteral("iconButton"));
    setAutoRaise(true);
    setCursor(Qt::PointingHandCursor);
    setIconSize(QSize(16, 16));
    setToolTip(tooltip);
    setFocusPolicy(Qt::TabFocus);
    refresh();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &IconButton::refresh);
}

void IconButton::setIconName(const QString& icon)
{
    m_icon = icon;
    refresh();
}

void IconButton::setTone(const QString& token)
{
    m_tone = token;
    refresh();
}

void IconButton::refresh()
{
    setIcon(ThemeManager::instance().icon(m_icon, m_tone));
}
