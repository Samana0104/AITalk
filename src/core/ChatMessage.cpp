#include "core/ChatMessage.h"

#include <QJsonArray>

#include "core/Member.h"

bool ChatMessage::isUser() const { return from == Members::User; }
bool ChatMessage::isSystem() const { return from == Members::System; }
bool ChatMessage::isAgent() const { return Members::isAgent(from); }

QString ChatMessage::displayText() const
{
    if (emoticon.isEmpty())
        return text;
    const QString tag = QStringLiteral("[이모티콘: %1]").arg(emoticon);
    return text.isEmpty() ? tag : text + QLatin1Char(' ') + tag;
}

QJsonObject ChatMessage::toJson() const
{
    QJsonObject o {
        { "id", id },
        { "ts", ts },
        { "from", from },
        { "text", text },
    };
    if (replyTo)
        o.insert(QStringLiteral("replyTo"), replyTo);
    if (!kind.isEmpty())
        o.insert(QStringLiteral("kind"), kind);
    if (!model.isEmpty())
        o.insert(QStringLiteral("model"), model);
    if (boosted)
        o.insert(QStringLiteral("boosted"), true);
    if (!emoticon.isEmpty())
        o.insert(QStringLiteral("emoticon"), emoticon);
    if (!reactions.isEmpty()) {
        QJsonObject r;
        for (auto it = reactions.cbegin(); it != reactions.cend(); ++it)
            r.insert(it.key(), QJsonArray::fromStringList(it.value()));
        o.insert(QStringLiteral("reactions"), r);
    }
    return o;
}

ChatMessage ChatMessage::fromJson(const QJsonObject& o)
{
    ChatMessage m;
    m.id = o.value(QLatin1String("id")).toInteger();
    m.ts = o.value(QLatin1String("ts")).toInteger();
    m.from = o.value(QLatin1String("from")).toString();
    m.text = o.value(QLatin1String("text")).toString();
    m.replyTo = o.value(QLatin1String("replyTo")).toInteger();
    m.kind = o.value(QLatin1String("kind")).toString();
    m.model = o.value(QLatin1String("model")).toString();
    m.boosted = o.value(QLatin1String("boosted")).toBool();
    m.emoticon = o.value(QLatin1String("emoticon")).toString();
    const QJsonObject r = o.value(QLatin1String("reactions")).toObject();
    for (auto it = r.begin(); it != r.end(); ++it) {
        QStringList who;
        for (const QJsonValue& v : it.value().toArray())
            who << v.toString();
        m.reactions.insert(it.key(), who);
    }
    return m;
}
