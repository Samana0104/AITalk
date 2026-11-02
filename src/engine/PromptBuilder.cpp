#include "engine/PromptBuilder.h"

#include <QDateTime>
#include <QFile>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <algorithm>

#include "core/AppConfig.h"
#include "core/Emoticons.h"
#include "core/Member.h"
#include "storage/ChatSession.h"

namespace {

QString readResource(const QString& path)
{
    QFile f(path);
    return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()) : QString();
}

const QString& briefTemplate()
{
    static const QString t = readResource(QStringLiteral(":/prompts/ko/brief.md"));
    return t;
}

const QStringList& sparkCards()
{
    static const QStringList cards = [] {
        QStringList out;
        for (const QString& line : readResource(QStringLiteral(":/prompts/ko/spark_cards.txt")).split(QLatin1Char('\n')))
            if (!line.trimmed().isEmpty())
                out << line.trimmed();
        return out;
    }();
    return cards;
}

QString clock(qint64 ts) { return QDateTime::fromMSecsSinceEpoch(ts).toString(QStringLiteral("HH:mm:ss")); }

QString ago(qint64 secs)
{
    if (secs < 60)
        return QStringLiteral("%1초 전").arg(secs);
    if (secs < 3600)
        return QStringLiteral("%1분 전").arg(qRound(secs / 60.0));
    return QStringLiteral("%1시간 전").arg(qRound(secs / 3600.0));
}

QString elapsed(qint64 secs) { return ago(secs).remove(QStringLiteral(" 전")); }

const QStringList kWeek { "일", "월", "화", "수", "목", "금", "토" };

QString weekday(const QDateTime& d) { return kWeek.value(d.date().dayOfWeek() % 7); }

QString timeOfDay(int h)
{
    if (h < 6) return QStringLiteral("새벽");
    if (h < 11) return QStringLiteral("아침");
    if (h < 14) return QStringLiteral("점심때");
    if (h < 18) return QStringLiteral("오후");
    if (h < 21) return QStringLiteral("저녁");
    return QStringLiteral("밤");
}

QString deepSection(const PromptBuilder::BriefInput& in, const AgentConfig& a)
{
    const bool canDeep = a.hasBoost() && in.boostMode != QLatin1String("off");
    if (!canDeep)
        return {};
    const QString def = a.label(false);
    const QString boost = a.label(true);
    if (in.deep) {
        const QString why = in.deep->by == QLatin1String("self")
            ? QStringLiteral("네가 켰어. 이유: %1").arg(in.deep->reason)
            : QStringLiteral("방이 올렸어. 이유: %1").arg(in.deep->reason);
        return QStringLiteral("\n\n## 지금은 진심모드야\n"
                              "- 이번 턴은 평소 설정(%1) 대신 %2로 답하고 있어. %3\n"
                              "- 대충 넘기지 말고 제대로 생각해서 답해. 그래도 단톡방이니까 말풍선은 읽기 좋게 나눠(최대 4개).")
            .arg(def, boost, why);
    }
    const bool autoMode = in.boostMode == QLatin1String("auto");
    QString s = QStringLiteral("\n\n## 진심모드\n"
                               "- 너는 평소 %1로 대답해. 잡담은 이걸로 충분해.\n"
                               "- 무거운 턴은 %2로 올라가(진심모드). 방장이 너한테 \"진지하게\", \"각잡고\" 같은 말을 하거나 /boost로 부르면%3 방이 턴마다 판단해서 올려줘.")
                    .arg(def, boost,
                         autoMode ? QStringLiteral(", 또는 누가 너한테 코드 작업, 복잡한 계산, 여러 단계로 따져야 하는 질문을 하면") : QString());
    if (autoMode) {
        s += QStringLiteral("\n- 기본 설정으로 불렸는데 제대로 생각해야 하는 턴이면 직접 켤 수도 있어: JSON에 \"boost\": \"이유 한 줄\". "
                            "그러면 이번 턴을 %1가 다시 맡아. 그때 messages는 비우거나 짧은 한마디만(\"잠깐, 이건 제대로 답할게\" 같은).\n"
                            "- 진심모드는 사용량을 훨씬 많이 써. 남발하지 마.")
                 .arg(boost);
    }
    return s;
}

// CodePet 의 이모티콘 규칙을 단톡방에 맞게: 매 답이 아니라 어울릴 때만, 한 턴에 하나.
QString emoticonLine(const QString& memberId)
{
    QStringList guide;
    for (const EmoticonInfo& e : Emoticons::all())
        if (Emoticons::isAvailable(memberId, e.key))
            guide << QStringLiteral("%1=%2").arg(e.key, e.meaning);
    if (guide.isEmpty())
        return {};
    return QStringLiteral("- 너한테는 네 캐릭터 이모티콘이 있어. \"emoticon\"에 키를 넣으면 네 말풍선 다음에 그림으로 올라가. "
                          "말로 하는 것보다 그림 하나가 더 어울릴 때만, 한 턴에 최대 1개. 매번 쓰면 질려.\n"
                          "  키=쓰는 상황: %1\n"
                          "  단어가 나왔다고 고르지 말고, 지금 네 태도에 뜻이 딱 맞는 걸 골라. 목록에 없는 키는 안 보여.")
        .arg(guide.join(QStringLiteral("; ")));
}

