#include "component/ChipButton.h"

#include "style/ThemeManager.h"

ChipButton::ChipButton(const QString& text, QWidget* parent)
    : ChipButton(QString(), text, parent)
{
}

ChipButton::ChipButton(const QString& iconName, const QString& text, QWidget* parent)
    : QPushButton(text, parent)
    , m_iconName(iconName)
{
    setProperty("chip", true);
    setCursor(Qt::PointingHandCursor);
    setIconSize(QSize(13, 13));
    refreshIcon();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &ChipButton::refreshIcon);
    connect(this, &QPushButton::toggled, this, &ChipButton::refreshIcon);
}

void ChipButton::refreshIcon()
{
    if (m_iconName.isEmpty())
        return;
    const QString tone = isChecked() ? QStringLiteral("accent-ink") : QStringLiteral("ink-soft");
    setIcon(ThemeManager::instance().icon(m_iconName, tone));
}
