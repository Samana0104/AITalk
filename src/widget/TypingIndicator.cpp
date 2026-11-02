#include "widget/TypingIndicator.h"

#include <QHBoxLayout>
#include <QLabel>

TypingIndicator::TypingIndicator(QWidget* parent)
    : QWidget(parent)
{
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(20, 2, 20, 2);
    m_layout->setSpacing(6);
    m_layout->addStretch(1);
    m_timer.setInterval(450);
    connect(&m_timer, &QTimer::timeout, this, &TypingIndicator::tickDots);
    setVisible(false);
}

void TypingIndicator::setEntries(const QList<QPair<QString, QString>>& entries)
{
    if (entries == m_entries)
        return;
    m_entries = entries;
    qDeleteAll(m_pills);
    m_pills.clear();
    for (int i = 0; i < entries.size(); ++i) {
        auto* pill = new QLabel(this);
        pill->setObjectName(QStringLiteral("typingPill"));
        m_layout->insertWidget(i, pill);
        m_pills << pill;
    }
    setVisible(!entries.isEmpty());
    entries.isEmpty() ? m_timer.stop() : m_timer.start();
    tickDots();
}

void TypingIndicator::tickDots()
{
    m_dots = (m_dots + 1) % 4;
    const QString dots = QString(m_dots, QLatin1Char('.')).leftJustified(3, QLatin1Char(' '));
    for (int i = 0; i < m_pills.size(); ++i)
        m_pills[i]->setText(QStringLiteral("● %1 %2%3").arg(m_entries[i].first, m_entries[i].second, dots));
}
