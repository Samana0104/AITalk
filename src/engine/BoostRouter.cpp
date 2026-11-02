#include "engine/BoostRouter.h"

#include <QDateTime>
#include <QRegularExpression>

#include "core/AppConfig.h"
#include "core/Member.h"
#include "storage/ChatSession.h"

namespace {

using Patterns = QList<QRegularExpression>;
constexpr auto CI = QRegularExpression::CaseInsensitiveOption;

// [한국어, 영어]
const Patterns& serious()
{
    static const Patterns p {
        QRegularExpression(QStringLiteral(R"((진지하게|진지한\s*(답|대답|얘기)|진지\s*모드|진심\s*모드|각\s*잡고|각\s*잡아|제대로\s*(생각|답|따져|분석|검토|설계|풀어|해\s*봐|해줘)|깊게\s*생각|신중하게|꼼꼼하게\s*(봐|따져|생각)))")),
        QRegularExpression(QStringLiteral(R"(\b(seriously|be serious|get serious|for real|boost mode|think (hard|carefully|it through|deeply)|deep dive|in depth|thoroughly|properly (think|answer|analy[sz]e|review|design))\b)"), CI),
    };
    return p;
}

const Patterns& request()
{
    static const Patterns p {
        QRegularExpression(QStringLiteral(R"((해\s*줘|해줄래|해\s*봐|해봐|해\s*볼래|줘|줄래|할\s*수\s*있어|가능해|부탁|알려|설명|풀어|계산|\?|？))")),
        QRegularExpression(QStringLiteral(R"((\b(can you|could you|would you|will you|please|pls|help me|tell me|explain|show me|walk me through|solve|calculate)\b|\?))"), CI),
    };
    return p;
}

const Patterns& code()
{
    static const Patterns p {
        QRegularExpression(QStringLiteral(R"((코드|코딩|디버깅|디버그|버그\s*(좀|찾|고쳐)|리팩터|리팩토링|스크립트|알고리즘|함수|정규식|쿼리|구현|자바스크립트|파이썬))"), CI),
        QRegularExpression(QStringLiteral(R"(\b(code|coding|debug|debugging|bugs?|refactor|script|algorithm|function|regex|query|implement|javascript|python|typescript|c\+\+|sql)\b)"), CI),
    };
    return p;
}

const Patterns& math()
{
    static const Patterns p {
        QRegularExpression(QStringLiteral(R"((\d+(?:\.\d+)?\s*[+\-*/×÷^]\s*\d+(?:\.\d+)?\s*[+\-*/×÷^]\s*\d|확률|기댓값|방정식|적분|미분|증명|최적화|경우의\s*수|시간\s*복잡도|계산해))")),
        QRegularExpression(QStringLiteral(R"(\b(probability|expected value|equation|integral|derivative|prove|proof|optimi[sz]e|combinatorics|time complexity|calculate|compute)\b)"), CI),
    };
    return p;
}

const Patterns& reasoning()
{
    static const Patterns p {
        QRegularExpression(QStringLiteral(R"((단계별로|차근차근|논리적으로|근거\s*(를|까지)|비교\s*분석|장단점|트레이드\s*오프|반례|추론해|따져\s*(봐|줘)))")),
        QRegularExpression(QStringLiteral(R"(\b(step by step|logically|reasoning|pros and cons|trade-?offs?|counter-?examples?|compare and contrast|analy[sz]e|justify)\b)"), CI),
    };
    return p;
}

bool any(const Patterns& list, const QString& text)
{
    for (const QRegularExpression& re : list)
        if (re.match(text).hasMatch())
            return true;
    return false;
}

QString firstMatch(const Patterns& list, const QString& text)
{
    for (const QRegularExpression& re : list) {
        const QRegularExpressionMatch m = re.match(text);
        if (m.hasMatch())
            return m.captured(0).trimmed();
    }
    return {};
}

bool addressedTo(const ChatMessage& m, const QString& id, const ChatSession& s)
{
    if (const ChatMessage* r = s.byId(m.replyTo); r && r->from == id)
        return true;
    return Members::mentions(m.text, id);
}

QString heavyReason(const QString& text)
{
    if (text.contains(QStringLiteral("```")))
        return QStringLiteral("코드 블록");
    if (!any(request(), text))
        return {};
    if (any(code(), text))
        return QStringLiteral("코드 요청");
    if (any(math(), text))
        return QStringLiteral("계산 요청");
    if (any(reasoning(), text))
        return QStringLiteral("다단계 추론 질문");
    return {};
}

}

