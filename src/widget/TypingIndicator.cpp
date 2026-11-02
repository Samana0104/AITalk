#include "widget/TypingIndicator.h"

#include <QHBoxLayout>
#include <QLabel>

TypingIndicator::TypingIndicator(QWidget* parent)
    : QWidget(parent)
{
    this->layout = new QHBoxLayout(this);
    this->layout->setContentsMargins(20, 2, 20, 2);
    this->layout->setSpacing(6);
    this->layout->addStretch(1);
    this->timer.setInterval(450);
    connect(&this->timer, &QTimer::timeout, this, &TypingIndicator::tickDots);
    setVisible(false);
}

void TypingIndicator::setEntries(const QList<QPair<QString, QString>>& entries)
{
    if (entries == this->entries)
        return;
    this->entries = entries;
    qDeleteAll(this->pills);
    this->pills.clear();
    for (int i = 0; i < entries.size(); ++i) {
        auto* pill = new QLabel(this);
        pill->setObjectName(QStringLiteral("typingPill"));
        this->layout->insertWidget(i, pill);
        this->pills << pill;
    }
    setVisible(!entries.isEmpty());
    entries.isEmpty() ? this->timer.stop() : this->timer.start();
    tickDots();
}

void TypingIndicator::tickDots()
{
    this->dots = (this->dots + 1) % 4;
    const QString dots = QString(this->dots, QLatin1Char('.')).leftJustified(3, QLatin1Char(' '));
    for (int i = 0; i < this->pills.size(); ++i)
        this->pills[i]->setText(QStringLiteral("● %1 %2%3").arg(this->entries[i].first, this->entries[i].second, dots));
}
