#include "widget/MemberCard.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include "component/AvatarWidget.h"
#include "component/StatusDot.h"
#include "core/Member.h"
#include "util/DataManager.h"

MemberCard::MemberCard(const QString& memberId, QWidget* parent)
    : QFrame(parent)
    , m_id(memberId)
{
    setObjectName(QStringLiteral("memberCard"));
    const MemberInfo* info = Members::find(memberId);

    auto* lay = new QHBoxLayout(this);
    lay->setContentsMargins(10, 10, 10, 10);
    lay->setSpacing(10);

    auto* avatar = new AvatarWidget(34, this);
    avatar->setIdentity(memberId, info ? info->name : memberId, Members::colorOf(memberId));
    lay->addWidget(avatar, 0, Qt::AlignTop);

    auto* col = new QVBoxLayout;
    col->setSpacing(2);
    auto* top = new QHBoxLayout;
    top->setSpacing(6);
    auto* name = new QLabel(info ? info->name : memberId, this);
    name->setObjectName(QStringLiteral("memberCardName"));
    m_dot = new StatusDot(8, this);
    m_status = new QLabel(this);
    m_status->setObjectName(QStringLiteral("memberCardMeta"));
    top->addWidget(name);
    top->addWidget(m_dot);
    top->addWidget(m_status);
    top->addStretch(1);
    col->addLayout(top);

    m_model = new QLabel(this);
    m_model->setObjectName(QStringLiteral("memberCardMeta"));
    m_model->setWordWrap(true);
    m_stats = new QLabel(this);
    m_stats->setObjectName(QStringLiteral("memberCardMeta"));
    m_error = new QLabel(this);
    m_error->setObjectName(QStringLiteral("memberCardError"));
    m_error->setWordWrap(true);
    m_error->hide();
    col->addWidget(m_model);
    col->addWidget(m_stats);
    col->addWidget(m_error);
    lay->addLayout(col, 1);
}

void MemberCard::setState(const State& s)
{
    m_dot->setStatus(s.status);
    m_status->setText(StatusDot::labelFor(s.status));
    m_model->setText(s.cliPath.isEmpty() ? Text("member.noCli") : s.model);
    m_model->setToolTip(s.cliPath);
    m_stats->setText(s.calls ? Text("member.stats").arg(s.calls).arg(s.lastMs / 1000.0, 0, 'f', 1) : Text("member.noCalls"));
    m_error->setText(s.lastError.left(160));
    m_error->setToolTip(s.lastError);
    m_error->setVisible(!s.lastError.isEmpty());
}
