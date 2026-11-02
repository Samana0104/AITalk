#include "core/Member.h"

#include <QRegularExpression>

#include "util/DataManager.h"

namespace {

const QList<MemberInfo>& registry()
{
    static const QList<MemberInfo> list = {
        { "claude", "Claude", "Anthropic", QColor::fromRgb(0xd97a3a), QColor::fromRgb(0xf7d9c8), "claude", { "claude", "클로드" }, false },
        { "gpt", "ChatGPT", "OpenAI", QColor::fromRgb(0x2f7cf6), QColor::fromRgb(0xd8f3ea), "codex", { "chatgpt", "gpt", "지피티", "챗지피티" }, false },
        { "gemini", "Gemini", "Google", QColor::fromRgb(0x5b6cf0), QColor::fromRgb(0xdceaff), "agy", { "gemini", "제미나이", "제미니" }, false },
    };
    return list;
}

// 라틴 별명은 단어로 떨어져 있어야 한다 ("gpt" 는 "gpts" 가 아니다).
// 한국어는 조사가 붙으니 아무 데나 들어 있으면 된다.
bool hasAlias(const QString& lowerText, const QString& alias)
{
    static const QRegularExpression latin(QStringLiteral("^[a-z0-9 ]+$"));
    if (!latin.match(alias).hasMatch())
        return lowerText.contains(alias);
    const QRegularExpression re(QStringLiteral("(^|[^a-z0-9])%1($|[^a-z0-9])").arg(QRegularExpression::escape(alias)));
    return re.match(lowerText).hasMatch();
}

bool endsWithBatchim(const QString& word)
{
    if (word.isEmpty())
        return false;
    const char16_t c = word.back().unicode();
    return c >= 0xAC00 && c <= 0xD7A3 && (c - 0xAC00) % 28 != 0;
}

}

namespace Members {

const QList<MemberInfo>& all() { return registry(); }

QStringList ids()
{
    QStringList out;
    for (const MemberInfo& m : registry())
        out << m.id;
    return out;
}

const MemberInfo* find(const QString& id)
{
    for (const MemberInfo& m : registry())
        if (m.id == id)
            return &m;
    return nullptr;
}

bool isAgent(const QString& id) { return find(id) != nullptr; }

QString displayName(const QString& id, const QString& userName)
{
    if (id == User)
        return userName.isEmpty() ? Text("config.defaultUserName") : userName;
    if (id == System)
        return Text("member.system");
    if (const MemberInfo* m = find(id))
        return m->name;
    return id;
}

QColor colorOf(const QString& id)
{
    if (const MemberInfo* m = find(id))
        return m->color;
    if (id == User)
        return QColor::fromRgb(0x0f172a);
    return QColor::fromRgb(0x71717a);
}

bool mentions(const QString& text, const QString& id)
{
    const MemberInfo* m = find(id);
    if (!m)
        return false;
    const QString t = text.toLower();
    for (const QString& a : m->aliases)
        if (t.contains(QLatin1Char('@') + a) || hasAlias(t, a))
            return true;
    return false;
}

bool atMentions(const QString& text, const QString& id)
{
    const MemberInfo* m = find(id);
    if (!m)
        return false;
    const QString t = text.toLower();
    for (const QString& a : m->aliases)
        if (t.contains(QLatin1Char('@') + a))
            return true;
    return false;
}

QString withJosa(const QString& id, const QString& userName, const QString& pair)
{
    const QStringList p = pair.split(QLatin1Char('/'));
    const QString word = displayName(id, userName);
    const MemberInfo* m = find(id);
    const bool fin = m ? m->finalConsonant : endsWithBatchim(word);
    return word + (fin ? p.value(0) : p.value(1));
}

}
