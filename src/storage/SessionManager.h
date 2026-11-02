#pragma once

#include <QList>
#include <QObject>
#include <QPointer>

class ChatSession;
struct AppConfig;

// 세션 목록 관리: 불러오기·만들기·이름 바꾸기·삭제(휴지통으로 이동)·검색, 현재 세션 선택.
class SessionManager : public QObject {
    Q_OBJECT
public:
    SessionManager(const QString& root, const AppConfig* config, QObject* parent = nullptr);

    void load();

    QList<ChatSession*> sessions() const; // 최근 활동 순
    ChatSession* current() const { return this->currentSession; }
    ChatSession* find(const QString& id) const;

    ChatSession* createSession(const QString& title = {});
    void setCurrent(ChatSession* session);
    void renameSession(ChatSession* session, const QString& title);
    void removeSession(ChatSession* session);

    struct SearchHit {
        ChatSession* session = nullptr;
        qint64 messageId = 0;
        QString excerpt;
    };
    QList<SearchHit> search(const QString& query, int limit = 60) const;

signals:
    void sessionsChanged();
    void currentChanged(ChatSession* session);

private:
    void adopt(ChatSession* session);

    QString root;
    const AppConfig* config;
    QList<ChatSession*> loadedSessions;
    QPointer<ChatSession> currentSession;
};