QString formatMessage(const ChatMessage& m, const QString& selfId, const AppConfig& cfg)
{
    if (m.isSystem())
        return QStringLiteral("#%1 %2 · %3").arg(m.id).arg(clock(m.ts), m.text);
    QString who = Members::displayName(m.from, cfg.userName);
    if (m.from == selfId)
        who += QStringLiteral("(나)");
    QString line = QStringLiteral("#%1 %2 %3").arg(m.id).arg(clock(m.ts), who);
    if (m.replyTo)
        line += QStringLiteral(" ↪#%1").arg(m.replyTo);
    line += QStringLiteral(": ") + m.displayText().replace(QLatin1Char('\n'), QStringLiteral("\n    "));
    if (!m.reactions.isEmpty()) {
        QStringList parts;
        for (auto it = m.reactions.cbegin(); it != m.reactions.cend(); ++it) {
            QStringList names;
            for (const QString& w : it.value())
                names << Members::displayName(w, cfg.userName);
            parts << QStringLiteral("%1 %2").arg(it.key(), names.join(QLatin1Char(',')));
        }
        line += QStringLiteral("   [반응 %1]").arg(parts.join(QStringLiteral(" / ")));
    }
    return line;
}

// 메모 중 "하고 싶은 것: …" 줄
const QRegularExpression& wishPattern()
{
    static const QRegularExpression re(QStringLiteral(R"(하고\s*싶은\s*것|해보고\s*싶|하고싶|want\s*to\s*(do|try))"), QRegularExpression::CaseInsensitiveOption);
    return re;
}

QStringList sparkSituation(const QString& note, const ChatMessage* last, qint64 now)
{
    const qint64 quietMin = last ? qMax<qint64>(1, qRound((now - last->ts) / 60000.0)) : 0;

    QStringList wishes;
    for (const QString& l : note.split(QLatin1Char('\n')))
        if (wishPattern().match(l).hasMatch())
            wishes << QStringLiteral("\"%1\"").arg(l.trimmed().remove(QRegularExpression(QStringLiteral(R"(^[-*\s]+)"))));
    wishes = wishes.mid(qMax<qsizetype>(0, wishes.size() - 5));

    QStringList cards = sparkCards();
    std::shuffle(cards.begin(), cards.end(), *QRandomGenerator::global());
    const QDateTime d = QDateTime::fromMSecsSinceEpoch(now);

    QStringList ideas;
    if (!wishes.isEmpty())
        ideas << QStringLiteral("네 메모의 '하고 싶은 것': %1").arg(wishes.join(QStringLiteral(", ")));
    ideas << QStringLiteral("지금은 %1요일 %2이야. 시간대에 어울리는 얘기").arg(weekday(d), timeOfDay(d.time().hour()));
    for (const QString& c : cards.mid(0, 2))
        ideas << QStringLiteral("오늘의 카드: %1").arg(c);

    QStringList ideaLines;
    for (const QString& i : ideas)
        ideaLines << QStringLiteral("  · ") + i;

    return {
        QStringLiteral("- 방이 %1 조용해. 이번엔 네가 침묵을 깰 차례로 뽑혔어. 다른 멤버들은 지금 안 불려서, 네가 안 하면 계속 조용해.")
            .arg(quietMin ? QStringLiteral("%1분째").arg(quietMin) : QStringLiteral("한동안")),
        QStringLiteral("- \"다들 뭐 해?\", \"조용하네\" 같은 빈 인사 말고, 네가 진짜 하고 싶은 걸로 바로 시작해. 누구한테 말을 걸면 그 멤버가 받을 거야."),
        QStringLiteral("- 떠오르는 게 없으면 이 중에서 골라도 돼 (그대로 읽지 말고 네 식으로):\n") + ideaLines.join(QLatin1Char('\n')),
        QStringLiteral("- 정말 하고 싶은 게 없으면 pass해도 돼. 그러면 좀 있다가 다른 멤버 차례가 와."),
    };
}

}

