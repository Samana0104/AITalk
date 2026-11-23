#include "ui/SessionListModel.h"

#include <QDateTime>
#include <QRegularExpression>
#include <algorithm>

#include "core/Member.h"
#include "storage/ChatSession.h"
#include "storage/SessionManager.h"
#include "util/DataManager.h"

namespace
{
QString PreviewOf(const ChatSession* session)
{
    static const QRegularExpression markdownMarks(QStringLiteral("[*_`#>~]+"));
    const QList<ChatMessage>& all = session->messages();
    for(qsizetype i = all.size() - 1; i >= 0; --i)
    {
        const ChatMessage& message = all.at(i);
        if(!message.isSystem() && !message.displayText().isEmpty())
        {
            return QStringLiteral("%1: %2").arg(Members::displayName(message.from, Text("common.me")),
                                                message.displayText().remove(markdownMarks).simplified());
        }
    }
    return {};
}
}

SessionListModel::SessionListModel(SessionManager* sessions, QObject* parent)
    : QAbstractListModel(parent)
    , sessions(sessions)
{
    this->rebuildTimer.setSingleShot(true);
    this->rebuildTimer.setInterval(150);
    connect(&this->rebuildTimer, &QTimer::timeout, this, &SessionListModel::Rebuild);
    connect(sessions, &SessionManager::sessionsChanged, &this->rebuildTimer, qOverload<>(&QTimer::start));
    connect(sessions, &SessionManager::currentChanged, &this->rebuildTimer, qOverload<>(&QTimer::start));
    this->rows = BuildRows();
}

void SessionListModel::SetQuery(const QString& query)
{
    if(query == this->query)
    {
        return;
    }
    this->query = query;
    emit QueryChanged();
    this->rebuildTimer.start();
}

QList<SessionListModel::Row> SessionListModel::BuildRows() const
{
    QList<Row> out;
    const QString trimmed = this->query.trimmed();
    if(trimmed.isEmpty())
    {
        for(ChatSession* session : this->sessions->sessions())
        {
            out.append({ session->id(), session->id(), session->title(), PreviewOf(session), session->updatedAt() });
        }
        return out;
    }
    for(const SessionManager::SearchHit& hit : this->sessions->search(trimmed))
    {
        const QString key = QStringLiteral("%1:%2").arg(hit.session->id()).arg(hit.messageId);
        out.append({ key, hit.session->id(), hit.session->title(), hit.excerpt, hit.session->updatedAt() });
    }
    return out;
}

void SessionListModel::Rebuild()
{
    const QList<Row> next = BuildRows();

    // 1) 없어진 행 빼기
    for(int i = int(this->rows.size()) - 1; i >= 0; --i)
    {
        const QString& key = this->rows.at(i).key;
        const bool isKept = std::any_of(next.cbegin(), next.cend(), [&key](const Row& row) { return row.key == key; });
        if(!isKept)
        {
            beginRemoveRows(QModelIndex(), i, i);
            this->rows.removeAt(i);
            endRemoveRows();
        }
    }
    // 2) 순서대로 옮기거나 넣기
    for(int i = 0; i < next.size(); ++i)
    {
        const QString& key = next.at(i).key;
        int from = -1;
        for(int j = i; j < this->rows.size(); ++j)
        {
            if(this->rows.at(j).key == key)
            {
                from = j;
                break;
            }
        }
        if(from == i)
        {
            this->rows[i] = next.at(i);
        }
        else if(from > i)
        {
            beginMoveRows(QModelIndex(), from, from, QModelIndex(), i);
            this->rows.move(from, i);
            endMoveRows();
            this->rows[i] = next.at(i);
        }
        else
        {
            beginInsertRows(QModelIndex(), i, i);
            this->rows.insert(i, next.at(i));
            endInsertRows();
        }
    }
    if(!this->rows.isEmpty())
    {
        emit dataChanged(index(0), index(int(this->rows.size()) - 1));
    }
}

int SessionListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : int(this->rows.size());
}

QVariant SessionListModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid() || index.row() >= this->rows.size())
    {
        return {};
    }
    const Row& row = this->rows.at(index.row());
    switch(static_cast<Role>(role))
    {
    case Role::SESSION_ID:
        return row.sessionId;
    case Role::TITLE:
        return row.title;
    case Role::PREVIEW:
        return row.preview.isEmpty() ? Text("sessions.empty") : row.preview;
    case Role::TIME_TEXT:
        return RelativeTime(row.updatedAt);
    case Role::IS_CURRENT:
        return this->sessions->current() && this->sessions->current()->id() == row.sessionId;
    }
    return {};
}

QHash<int, QByteArray> SessionListModel::roleNames() const
{
    return {
        { int(Role::SESSION_ID), "sessionId" },
        { int(Role::TITLE), "title" },
        { int(Role::PREVIEW), "preview" },
        { int(Role::TIME_TEXT), "timeText" },
        { int(Role::IS_CURRENT), "isCurrent" },
    };
}

QString SessionListModel::RelativeTime(qint64 timestamp)
{
    const qint64 seconds = (QDateTime::currentMSecsSinceEpoch() - timestamp) / 1000;
    if(seconds < 60)
    {
        return Text("time.justNow");
    }
    if(seconds < 3600)
    {
        return Text("time.minutesAgo").arg(seconds / 60);
    }
    if(seconds < 86400)
    {
        return Text("time.hoursAgo").arg(seconds / 3600);
    }
    if(seconds < 2 * 86400)
    {
        return Text("time.yesterday");
    }
    return QDateTime::fromMSecsSinceEpoch(timestamp).toString(QStringLiteral("M/d"));
}
