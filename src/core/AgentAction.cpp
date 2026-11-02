#include "core/AgentAction.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

namespace {

constexpr int kMaxBubbles = 4;
constexpr int kMaxBubbleChars = 2000;

qint64 toId(const QJsonValue& v)
{
    if (v.isDouble())
        return v.toInteger();
    if (v.isString())
        return v.toString().remove(QLatin1Char('#')).toLongLong();
    return 0;
}

AgentAction fromObject(const QJsonObject& o)
{
    AgentAction a;
    a.action = o.value(QLatin1String("action")).toString();

    QStringList msgs;
    const QJsonValue mv = o.value(QLatin1String("messages"));
    if (mv.isArray()) {
        for (const QJsonValue& v : mv.toArray())
            msgs << v.toString();
    } else if (mv.isString()) {
        msgs << mv.toString();
    }
    for (QString s : msgs) {
        s = s.trimmed();
        if (!s.isEmpty() && a.messages.size() < kMaxBubbles)
            a.messages << s.left(kMaxBubbleChars);
    }
    if (a.action.isEmpty())
        a.action = a.messages.isEmpty() ? QStringLiteral("pass") : QStringLiteral("say");

    a.replyTo = toId(o.value(QLatin1String("reply_to")));
    const QJsonObject react = o.value(QLatin1String("react")).toObject();
    a.reactId = toId(react.value(QLatin1String("id")));
    a.reactEmoji = react.value(QLatin1String("emoji")).toString().left(8);
    a.noteAdd = o.value(QLatin1String("note_add")).toString();
    a.noteReplace = o.value(QLatin1String("note_replace")).toString();
    a.emoticon = o.value(QLatin1String("emoticon")).toString().trimmed();
    a.boost = o.value(QLatin1String("boost")).toString();
    return a;
}

}

std::optional<AgentAction> AgentAction::parse(const QString& raw)
{
    if (raw.trimmed().isEmpty())
        return std::nullopt;
    static const QRegularExpression fence(QStringLiteral("```(?:json)?"), QRegularExpression::CaseInsensitiveOption);
    QString text = raw;
    text.remove(fence);

    // 따옴표 안의 중괄호는 건너뛰면서 짝이 맞는 { ... } 를 찾는다.
    for (qsizetype start = text.indexOf(QLatin1Char('{')); start != -1; start = text.indexOf(QLatin1Char('{'), start + 1)) {
        int depth = 0;
        bool inStr = false;
        bool esc = false;
        for (qsizetype i = start; i < text.size(); ++i) {
            const QChar c = text.at(i);
            if (inStr) {
                if (esc)
                    esc = false;
                else if (c == QLatin1Char('\\'))
                    esc = true;
                else if (c == QLatin1Char('"'))
                    inStr = false;
                continue;
            }
            if (c == QLatin1Char('"')) {
                inStr = true;
            } else if (c == QLatin1Char('{')) {
                ++depth;
            } else if (c == QLatin1Char('}') && --depth == 0) {
                QJsonParseError err;
                const QJsonDocument doc = QJsonDocument::fromJson(text.mid(start, i - start + 1).toUtf8(), &err);
                if (err.error == QJsonParseError::NoError && doc.isObject()) {
                    const QJsonObject o = doc.object();
                    if (o.contains(QLatin1String("action")) || o.contains(QLatin1String("messages")))
                        return fromObject(o);
                }
                break;
            }
        }
    }
    return std::nullopt;
}
