#include "widget/AgentChip.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>

#include "component/AvatarWidget.h"
#include "component/StatusDot.h"
#include "core/Member.h"

AgentChip::AgentChip(const QString& memberId, QWidget* parent)
    : QPushButton(parent)
    , m_id(memberId)
{
    setObjectName(QStringLiteral("agentChip"));
    setCursor(Qt::PointingHandCursor);
    setProperty("active", true);

    auto* lay = new QHBoxLayout(this);
    lay->setContentsMargins(4, 3, 10, 3);
    lay->setSpacing(6);

    m_avatar = new AvatarWidget(22, this);
    m_avatar->setIdentity(memberId, Members::displayName(memberId, {}), Members::colorOf(memberId));
    m_avatar->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_name = new QLabel(Members::displayName(memberId, {}), this);
    m_name->setObjectName(QStringLiteral("agentChipName"));
    m_name->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_dot = new StatusDot(8, this);
    m_dot->setAttribute(Qt::WA_TransparentForMouseEvents);

    lay->addWidget(m_avatar);
    lay->addWidget(m_name);
    lay->addWidget(m_dot);
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
    m_dot->setStatus(status);
    const bool active = status != QLatin1String("disabled") && status != QLatin1String("missing");
    if (property("active").toBool() != active) {
        setProperty("active", active);
        style()->unpolish(this);
        style()->polish(this);
        style()->unpolish(m_name);
        style()->polish(m_name);
    }
    m_name->setText(Members::displayName(m_id, {}) + (boosting ? QStringLiteral(" 🔥") : QString()));
    setToolTip(QStringLiteral("%1\n%2").arg(StatusDot::labelFor(status), modelLabel));
    updateGeometry();
}
