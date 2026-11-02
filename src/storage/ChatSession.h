#pragma once

#include <QHash>
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>

#include "core/ChatMessage.h"

// 대화방(세션) 하나. 폴더 하나에 저장된다.
//   session.json     제목, 방 설정(속도·진심모드·참여 멤버), 멤버별 읽은 위치
//   messages.jsonl   메시지 (추가만 하는 로그. 반응은 별도 레코드로 덧붙인다)
//   notes/<id>.md    멤버 개인 메모
//   logs/<id>.log    CLI 호출 기록 (디버깅용)
class ChatSession : public QObject {
    Q_OBJECT
public:
    explicit ChatSession(const QString& dir, QObject* parent = nullptr);
    static ChatSession* create(const QString& parentDir, const QString& title, const QString& speed,
                               const QString& boostMode, QObject* parent = nullptr);

    QString id() const { return m_id; }
    QString dir() const { return m_dir; }
    QString title() const { return m_title; }
    void setTitle(const QString& title);
    qint64 createdAt() const { return m_createdAt; }
    qint64 updatedAt() const { return m_updatedAt; }

    // 방 설정
    QString speed() const { return m_speed; }
    void setSpeed(const QString& speed);
    QString boostMode() const { return m_boostMode; }
    void setBoostMode(const QString& mode);
    bool isMemberEnabled(const QString& memberId) const;
    void setMemberEnabled(const QString& memberId, bool enabled);
    qint64 seenOf(const QString& memberId) const;
    void setSeen(const QString& memberId, qint64 messageId);

    // 메시지
    const QList<ChatMessage>& messages() const { return m_messages; }
    const ChatMessage* byId(qint64 id) const;
    int indexOf(qint64 id) const;
    qint64 lastId() const { return m_nextId - 1; }
    const ChatMessage* lastMessage() const;
    QList<ChatMessage> after(qint64 id) const;
    QList<ChatMessage> recent(int n) const;

    ChatMessage addMessage(ChatMessage m);
    // 같은 사람이 같은 이모지는 한 번만.
    bool applyReaction(qint64 targetId, const QString& by, const QString& emoji);

    // 개인 메모
    QString readNote(const QString& memberId) const;
    void writeNote(const QString& memberId, const QString& text);
    void appendNote(const QString& memberId, const QString& line);

    void log(const QString& who, const QString& text) const;

signals:
    void messageAdded(const ChatMessage& message);
    void messageUpdated(const ChatMessage& message);
    void noteChanged(const QString& memberId);
    void metaChanged();

private:
    void loadMeta();
    void saveMeta() const;
    void loadMessages();
    void appendRecord(const QJsonObject& record) const;
    ChatMessage* mutableById(qint64 id);
    QString notePath(const QString& memberId) const;

    QString m_dir;
    QString m_id;
    QString m_title;
    qint64 m_createdAt = 0;
    qint64 m_updatedAt = 0;
    QString m_speed;
    QString m_boostMode;
    QMap<QString, bool> m_enabled;
    QMap<QString, qint64> m_seen;

    QList<ChatMessage> m_messages;
    QHash<qint64, int> m_index;
    qint64 m_nextId = 1;
};
