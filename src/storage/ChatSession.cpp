#include "storage/ChatSession.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QUuid>

#include "core/Member.h"
#include "manager/DataManager.h"

namespace {

constexpr int maxNoteChars = 4000;

qint64 nowMs() { return QDateTime::currentMSecsSinceEpoch(); }

QJsonObject readJsonFile(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return {};
    return QJsonDocument::fromJson(f.readAll()).object();
}

void writeTextFile(const QString& path, const QString& text)
{
    QSaveFile f(path);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(text.toUtf8());
        f.commit();
    }
}

}

ChatSession::ChatSession(const QString& dir, QObject* parent)
    : QObject(parent)
    , directory(dir)
{
    QDir(dir).mkpath(QStringLiteral("notes"));
    QDir(dir).mkpath(QStringLiteral("logs"));
    loadMeta();
    loadMessages();
    if (const ChatMessage* last = lastMessage())
        this->lastUpdateTime = qMax(this->lastUpdateTime, last->ts);
}

ChatSession* ChatSession::create(const QString& parentDir, const QString& title, const QString& speed,
                                 const QString& boostMode, QObject* parent)
{
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString dir = QDir(parentDir).filePath(id);
    QDir().mkpath(dir);
    const qint64 now = nowMs();
    QJsonObject meta {
        { "id", id },
        { "title", title },
        { "createdAt", now },
        { "updatedAt", now },
        { "speed", speed },
        { "boostMode", boostMode },
    };
    QSaveFile f(QDir(dir).filePath(QStringLiteral("session.json")));
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(meta).toJson(QJsonDocument::Indented));
        f.commit();
    }
    return new ChatSession(dir, parent);
}

// ---------- meta ----------

void ChatSession::loadMeta()
{
    const QJsonObject o = readJsonFile(QDir(this->directory).filePath(QStringLiteral("session.json")));
    this->sessionId = o.value(QLatin1String("id")).toString(QFileInfo(this->directory).fileName());
    this->sessionTitle = o.value(QLatin1String("title")).toString(Text("sessions.new"));
    this->creationTime = o.value(QLatin1String("createdAt")).toInteger(nowMs());
    this->lastUpdateTime = o.value(QLatin1String("updatedAt")).toInteger(this->creationTime);
    this->conversationSpeed = o.value(QLatin1String("speed")).toString(QStringLiteral("normal"));
    this->boostPolicy = o.value(QLatin1String("boostMode")).toString(QStringLiteral("auto"));
    const QJsonObject enabled = o.value(QLatin1String("enabled")).toObject();
    for (auto it = enabled.begin(); it != enabled.end(); ++it)
        this->enabled.insert(it.key(), it.value().toBool(true));
    const QJsonObject seen = o.value(QLatin1String("seen")).toObject();
    for (auto it = seen.begin(); it != seen.end(); ++it)
        this->seen.insert(it.key(), it.value().toInteger());
}

void ChatSession::saveMeta() const
{
    QJsonObject enabled;
    for (auto it = this->enabled.cbegin(); it != this->enabled.cend(); ++it)
        enabled.insert(it.key(), it.value());
    QJsonObject seen;
    for (auto it = this->seen.cbegin(); it != this->seen.cend(); ++it)
        seen.insert(it.key(), it.value());
    const QJsonObject o {
        { "id", this->sessionId },
        { "title", this->sessionTitle },
        { "createdAt", this->creationTime },
        { "updatedAt", this->lastUpdateTime },
        { "speed", this->conversationSpeed },
        { "boostMode", this->boostPolicy },
        { "enabled", enabled },
        { "seen", seen },
    };
    QSaveFile f(QDir(this->directory).filePath(QStringLiteral("session.json")));
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
        f.commit();
    }
}

void ChatSession::setTitle(const QString& title)
{
    const QString t = title.trimmed();
    if (t.isEmpty() || t == this->sessionTitle)
        return;
    this->sessionTitle = t;
    saveMeta();
    emit metaChanged();
}

void ChatSession::setSpeed(const QString& speed)
{
    if (speed == this->conversationSpeed)
        return;
    this->conversationSpeed = speed;
    saveMeta();
    emit metaChanged();
}

void ChatSession::setBoostMode(const QString& mode)
{
    if (mode == this->boostPolicy)
        return;
    this->boostPolicy = mode;
    saveMeta();
    emit metaChanged();
}

bool ChatSession::isMemberEnabled(const QString& memberId) const
{
    return this->enabled.value(memberId, true);
}

void ChatSession::setMemberEnabled(const QString& memberId, bool enabled)
{
    if (isMemberEnabled(memberId) == enabled)
        return;
    this->enabled.insert(memberId, enabled);
    saveMeta();
    emit metaChanged();
}

qint64 ChatSession::seenOf(const QString& memberId) const
{
    // 처음 보는 멤버는 지난 기록을 "이미 읽은" 것으로 시작한다.
    return this->seen.value(memberId, lastId());
}

void ChatSession::setSeen(const QString& memberId, qint64 messageId)
{
    if (this->seen.value(memberId, -1) == messageId)
        return;
    this->seen.insert(memberId, messageId);
    saveMeta();
}

// ---------- messages ----------

