#include "ui/MessageListModel.h"

#include <QDateTime>
#include <QRegularExpression>

#include "core/Emoticons.h"
#include "core/Member.h"
#include "storage/ChatSession.h"

MessageListModel::MessageListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

void MessageListModel::SetSession(ChatSession* session, const QString& userName)
{
    beginResetModel();
    for(const QMetaObject::Connection& connection : std::as_const(this->connections))
    {
        disconnect(connection);
    }
    this->connections.clear();
    this->session = session;
    this->userName = userName;
    this->rows = session ? int(session->messages().size()) : 0;
    if(session)
    {
        this->connections << connect(session, &ChatSession::messageAdded, this, &MessageListModel::AppendMessage);
        this->connections << connect(session, &ChatSession::messageUpdated, this, &MessageListModel::UpdateMessage);
    }
    endResetModel();
}

void MessageListModel::SetUserName(const QString& userName)
{
    if(userName == this->userName)
    {
        return;
    }
    this->userName = userName;
    if(this->rows > 0)
    {
        emit dataChanged(index(0), index(this->rows - 1));
    }
}

int MessageListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : this->rows;
}

const ChatMessage* MessageListModel::MessageAt(int row) const
{
    if(!this->session || row < 0 || row >= this->rows)
    {
        return nullptr;
    }
    return &this->session->messages().at(this->rows - 1 - row);
}

QVariant MessageListModel::data(const QModelIndex& index, int role) const
{
    const ChatMessage* message = MessageAt(index.row());
    if(!message)
    {
        return {};
    }
    switch(static_cast<Role>(role))
    {
    case Role::MESSAGE_ID:
        return message->id;
    case Role::SENDER:
        return message->from;
    case Role::AUTHOR:
        return Members::displayName(message->from, this->userName);
    case Role::AUTHOR_COLOR:
        return Members::colorOf(message->from);
    case Role::BODY:
        return message->text;
    case Role::PREVIEW_TEXT:
        return message->displayText().simplified();
    case Role::IS_USER:
        return message->isUser();
    case Role::IS_SYSTEM:
        return message->isSystem();
    case Role::KIND:
        return message->kind;
    case Role::TIME_TEXT:
        return QDateTime::fromMSecsSinceEpoch(message->ts).toString(QStringLiteral("HH:mm"));
    case Role::FULL_TIME:
        return QDateTime::fromMSecsSinceEpoch(message->ts).toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    case Role::MODEL_LABEL:
        return message->model;
    case Role::IS_BOOSTED:
        return message->boosted;
    case Role::EMOTICON_SOURCE:
    {
        if(message->emoticon.isEmpty())
        {
            return QString();
        }
        const QString path = Emoticons::resourcePath(message->from, message->emoticon);
        return path.isEmpty() ? QString() : QStringLiteral("qrc") + path;
    }
    case Role::REPLY_AUTHOR:
    {
        const ChatMessage* target = this->session->byId(message->replyTo);
        return target ? Members::displayName(target->from, this->userName) : QString();
    }
    case Role::REPLY_EXCERPT:
    {
        static const QRegularExpression markdownMarks(QStringLiteral("[*_`#>~]+"));
        const ChatMessage* target = this->session->byId(message->replyTo);
        return target ? target->displayText().remove(markdownMarks).simplified().left(60) : QString();
    }
    case Role::REACTIONS:
        return ReactionsOf(*message);
    }
    return {};
}

QVariantList MessageListModel::ReactionsOf(const ChatMessage& message) const
{
    QVariantList out;
    for(auto it = message.reactions.cbegin(); it != message.reactions.cend(); ++it)
    {
        if(it.value().isEmpty())
        {
            continue;
        }
        QStringList names;
        for(const QString& who : it.value())
        {
            names << Members::displayName(who, this->userName);
        }
        out.append(QVariantMap {
            { QStringLiteral("emoji"), it.key() },
            { QStringLiteral("count"), it.value().size() },
            { QStringLiteral("isMine"), it.value().contains(Members::User) },
            { QStringLiteral("names"), names.join(QStringLiteral(", ")) },
        });
    }
    return out;
}

QHash<int, QByteArray> MessageListModel::roleNames() const
{
    return {
        { int(Role::MESSAGE_ID), "messageId" },
        { int(Role::SENDER), "sender" },
        { int(Role::AUTHOR), "author" },
        { int(Role::AUTHOR_COLOR), "authorColor" },
        { int(Role::BODY), "body" },
        { int(Role::PREVIEW_TEXT), "previewText" },
        { int(Role::IS_USER), "isUser" },
        { int(Role::IS_SYSTEM), "isSystem" },
        { int(Role::KIND), "kind" },
        { int(Role::TIME_TEXT), "timeText" },
        { int(Role::FULL_TIME), "fullTime" },
        { int(Role::MODEL_LABEL), "modelLabel" },
        { int(Role::IS_BOOSTED), "isBoosted" },
        { int(Role::EMOTICON_SOURCE), "emoticonSource" },
        { int(Role::REPLY_AUTHOR), "replyAuthor" },
        { int(Role::REPLY_EXCERPT), "replyExcerpt" },
        { int(Role::REACTIONS), "reactions" },
    };
}

void MessageListModel::AppendMessage(const ChatMessage& message)
{
    Q_UNUSED(message);
    if(!this->session || this->rows >= this->session->messages().size())
    {
        return;
    }
    beginInsertRows(QModelIndex(), 0, 0);
    ++this->rows;
    endInsertRows();
}

void MessageListModel::UpdateMessage(const ChatMessage& message)
{
    if(!this->session)
    {
        return;
    }
    const int at = this->session->indexOf(message.id);
    if(at < 0 || at >= this->rows)
    {
        return;
    }
    const QModelIndex changed = index(this->rows - 1 - at);
    emit dataChanged(changed, changed);
}