namespace PromptBuilder {

QString buildBrief(const BriefInput& in, const AppConfig& cfg)
{
    const MemberInfo* me = Members::find(in.memberId);
    if (!me)
        return {};
    const AgentConfig a = cfg.agents.value(in.memberId);

    QStringList roster;
    for (const MemberInfo& m : Members::all())
        roster << QStringLiteral("  - %1 (%2)%3").arg(m.name, m.maker, m.id == me->id ? QStringLiteral(" ← 너") : QString());

    const bool boostField = a.hasBoost() && !in.deep && in.boostMode == QLatin1String("auto");

    QString s = briefTemplate();
    s.replace(QStringLiteral("{{name}}"), me->name);
    s.replace(QStringLiteral("{{maker}}"), me->maker);
    s.replace(QStringLiteral("{{roomName}}"), in.roomName);
    s.replace(QStringLiteral("{{userName}}"), cfg.userName);
    s.replace(QStringLiteral("{{roster}}"), roster.join(QLatin1Char('\n')));
    s.replace(QStringLiteral("{{emoticonLine}}"), emoticonLine(in.memberId));
    s.replace(QStringLiteral("{{deepSection}}"), deepSection(in, a));
    s.replace(QStringLiteral("{{boostField}}"), boostField ? QStringLiteral(",\n  \"boost\": \"진심모드를 켤 이유(필요할 때만)\"") : QString());
    return s;
}

QString buildTurn(const TurnInput& in, const AppConfig& cfg)
{
    const ChatSession& s = *in.session;
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const QDateTime d = QDateTime::fromMSecsSinceEpoch(now);
    QStringList parts;

    parts << QStringLiteral("[지금] %1 (%2) %3").arg(d.toString(QStringLiteral("yyyy-MM-dd")), weekday(d), clock(now));

    const QString note = s.readNote(in.memberId).trimmed();
    parts << QStringLiteral("[네 개인 메모]\n%1").arg(note.isEmpty() ? QStringLiteral("(아직 비어 있음)") : note);

    const QList<ChatMessage> hist = s.recent(in.historyCount);
    QStringList lines;
    for (const ChatMessage& m : hist)
        lines << formatMessage(m, in.memberId, cfg);
    parts << QStringLiteral("[대화 기록] 최근 %1개, 오래된 것부터\n%2")
                 .arg(hist.size())
                 .arg(lines.isEmpty() ? QStringLiteral("(아직 아무 말도 없음)") : lines.join(QLatin1Char('\n')));

    // ---- 상황 ----
    QStringList sit;
    QList<ChatMessage> fresh;
    for (const ChatMessage& m : s.after(in.seen))
        if (m.from != in.memberId)
            fresh << m;
    if (!fresh.isEmpty())
        sit << QStringLiteral("- 네가 마지막으로 본 뒤 새 메시지 %1개 (#%2부터).").arg(fresh.size()).arg(fresh.first().id);
    else
        sit << QStringLiteral("- 네가 마지막으로 본 뒤 새 메시지는 없어.");

    QStringList called, fromUser;
    for (const ChatMessage& m : fresh) {
        if (m.isSystem())
            continue;
        const ChatMessage* r = s.byId(m.replyTo);
        if (Members::mentions(m.text, in.memberId) || (r && r->from == in.memberId))
            called << QStringLiteral("#%1").arg(m.id);
        if (m.isUser())
            fromUser << QStringLiteral("#%1").arg(m.id);
    }
    if (!called.isEmpty())
        sit << QStringLiteral("- 너를 부르거나 언급하거나 너한테 답한 메시지: %1").arg(called.join(QStringLiteral(", ")));
    if (!fromUser.isEmpty())
        sit << QStringLiteral("- %1 메시지: %2").arg(cfg.userName, fromUser.join(QStringLiteral(", ")));

    const ChatMessage* last = s.lastMessage();
    if (last)
        sit << QStringLiteral("- 마지막 메시지 이후 %1 지났어.").arg(elapsed((now - last->ts) / 1000));

    const QList<ChatMessage>& all = s.messages();
    for (qsizetype i = all.size() - 1; i >= 0; --i) {
        if (all.at(i).from == in.memberId) {
            sit << QStringLiteral("- 네 마지막 발언: %1.").arg(ago((now - all.at(i).ts) / 1000));
            break;
        }
    }

    int window = 0, mine = 0;
    for (const ChatMessage& m : s.recent(20)) {
        if (m.isSystem())
            continue;
        ++window;
        if (m.from == in.memberId)
            ++mine;
    }
    if (window >= 8 && double(mine) / window > 0.4)
        sit << QStringLiteral("- 최근 메시지 %1개 중 %2개가 네 거야. 좀 말이 많았어.").arg(window).arg(mine);
    if (in.boostWaitSec > 0)
        sit << QStringLiteral("- 진심모드는 %1초 뒤에 다시 쓸 수 있어.").arg(in.boostWaitSec);
    if (note.size() > 1800)
        sit << QStringLiteral("- 네 메모가 %1자로 길어졌어. 이번에 note_replace로 핵심만 남겨 정리해줘.").arg(note.size());
    if (in.reason == QLatin1String("idle"))
        sit << QStringLiteral("- 한동안 조용해. 꺼내고 싶은 얘기가 있으면 해도 되고, 없으면 pass.");
    if (in.reason == QLatin1String("spark"))
        sit << sparkSituation(note, last, now);
    bool anyoneSpoke = false;
    for (const ChatMessage& m : all)
        if (!m.isSystem()) {
            anyoneSpoke = true;
            break;
        }
    if (!anyoneSpoke)
        sit << QStringLiteral("- 방이 막 만들어졌어. 아직 아무도 말을 안 했어.");

    parts << QStringLiteral("[상황]\n") + sit.join(QLatin1Char('\n'));
    parts << QStringLiteral("네 차례야. JSON 하나로 답해.");
    return parts.join(QStringLiteral("\n\n"));
}

}