void ChatSession::loadMessages()
{
    QFile f(QDir(this->directory).filePath(QStringLiteral("messages.jsonl")));
    if (!f.open(QIODevice::ReadOnly))
        return;
    while (!f.atEnd()) {
        const QByteArray line = f.readLine().trimmed();
        if (line.isEmpty())
            continue;
        const QJsonObject rec = QJsonDocument::fromJson(line).object();
        if (rec.isEmpty())
            continue;
        if (rec.value(QLatin1String("kind")).toString() == QLatin1String("react")) {
            if (ChatMessage* m = mutableById(rec.value(QLatin1String("target")).toInteger())) {
                const QString emoji = rec.value(QLatin1String("emoji")).toString();
                const QString by = rec.value(QLatin1String("by")).toString();
                QStringList& who = m->reactions[emoji];
                if (!who.contains(by))
                    who << by;
            }
            continue;
        }
        const ChatMessage msg = ChatMessage::fromJson(rec);
        this->index.insert(msg.id, int(this->messageHistory.size()));
        this->messageHistory.append(msg);
        this->nextId = qMax(this->nextId, msg.id + 1);
    }
}

void ChatSession::appendRecord(const QJsonObject& record) const
{
    QFile f(QDir(this->directory).filePath(QStringLiteral("messages.jsonl")));
    if (f.open(QIODevice::WriteOnly | QIODevice::Append)) {
        f.write(QJsonDocument(record).toJson(QJsonDocument::Compact));
        f.write("\n");
    }
}

ChatMessage* ChatSession::mutableById(qint64 id)
{
    const auto it = this->index.constFind(id);
    return it == this->index.cend() ? nullptr : &this->messageHistory[it.value()];
}

const ChatMessage* ChatSession::byId(qint64 id) const
{
    const auto it = this->index.constFind(id);
    return it == this->index.cend() ? nullptr : &this->messageHistory.at(it.value());
}

int ChatSession::indexOf(qint64 id) const
{
    return this->index.value(id, -1);
}

const ChatMessage* ChatSession::lastMessage() const
{
    return this->messageHistory.isEmpty() ? nullptr : &this->messageHistory.constLast();
}

QList<ChatMessage> ChatSession::after(qint64 id) const
{
    // id 순서로 쌓이므로 뒤에서부터 훑는다.
    qsizetype i = this->messageHistory.size();
    while (i > 0 && this->messageHistory.at(i - 1).id > id)
        --i;
    return this->messageHistory.mid(i);
}

QList<ChatMessage> ChatSession::recent(int n) const
{
    return n >= this->messageHistory.size() ? this->messageHistory : this->messageHistory.mid(this->messageHistory.size() - n);
}

ChatMessage ChatSession::addMessage(ChatMessage m)
{
    m.id = this->nextId++;
    if (!m.ts)
        m.ts = nowMs();
    this->index.insert(m.id, int(this->messageHistory.size()));
    this->messageHistory.append(m);
    this->lastUpdateTime = m.ts;
    appendRecord(m.toJson());
    emit messageAdded(m);
    return m;
}

bool ChatSession::applyReaction(qint64 targetId, const QString& by, const QString& emoji)
{
    ChatMessage* m = mutableById(targetId);
    if (!m || emoji.isEmpty())
        return false;
    QStringList& who = m->reactions[emoji];
    if (who.contains(by))
        return false;
    who << by;
    appendRecord(QJsonObject {
        { "kind", "react" },
        { "target", targetId },
        { "by", by },
        { "emoji", emoji },
        { "ts", nowMs() },
    });
    emit messageUpdated(*m);
    return true;
}

// ---------- notes ----------

QString ChatSession::notePath(const QString& memberId) const
{
    return QDir(this->directory).filePath(QStringLiteral("notes/%1.md").arg(memberId));
}

QString ChatSession::readNote(const QString& memberId) const
{
    QFile f(notePath(memberId));
    return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()) : QString();
}

void ChatSession::writeNote(const QString& memberId, const QString& text)
{
    writeTextFile(notePath(memberId), text.trimmed().left(maxNoteChars) + QLatin1Char('\n'));
    emit noteChanged(memberId);
}

void ChatSession::appendNote(const QString& memberId, const QString& line)
{
    QString note = readNote(memberId).trimmed();
    note += (note.isEmpty() ? QString() : QStringLiteral("\n")) + QStringLiteral("- ") + line.simplified();
    // 너무 길어지면 오래된 줄부터 버린다. 멤버에게는 정리하라고 따로 알려 준다.
    while (note.size() > maxNoteChars && note.contains(QLatin1Char('\n')))
        note = note.mid(note.indexOf(QLatin1Char('\n')) + 1);
    writeTextFile(notePath(memberId), note + QLatin1Char('\n'));
    emit noteChanged(memberId);
}

void ChatSession::log(const QString& who, const QString& text) const
{
    QFile f(QDir(this->directory).filePath(QStringLiteral("logs/%1.log").arg(who)));
    if (f.open(QIODevice::WriteOnly | QIODevice::Append)) {
        const QString stamp = QDateTime::currentDateTime().toString(Qt::ISODate);
        f.write(QStringLiteral("[%1] %2\n").arg(stamp, text).toUtf8());
    }
}
