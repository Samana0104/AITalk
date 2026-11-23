#include "ui/MemberListModel.h"

#include "agent/AgentHub.h"
#include "core/Member.h"
#include "engine/RoomEngine.h"
#include "manager/DataManager.h"

MemberListModel::MemberListModel(RoomEngine* engine, AgentHub* hub, QObject* parent)
    : QAbstractListModel(parent)
    , engine(engine)
    , hub(hub)
{
}

void MemberListModel::Refresh(const QString& memberId)
{
    const int row = int(Members::ids().indexOf(memberId));
    if(row >= 0)
    {
        emit dataChanged(index(row), index(row));
    }
}

void MemberListModel::RefreshAll()
{
    emit dataChanged(index(0), index(rowCount() - 1));
}

int MemberListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : int(Members::all().size());
}

QVariant MemberListModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid() || index.row() >= rowCount())
    {
        return {};
    }
    const MemberInfo& info = Members::all().at(index.row());
    const MemberRuntime& runtime = this->engine->runtime(info.id);
    switch(static_cast<Role>(role))
    {
    case Role::MEMBER_ID:
        return info.id;
    case Role::NAME:
        return info.name;
    case Role::COLOR:
        return info.color;
    case Role::STATUS:
        return this->engine->statusOf(info.id);
    case Role::STATUS_LABEL:
        return StatusLabel(info.id, this->engine->statusOf(info.id));
    case Role::MODEL_LABEL:
        return this->engine->modelLabelOf(info.id);
    case Role::CLI_PATH:
        return this->hub->binOf(info.id);
    case Role::CALLS:
        return runtime.calls;
    case Role::LAST_SECONDS:
        return runtime.lastMs / 1000.0;
    case Role::LAST_ERROR:
        return runtime.lastError;
    case Role::IS_ENABLED:
        return this->engine->session() ? this->engine->session()->isMemberEnabled(info.id) : true;
    case Role::IS_BOOSTING:
        return runtime.deepNow;
    }
    return {};
}

QString MemberListModel::StatusLabel(const QString& memberId, const QString& status) const
{
    if(status == QLatin1String("reading"))
    {
        return this->engine->runtime(memberId).deepNow ? Text("status.thinkingHard") : Text("status.reading");
    }
    if(status == QLatin1String("typing"))
    {
        return Text("status.typing");
    }
    if(status == QLatin1String("offline"))
    {
        return Text("status.offline");
    }
    if(status == QLatin1String("disabled"))
    {
        return Text("status.disabled");
    }
    if(status == QLatin1String("missing"))
    {
        return Text("status.missing");
    }
    return Text("status.idle");
}

QHash<int, QByteArray> MemberListModel::roleNames() const
{
    return {
        { int(Role::MEMBER_ID), "memberId" },
        { int(Role::NAME), "name" },
        { int(Role::COLOR), "memberColor" },
        { int(Role::STATUS), "status" },
        { int(Role::STATUS_LABEL), "statusLabel" },
        { int(Role::MODEL_LABEL), "modelLabel" },
        { int(Role::CLI_PATH), "cliPath" },
        { int(Role::CALLS), "calls" },
        { int(Role::LAST_SECONDS), "lastSeconds" },
        { int(Role::LAST_ERROR), "lastError" },
        { int(Role::IS_ENABLED), "isEnabled" },
        { int(Role::IS_BOOSTING), "isBoosting" },
    };
}
