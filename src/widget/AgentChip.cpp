#include "widget/AgentChip.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>

#include "component/AvatarWidget.h"
#include "component/StatusDot.h"
#include "core/Member.h"

AgentChip::AgentChip(const QString& memberId, QWidget* parent)
    : QPushButton(parent)
    , id(memberId)
{
    setObjectName(QStringLiteral("agentChip"));
    setCursor(Qt::PointingHandCursor);
    setProperty("active", true);

    auto* lay = new QHBoxLayout(this);
    lay->setContentsMargins(4, 3, 10, 3);
    lay->setSpacing(6);

    this->avatar = new AvatarWidget(22, this);
    this->avatar->setIdentity(memberId, Members::displayName(memberId, {}), Members::colorOf(memberId));
    this->avatar->setAttribute(Qt::WA_TransparentForMouseEvents);
    this->name = new QLabel(Members::displayName(memberId, {}), this);
    this->name->setObjectName(QStringLiteral("agentChipName"));
    this->name->setAttribute(Qt::WA_TransparentForMouseEvents);
    this->dot = new StatusDot(8, this);
    this->dot->setAttribute(Qt::WA_TransparentForMouseEvents);

    lay->addWidget(this->avatar);
    lay->addWidget(this->name);
    lay->addWidget(this->dot);
}

QSize AgentChip::sizeHint() const
{
    return layout()->sizeHint().expandedTo(QSize(0, 32));
}

QSize AgentChip::minimumSizeHint() const
{
    return sizeHint();
}

void AgentChip::setState(const QString& status, const QString& modelLabel, bool boosting)
{
    this->dot->setStatus(status);
    const bool active = status != QLatin1String("disabled") && status != QLatin1String("missing");
    if (property("active").toBool() != active) {
        setProperty("active", active);
        style()->unpolish(this);
        style()->polish(this);
        style()->unpolish(this->name);
        style()->polish(this->name);
    }
    this->name->setText(Members::displayName(this->id, {}) + (boosting ? QStringLiteral(" 🔥") : QString()));
    setToolTip(QStringLiteral("%1\n%2").arg(StatusDot::labelFor(status), modelLabel));
    updateGeometry();
}
