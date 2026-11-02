#include "widget/SessionItemWidget.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include "component/ElidedLabel.h"
#include "util/DataManager.h"

SessionItemWidget::SessionItemWidget(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents); // 클릭은 목록(QListWidget)이 받는다
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(10, 7, 10, 7);
    lay->setSpacing(1);

    auto* top = new QHBoxLayout;
    top->setSpacing(6);
    this->title = new ElidedLabel(QString(), this);
    this->title->setObjectName(QStringLiteral("sessionItemTitle"));
    QFont bold = this->title->font();
    bold.setBold(true);
    this->title->setFont(bold);
    this->time = new QLabel(this);
    this->time->setObjectName(QStringLiteral("sessionItemMeta"));
    top->addWidget(this->title, 1);
    top->addWidget(this->time);
    lay->addLayout(top);

    this->preview = new ElidedLabel(QString(), this);
    this->preview->setObjectName(QStringLiteral("sessionItemPreview"));
    lay->addWidget(this->preview);
}

void SessionItemWidget::setContent(const QString& title, const QString& preview, qint64 updatedAt)
{
    this->title->setText(title);
    this->preview->setText(preview.isEmpty() ? Text("sessions.empty") : preview);
    this->time->setText(relativeTime(updatedAt));
}

QString SessionItemWidget::relativeTime(qint64 ts)
{
    const qint64 secs = (QDateTime::currentMSecsSinceEpoch() - ts) / 1000;
    if (secs < 60)
        return Text("time.justNow");
    if (secs < 3600)
        return Text("time.minutesAgo").arg(secs / 60);
    if (secs < 86400)
        return Text("time.hoursAgo").arg(secs / 3600);
    const QDateTime d = QDateTime::fromMSecsSinceEpoch(ts);
    if (secs < 2 * 86400)
        return Text("time.yesterday");
    return d.toString(QStringLiteral("M/d"));
}
