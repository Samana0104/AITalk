#include "storage/SessionManager.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <algorithm>

#include "app/AppPaths.h"
#include "core/AppConfig.h"
#include "storage/ChatSession.h"

SessionManager::SessionManager(const QString& root, const AppConfig* config, QObject* parent)
    : QObject(parent)
    , root(root)
    , config(config)
{
}

void SessionManager::load()
{
    const QDir dir(this->root);
    for (const QString& name : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (!QFileInfo::exists(dir.filePath(name + QStringLiteral("/session.json"))))
            continue;
        adopt(new ChatSession(dir.filePath(name), this));
    }
    const QList<ChatSession*> list = sessions();
    setCurrent(list.isEmpty() ? createSession() : list.first());
}

QList<ChatSession*> SessionManager::sessions() const
{
    QList<ChatSession*> out = this->loadedSessions;
    std::sort(out.begin(), out.end(), [](ChatSession* a, ChatSession* b) {
        return a->updatedAt() > b->updatedAt();
    });
    return out;
}

ChatSession* SessionManager::find(const QString& id) const
{
    for (ChatSession* s : this->loadedSessions)
        if (s->id() == id)
            return s;
    return nullptr;
}

ChatSession* SessionManager::createSession(const QString& title)
{
    QString t = title.trimmed();
    if (t.isEmpty())
        t = this->loadedSessions.isEmpty() ? this->config->defaultRoomName
                                 : QStringLiteral("%1 %2").arg(this->config->defaultRoomName).arg(this->loadedSessions.size() + 1);
    ChatSession* s = ChatSession::create(this->root, t, this->config->speed, this->config->boostMode, this);
    adopt(s);
    emit sessionsChanged();
    return s;
}

void SessionManager::adopt(ChatSession* session)
{
    this->loadedSessions.append(session);
    connect(session, &ChatSession::messageAdded, this, &SessionManager::sessionsChanged);
    connect(session, &ChatSession::metaChanged, this, &SessionManager::sessionsChanged);
}

void SessionManager::setCurrent(ChatSession* session)
{
    if (!session || session == this->currentSession)
        return;
    this->currentSession = session;
    emit currentChanged(session);
}

void SessionManager::renameSession(ChatSession* session, const QString& title)
{
    if (session)
        session->setTitle(title);
}

void SessionManager::removeSession(ChatSession* session)
{
    if (!session || !this->loadedSessions.contains(session))
        return;
    // 지우기 전에 현재 세션을 다른 곳으로 옮겨서 엔진이 먼저 떨어지게 한다.
    if (session == this->currentSession) {
        ChatSession* next = nullptr;
        for (ChatSession* s : sessions())
            if (s != session) {
                next = s;
                break;
            }
        setCurrent(next ? next : createSession());
    }
    this->loadedSessions.removeAll(session);
    const QString dir = session->dir();
    session->deleteLater();

    const QString target = QDir(AppPaths::trashDir())
                               .filePath(QStringLiteral("%1-%2").arg(QFileInfo(dir).fileName())
                                                                .arg(QDateTime::currentMSecsSinceEpoch()));
    if (!QDir().rename(dir, target))
        QDir(dir).removeRecursively();
    emit sessionsChanged();
}

QList<SessionManager::SearchHit> SessionManager::search(const QString& query, int limit) const
{
    QList<SearchHit> hits;
    const QString q = query.trimmed();
    if (q.isEmpty())
        return hits;
    for (ChatSession* s : sessions()) {
        if (s->title().contains(q, Qt::CaseInsensitive))
            hits.append({ s, 0, s->title() });
        const QList<ChatMessage>& msgs = s->messages();
        for (qsizetype i = msgs.size() - 1; i >= 0 && hits.size() < limit; --i) {
            const ChatMessage& m = msgs.at(i);
            if (m.isSystem())
                continue;
            const qsizetype at = m.text.indexOf(q, 0, Qt::CaseInsensitive);
            if (at < 0)
                continue;
            const qsizetype from = qMax<qsizetype>(0, at - 20);
            QString excerpt = m.text.mid(from, 80).simplified();
            if (from > 0)
                excerpt.prepend(QStringLiteral("…"));
            hits.append({ s, m.id, excerpt });
        }
        if (hits.size() >= limit)
            break;
    }
    return hits;
}