std::optional<BoostDecision> BoostRouter::decide(const QString& id, const ChatSession& s, qint64 seen, const QString& mode)
{
    if (mode == QLatin1String("off") || !this->config->agents.value(id).hasBoost())
        return std::nullopt;
    const qint64 now = QDateTime::currentMSecsSinceEpoch();

    if (this->pending.contains(id)) {
        const qint64 at = this->pending.take(id);
        if (now - at < 10 * 60000)
            return BoostDecision { QStringLiteral("command"), QStringLiteral("%1의 /boost").arg(this->config->userName) };
    }

    QList<ChatMessage> fresh;
    for (const ChatMessage& m : s.after(seen))
        if (m.from != id && !m.isSystem() && !m.text.isEmpty())
            fresh << m;
    fresh = fresh.mid(qMax<qsizetype>(0, fresh.size() - 8));

    for (auto it = fresh.crbegin(); it != fresh.crend(); ++it) {
        if (!it->isUser() || !addressedTo(*it, id, s))
            continue;
        const QString hit = firstMatch(serious(), it->text);
        if (!hit.isEmpty())
            return BoostDecision { QStringLiteral("user"),
                                   QStringLiteral("%1 \"%2\"").arg(Members::withJosa(Members::User, this->config->userName, QStringLiteral("이/가")), hit) };
    }
    if (mode != QLatin1String("auto"))
        return std::nullopt;

    for (auto it = fresh.crbegin(); it != fresh.crend(); ++it) {
        if (!addressedTo(*it, id, s))
            continue;
        const QString why = heavyReason(it->text);
        if (why.isEmpty())
            continue;
        if (!it->isUser()) {
            // 멤버끼리는 일을 자주 부탁하니 진심모드 한도를 다 쓰지 않게 쿨다운.
            const qint64 cool = qint64(this->config->boostAiRequestCooldownSec) * 1000;
            if (now - this->lastAutoBoostAt.value(id, 0) < cool)
                continue;
            this->lastAutoBoostAt.insert(id, now);
        }
        return BoostDecision { QStringLiteral("context"), QStringLiteral("%1 (#%2)").arg(why).arg(it->id) };
    }
    return std::nullopt;
}

std::optional<BoostRouter::Command> BoostRouter::parseCommand(const QString& text)
{
    static const QRegularExpression cmd(QStringLiteral(R"(^/boost\b([\s\S]*)$)"), CI);
    static const QRegularExpression target(QStringLiteral(R"(^@?([A-Za-z가-힣]+)\s*)"));
    const QRegularExpressionMatch m = cmd.match(text.trimmed());
    if (!m.hasMatch())
        return std::nullopt;

    Command c;
    QString rest = m.captured(1).trimmed();
    for (;;) {
        const QRegularExpressionMatch t = target.match(rest);
        if (!t.hasMatch())
            break;
        QString found;
        for (const MemberInfo& info : Members::all())
            if (info.aliases.contains(t.captured(1).toLower()))
                found = info.id;
        if (found.isEmpty())
            break;
        if (!c.ids.contains(found))
            c.ids << found;
        rest = rest.mid(t.capturedLength(0));
    }
    c.rest = rest.trimmed();
    return c;
}

void BoostRouter::arm(const QString& memberId)
{
    this->pending.insert(memberId, QDateTime::currentMSecsSinceEpoch());
}

void BoostRouter::reset()
{
    this->pending.clear();
    this->lastAutoBoostAt.clear();
}
