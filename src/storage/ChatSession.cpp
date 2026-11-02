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
#include "util/DataManager.h"

namespace {

constexpr int kMaxNoteChars = 4000;

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
    , m_dir(dir)
{
    QDir(dir).mkpath(QStringLiteral("notes"));
    QDir(dir).mkpath(QStringLiteral("logs"));
    loadMeta();
    loadMessages();
    if (const ChatMessage* last = lastMessage())
        m_updatedAt = qMax(m_updatedAt, last->ts);
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
    const QJsonObject o = readJsonFile(QDir(m_dir).filePath(QStringLiteral("session.json")));
    m_id = o.value(QLatin1String("id")).toString(QFileInfo(m_dir).fileName());
    m_title = o.value(QLatin1String("title")).toString(Text("sessions.new"));
    m_createdAt = o.value(QLatin1String("createdAt")).toInteger(nowMs());
    m_updatedAt = o.value(QLatin1String("updatedAt")).toInteger(m_createdAt);
    m_speed = o.value(QLatin1String("speed")).toString(QStringLiteral("normal"));
    m_boostMode = o.value(QLatin1String("boostMode")).toString(QStringLiteral("auto"));
    const QJsonObject enabled = o.value(QLatin1String("enabled")).toObject();
    for (auto it = enabled.begin(); it != enabled.end(); ++it)
        m_enabled.insert(it.key(), it.value().toBool(true));
    const QJsonObject seen = o.value(QLatin1String("seen")).toObject();
    for (auto it = seen.begin(); it != seen.end(); ++it)
        m_seen.insert(it.key(), it.value().toInteger());
}

void ChatSession::saveMeta() const
{
    QJsonObject enabled;
    for (auto it = m_enabled.cbegin(); it != m_enabled.cend(); ++it)
        enabled.insert(it.key(), it.value());
    QJsonObject seen;
    for (auto it = m_seen.cbegin(); it != m_seen.cend(); ++it)
        seen.insert(it.key(), it.value());
    const QJsonObject o {
        { "id", m_id },
        { "title", m_title },
        { "createdAt", m_createdAt },
        { "updatedAt", m_updatedAt },
        { "speed", m_speed },
        { "boostMode", m_boostMode },
        { "enabled", enabled },
        { "seen", seen },
    };
    QSaveFile f(QDir(m_dir).filePath(QStringLiteral("session.json")));
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
        f.commit();
    }
}

void ChatSession::setTitle(const QString& title)
{
    const QString t = title.trimmed();
    if (t.isEmpty() || t == m_title)
        return;
    m_title = t;
    saveMeta();
    emit metaChanged();
}

void ChatSession::setSpeed(const QString& speed)
{
    if (speed == m_speed)
        return;
    m_speed = speed;
    saveMeta();
    emit metaChanged();
}

void ChatSession::setBoostMode(const QString& mode)
{
    if (mode == m_boostMode)
        return;
    m_boostMode = mode;
    saveMeta();
    emit metaChanged();
}

bool ChatSession::isMemberEnabled(const QString& memberId) const
{
    return m_enabled.value(memberId, true);
}

void ChatSession::setMemberEnabled(const QString& memberId, bool enabled)
{
    if (isMemberEnabled(memberId) == enabled)
        return;
    m_enabled.insert(memberId, enabled);
    saveMeta();
    emit metaChanged();
}

qint64 ChatSession::seenOf(const QString& memberId) const
{
    // 처음 보는 멤버는 지난 기록을 "이미 읽은" 것으로 시작한다.
    return m_seen.value(memberId, lastId());
}

void ChatSession::setSeen(const QString& memberId, qint64 messageId)
{
    if (m_seen.value(memberId, -1) == messageId)
        return;
    m_seen.insert(memberId, messageId);
    saveMeta();
}

// ---------- messages ----------

void ChatSession::loadMessages()
{
    QFile f(QDir(m_dir).filePath(QStringLiteral("messages.jsonl")));
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
        m_index.insert(msg.id, int(m_messages.size()));
        m_messages.append(msg);
        m_nextId = qMax(m_nextId, msg.id + 1);
    }
}

void ChatSession::appendRecord(const QJsonObject& record) const
{
    QFile f(QDir(m_dir).filePath(QStringLiteral("messages.jsonl")));
    if (f.open(QIODevice::WriteOnly | QIODevice::Append)) {
        f.write(QJsonDocument(record).toJson(QJsonDocument::Compact));
        f.write("\n");
    }
}

ChatMessage* ChatSession::mutableById(qint64 id)
{
    const auto it = m_index.constFind(id);
    return it == m_index.cend() ? nullptr : &m_messages[it.value()];
}

const ChatMessage* ChatSession::byId(qint64 id) const
{
    const auto it = m_index.constFind(id);
    return it == m_index.cend() ? nullptr : &m_messages.at(it.value());
}

int ChatSession::indexOf(qint64 id) const
{
    return m_index.value(id, -1);
}

const ChatMessage* ChatSession::lastMessage() const
{
    return m_messages.isEmpty() ? nullptr : &m_messages.constLast();
}

QList<ChatMessage> ChatSession::after(qint64 id) const
{
    // id 순서로 쌓이므로 뒤에서부터 훑는다.
    qsizetype i = m_messages.size();
    while (i > 0 && m_messages.at(i - 1).id > id)
        --i;
    return m_messages.mid(i);
}

QList<ChatMessage> ChatSession::recent(int n) const
{
    return n >= m_messages.size() ? m_messages : m_messages.mid(m_messages.size() - n);
}

ChatMessage ChatSession::addMessage(ChatMessage m)
{
    m.id = m_nextId++;
    if (!m.ts)
        m.ts = nowMs();
    m_index.insert(m.id, int(m_messages.size()));
    m_messages.append(m);
    m_updatedAt = m.ts;
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
    return QDir(m_dir).filePath(QStringLiteral("notes/%1.md").arg(memberId));
}

QString ChatSession::readNote(const QString& memberId) const
{
    QFile f(notePath(memberId));
    return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()) : QString();
}

void ChatSession::writeNote(const QString& memberId, const QString& text)
{
    writeTextFile(notePath(memberId), text.trimmed().left(kMaxNoteChars) + QLatin1Char('\n'));
    emit noteChanged(memberId);
}

void ChatSession::appendNote(const QString& memberId, const QString& line)
{
    QString note = readNote(memberId).trimmed();
    note += (note.isEmpty() ? QString() : QStringLiteral("\n")) + QStringLiteral("- ") + line.simplified();
    // 너무 길어지면 오래된 줄부터 버린다. 멤버에게는 정리하라고 따로 알려 준다.
    while (note.size() > kMaxNoteChars && note.contains(QLatin1Char('\n')))
        note = note.mid(note.indexOf(QLatin1Char('\n')) + 1);
    writeTextFile(notePath(memberId), note + QLatin1Char('\n'));
    emit noteChanged(memberId);
}

void ChatSession::log(const QString& who, const QString& text) const
{
    QFile f(QDir(m_dir).filePath(QStringLiteral("logs/%1.log").arg(who)));
    if (f.open(QIODevice::WriteOnly | QIODevice::Append)) {
        const QString stamp = QDateTime::currentDateTime().toString(Qt::ISODate);
        f.write(QStringLiteral("[%1] %2\n").arg(stamp, text).toUtf8());
    }
}
