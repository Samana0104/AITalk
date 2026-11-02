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
    , id(memberId)
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
    this->dot = new StatusDot(8, this);
    this->status = new QLabel(this);
    this->status->setObjectName(QStringLiteral("memberCardMeta"));
    top->addWidget(name);
    top->addWidget(this->dot);
    top->addWidget(this->status);
    top->addStretch(1);
    col->addLayout(top);

    this->model = new QLabel(this);
    this->model->setObjectName(QStringLiteral("memberCardMeta"));
    this->model->setWordWrap(true);
    this->stats = new QLabel(this);
    this->stats->setObjectName(QStringLiteral("memberCardMeta"));
    this->error = new QLabel(this);
    this->error->setObjectName(QStringLiteral("memberCardError"));
    this->error->setWordWrap(true);
    this->error->hide();
    col->addWidget(this->model);
    col->addWidget(this->stats);
    col->addWidget(this->error);
    lay->addLayout(col, 1);
}

void MemberCard::setState(const State& s)
{
    this->dot->setStatus(s.status);
    this->status->setText(StatusDot::labelFor(s.status));
    this->model->setText(s.cliPath.isEmpty() ? Text("member.noCli") : s.model);
    this->model->setToolTip(s.cliPath);
    this->stats->setText(s.calls ? Text("member.stats").arg(s.calls).arg(s.lastMs / 1000.0, 0, 'f', 1) : Text("member.noCalls"));
    this->error->setText(s.lastError.left(160));
    this->error->setToolTip(s.lastError);
    this->error->setVisible(!s.lastError.isEmpty());
}
