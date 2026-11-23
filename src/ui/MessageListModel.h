#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QPointer>
#include <QtQml/qqmlregistration.h>

#include "core/ChatMessage.h"

class ChatSession;

// 현재 세션의 메시지 목록 (QML ListView 용).
// 채팅 목록은 아래에서 위로 쌓으므로(BottomToTop) 0번 행이 가장 최근 메시지다.
class MessageListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ANONYMOUS

public:
    enum class Role
    {
        MESSAGE_ID = Qt::UserRole + 1,
        SENDER,
        AUTHOR,
        AUTHOR_COLOR,
        BODY,
        PREVIEW_TEXT,
        IS_USER,
        IS_SYSTEM,
        KIND,
        TIME_TEXT,
        FULL_TIME,
        MODEL_LABEL,
        IS_BOOSTED,
        EMOTICON_SOURCE,
        REPLY_AUTHOR,
        REPLY_EXCERPT,
        REACTIONS
    };

    explicit MessageListModel(QObject* parent = nullptr);

    void SetSession(ChatSession* session, const QString& userName);
    void SetUserName(const QString& userName);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    void AppendMessage(const ChatMessage& message);
    void UpdateMessage(const ChatMessage& message);
    const ChatMessage* MessageAt(int row) const;
    QVariantList ReactionsOf(const ChatMessage& message) const;

    QPointer<ChatSession> session;
    QString userName;
    QList<QMetaObject::Connection> connections;
    // 세션 목록은 signal 전에 이미 늘어나 있으므로, 모델이 보여 주는 행 수를 따로 센다.
    int rows = 0;
};
