#include "engine/RoomEngine.h"

#include <QDateTime>
#include <QRandomGenerator>
#include <algorithm>
#include <cmath>

#include "agent/AgentBackend.h"
#include "agent/AgentHub.h"
#include "core/AppConfig.h"
#include "core/Emoticons.h"
#include "core/Member.h"
#include "engine/PromptBuilder.h"
#include "engine/SpeedPreset.h"
#include "storage/ChatSession.h"
#include "util/DataManager.h"

namespace {

qint64 nowMs() { return QDateTime::currentMSecsSinceEpoch(); }

double rnd(double lo, double hi) { return lo + QRandomGenerator::global()->generateDouble() * (hi - lo); }

QString nameOf(const QString& id) { return Members::displayName(id, QString()); }

}

struct RoomEngine::Turn {
    QString id;
    QString reason;
    qint64 snapshot = 0; // 턴 시작 시점의 마지막 메시지 id
    std::optional<BoostDecision> deep;
    bool spoke = false;
    quint64 epoch = 0;
};

RoomEngine::RoomEngine(AgentHub* hub, const AppConfig* config, QObject* parent)
    : QObject(parent)
    , m_hub(hub)
    , m_cfg(config)
    , m_router(config)
{
    for (const QString& id : Members::ids())
        m_rt[id].id = id;
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &RoomEngine::tick);
    m_timer.start();
}

RoomEngine::~RoomEngine()
{
    abortAll();
}

// ---------------------------------------------------------------------------
// 상태 조회

const MemberRuntime& RoomEngine::runtime(const QString& memberId) const
{
    return m_rt.at(memberId);
}

QString RoomEngine::statusOf(const QString& memberId) const
{
    if (!m_hub->isAvailable(memberId))
        return QStringLiteral("missing");
    if (m_session && !m_session->isMemberEnabled(memberId))
        return QStringLiteral("disabled");
    const MemberRuntime& a = m_rt.at(memberId);
    if (a.busy)
        return a.status;
    if (a.offlineUntil > nowMs())
        return QStringLiteral("offline");
    return QStringLiteral("idle");
}

QString RoomEngine::modelLabelOf(const QString& memberId) const
{
    return m_cfg->agents.value(memberId).label(m_rt.at(memberId).deepNow);
}

bool RoomEngine::isActive(const QString& memberId) const
{
    return m_session && m_session->isMemberEnabled(memberId) && m_hub->isAvailable(memberId);
}

QString RoomEngine::boostMode() const
{
    return m_session ? m_session->boostMode() : m_cfg->boostMode;
}

bool RoomEngine::hasBoost(const QString& memberId) const
{
    return boostMode() != QLatin1String("off") && m_cfg->agents.value(memberId).hasBoost();
}

int RoomEngine::selfBoostWaitSec(const MemberRuntime& a) const
{
    const qint64 left = qint64(m_cfg->boostSelfCooldownSec) * 1000 - (nowMs() - a.lastSelfBoostAt);
    return left > 0 ? int((left + 999) / 1000) : 0;
}

// 이 멤버가 바로 답해야 하는 메시지인가? 답장·@호출은 그렇다. 방장 메시지도 그렇다,
// 단 방장이 분명히 다른 멤버에게 말하는 경우는 빼고.
bool RoomEngine::isCalled(const ChatMessage& m, const QString& id) const
{
    if (m.isSystem() || m.from == id)
        return false;
    const ChatMessage* r = m_session->byId(m.replyTo);
    if (r && r->from == id)
        return true;
    if (Members::atMentions(m.text, id))
        return true;
    if (!m.isUser())
        return false;
    if (r && Members::isAgent(r->from))
        return false;
    QStringList named;
    for (const QString& o : Members::ids())
        if (Members::mentions(m.text, o))
            named << o;
    return named.isEmpty() || named.contains(id);
}

// ---------------------------------------------------------------------------
// 방 켜기·끄기, 세션 연결

void RoomEngine::attach(ChatSession* session)
{
    if (session == m_session)
        return;
    abortAll();
    m_session = session;
    m_running = false;
    m_sleeping = false;
    m_spark = {};
    m_router.reset();
    resetRuntimes();
    emit stateChanged(false, false);
    for (const auto& [id, a] : m_rt)
        emit memberChanged(id);
}

void RoomEngine::resetRuntimes()
{
    for (auto& [id, a] : m_rt) {
        a = MemberRuntime {};
        a.id = id;
        a.seen = m_session ? m_session->seenOf(id) : 0;
    }
}

void RoomEngine::abortAll()
{
    ++m_epoch;
    for (auto& [id, a] : m_rt) {
        if (a.call) {
            a.call->abort();
            a.call = nullptr;
        }
        a.busy = false;
        a.status = QStringLiteral("idle");
        a.wakeAt = 0;
        a.reason.clear();
        a.turnReason.clear();
        a.deepNow = false;
    }
    m_spark.who.clear();
}

void RoomEngine::start()
{
    if (!m_session || m_running)
        return;
    const qint64 now = nowMs();
    const ChatMessage* lastReal = nullptr;
    const QList<ChatMessage>& all = m_session->messages();
    for (qsizetype i = all.size() - 1; i >= 0; --i)
        if (!all.at(i).isSystem()) {
            lastReal = &all.at(i);
            break;
        }
    // 오래 비어 있던 방은 켜자마자 누군가 먼저 말을 꺼낸다.
    m_spark.quick = !lastReal || now - lastReal->ts > 10 * 60000;
    m_running = true;
    m_sleeping = false;
    m_startedAt = now;
    m_lastUserAt = now;

    bool anyone = false;
    for (const QString& id : Members::ids())
        anyone = anyone || isActive(id);
    if (!anyone)
        postSystem(QStringLiteral("error"), Text("engine.noAgents"));

    emit stateChanged(true, false);
}

void RoomEngine::stop()
{
    if (!m_running && !m_sleeping)
        return;
    abortAll();
    m_running = false;
    m_sleeping = false;
    emit stateChanged(false, false);
    for (const auto& [id, a] : m_rt)
        emit memberChanged(id);
}

void RoomEngine::sleepRoom()
{
    abortAll();
    m_running = false;
    m_sleeping = true;
    postSystem(QStringLiteral("sleep"), Text("engine.sleep"));
    emit stateChanged(false, true);
    for (const auto& [id, a] : m_rt)
        emit memberChanged(id);
}

void RoomEngine::sendUserMessage(const QString& rawText, qint64 replyTo)
{
    if (!m_session)
        return;
    QString text = rawText.trimmed();
    if (text.isEmpty())
        return;

    if (const auto cmd = BoostRouter::parseCommand(text)) {
        const QStringList targets = cmd->ids.isEmpty() ? Members::ids() : cmd->ids;
        QStringList armed;
        for (const QString& id : targets)
            if (hasBoost(id)) {
                m_router.arm(id);
                armed << nameOf(id);
            }
        text = cmd->rest;
        if (!armed.isEmpty())
            postSystem(QStringLiteral("boost"), Text("engine.boostArmed", armed.join(QStringLiteral(", "))));
    }

    if (!text.isEmpty()) {
        ChatMessage m;
        m.from = Members::User;
        m.text = text;
        m.replyTo = (replyTo && m_session->byId(replyTo)) ? replyTo : 0;
        m_session->addMessage(m);
    }
    m_lastUserAt = nowMs();
    if (!m_running)
        start();
}

void RoomEngine::reactAsUser(qint64 messageId, const QString& emoji)
{
    if (m_session)
        m_session->applyReaction(messageId, Members::User, emoji);
}

// ---------------------------------------------------------------------------
// 스케줄러

void RoomEngine::tick()
{
    if (!m_running || !m_session)
        return;
    const qint64 now = nowMs();
    if (m_cfg->autoSleepMinutes > 0 && now - m_lastUserAt > qint64(m_cfg->autoSleepMinutes) * 60000) {
        sleepRoom();
        return;
    }
    const SpeedPreset sp = speedPreset(m_session->speed());

    int inflight = 0;
    for (const auto& [id, a] : m_rt)
        inflight += a.busy ? 1 : 0;

    const ChatMessage* last = m_session->lastMessage();
    const qint64 quietMs = now - std::max(last ? last->ts : 0, m_startedAt);
    int aiBubbles = 0;
    for (const ChatMessage& m : m_session->recent(60))
        if (m.isAgent() && now - m.ts < 60000)
            ++aiBubbles;

    maybeSpark(now, sp);

    QStringList order = Members::ids();
    std::shuffle(order.begin(), order.end(), *QRandomGenerator::global());
    for (const QString& id : order) {
        MemberRuntime& a = m_rt[id];
        if (!isActive(id) || a.busy || now < a.offlineUntil)
            continue;

        bool hasFresh = false;
        bool urgent = false;
        for (const ChatMessage& m : m_session->after(a.seen)) {
            if (m.from == id || m.isSystem())
                continue;
            hasFresh = true;
            urgent = urgent || isCalled(m, id);
        }

        if (hasFresh) {
            a.idleStreak = 0;
            a.idleAt = 0;
            if (!a.wakeAt || (urgent && a.reason != QLatin1String("urgent"))) {
                const double lo = urgent ? 1 : sp.readLo;
                const double hi = urgent ? 3 : sp.readHi;
                a.wakeAt = now + qint64(rnd(lo, hi) * 1000);
                a.reason = urgent ? QStringLiteral("urgent") : QStringLiteral("new");
            }
        } else if (!a.wakeAt && !m_cfg->sparkEnabled) {
            // 침묵 깨기를 끈 경우: 각자 혼잣말 턴을 받는다 (예전 방식).
            if (!a.idleAt)
                a.idleAt = now + qint64(rnd(sp.idleLo, sp.idleHi) * 1000 * std::pow(1.7, std::min(a.idleStreak, 5)));
            if (now >= a.idleAt && quietMs > sp.idleLo * 500) {
                a.wakeAt = now;
                a.reason = QStringLiteral("idle");
            }
        }

        if (!a.wakeAt || now < a.wakeAt)
            continue;
        if (now - a.lastEnd < qint64(sp.cooldown) * 1000)
            continue;
        if (inflight >= m_cfg->maxInFlight)
            continue;
        if (a.reason != QLatin1String("urgent") && aiBubbles >= sp.perMin)
            continue;
        if (a.reason == QLatin1String("idle")) {
            bool otherIdle = false;
            for (const auto& [oid, o] : m_rt)
                otherIdle = otherIdle || (o.busy && o.turnReason == QLatin1String("idle"));
            if (otherIdle) {
                a.wakeAt = 0;
                a.idleAt = now + 15000;
                continue;
            }
        }
        ++inflight;
        runTurn(a);
    }
}

// ---- 침묵 깨기 ----
// 조용할 때 모두에게 혼잣말 턴을 주면 서로 미루다 다 pass 한다(방관자 효과).
// 그래서 한 번에 한 명만 골라 "네 차례"라고 알려 주고, 말할 거리(메모의 하고 싶은 것,
// 시간대, 랜덤 카드)를 준다. pass 하면 다음 차례까지 기다리는 시간이 늘어난다.

qint64 RoomEngine::lastSaid(const QString& id) const
{
    const QList<ChatMessage>& all = m_session->messages();
    for (qsizetype i = all.size() - 1; i >= 0; --i)
        if (all.at(i).from == id)
            return all.at(i).ts;
    return 0;
}

MemberRuntime* RoomEngine::pickSparker(qint64 now)
{
    QStringList cands;
    for (const QString& id : Members::ids()) {
        const MemberRuntime& a = m_rt[id];
        if (isActive(id) && !a.busy && !a.wakeAt && now >= a.offlineUntil)
            cands << id;
    }
    if (cands.isEmpty())
        return nullptr;
    if (cands.size() > 1)
        cands.removeAll(m_spark.lastBy);
    std::sort(cands.begin(), cands.end(), [this](const QString& x, const QString& y) { return lastSaid(x) < lastSaid(y); });
    const QStringList top = cands.mid(0, 2);
    return &m_rt[top.at(QRandomGenerator::global()->bounded(int(top.size())))];
}

void RoomEngine::maybeSpark(qint64 now, const SpeedPreset& sp)
{
    if (!m_cfg->sparkEnabled)
        return;
    qint64 lastReal = 0;
    const QList<ChatMessage> recent = m_session->recent(60);
    for (auto it = recent.crbegin(); it != recent.crend(); ++it)
        if (!it->isSystem()) {
            lastReal = it->ts;
            break;
        }
    const qint64 activity = std::max(lastReal, m_startedAt);
    if (activity != m_spark.base) {
        m_spark.base = activity;
        m_spark.streak = 0;
        m_spark.wait = m_spark.quick ? qint64(rnd(4, 25) * 1000) : -1;
        m_spark.quick = false;
    }
    if (!m_spark.who.isEmpty()) {
        const MemberRuntime& w = m_rt[m_spark.who];
        if (w.busy || w.wakeAt)
            return; // 뽑힌 멤버가 아직 진행 중
        m_spark.who.clear();
    }
    for (const auto& [id, a] : m_rt)
        if (a.busy || a.wakeAt)
            return; // 실제로는 조용하지 않음
    if (m_spark.wait < 0) {
        const double secs = std::min(rnd(sp.sparkLo, sp.sparkHi) * std::pow(m_cfg->sparkBackoff, m_spark.streak),
                                     double(m_cfg->sparkMaxWaitSec));
        m_spark.wait = qint64(secs * 1000);
    }
    if (now < std::max(m_spark.base, m_spark.lastEnd) + m_spark.wait)
        return;
    MemberRuntime* a = pickSparker(now);
    if (!a)
        return;
    m_spark.who = a->id;
    a->wakeAt = now;
    a->reason = QStringLiteral("spark");
    m_session->log(a->id, QStringLiteral("spark: picked after %1s of quiet (streak %2)").arg((now - m_spark.base) / 1000).arg(m_spark.streak));
}

void RoomEngine::sparkDone(MemberRuntime& a, bool spoke)
{
    m_spark.lastEnd = nowMs();
    m_spark.lastBy = a.id;
    m_spark.wait = -1;
    m_spark.streak = spoke ? 0 : m_spark.streak + 1;
    if (m_spark.who == a.id)
        m_spark.who.clear();
}

// ---------------------------------------------------------------------------
// 한 턴: 읽기(호출) → (필요하면 진심모드로 다시) → 수행(반응·메모·말풍선) → 정리

bool RoomEngine::stale(const TurnPtr& t) const
{
    return t->epoch != m_epoch || !m_session;
}

void RoomEngine::runTurn(MemberRuntime& a)
{
    auto t = std::make_shared<Turn>();
    t->id = a.id;
    t->reason = a.reason;
    t->snapshot = m_session->lastId();
    t->epoch = m_epoch;

    a.busy = true;
    a.turnReason = t->reason;
    a.wakeAt = 0;
    a.reason.clear();
    setStatus(a, QStringLiteral("reading"));

    // 진심모드 여부는 턴마다 새로 정한다.
    if (hasBoost(a.id))
        t->deep = m_router.decide(a.id, *m_session, a.seen, boostMode());
    if (t->deep)
        startDeep(a, *t->deep);

    askRouted(t, [this, t](std::optional<AgentAction> act, const QString& err) {
        if (stale(t))
            return;
        if (!err.isEmpty()) {
            failTurn(t, err);
            return;
        }
        MemberRuntime& a = m_rt[t->id];
        const QString selfAsk = act ? act->boost : QString();
        if (!t->deep && !selfAsk.isEmpty() && hasBoost(t->id) && boostMode() == QLatin1String("auto") && m_running) {
            if (selfBoostWaitSec(a) > 0) {
                m_session->log(t->id, QStringLiteral("boost requested while cooling down; answering as usual"));
            } else {
                t->deep = BoostDecision { QStringLiteral("self"), QStringLiteral("직접 요청: %1").arg(selfAsk.simplified().left(100)) };
                // 기본 모델의 짧은 한마디("잠깐, 제대로 답할게")는 먼저 올린다.
                if (!act->messages.isEmpty()) {
                    postAgent(t, act->messages.first(), act->replyTo);
                    t->spoke = true;
                }
                startDeep(a, *t->deep);
                askRouted(t, [this, t](std::optional<AgentAction> act2, const QString& err2) {
                    if (stale(t))
                        return;
                    if (!err2.isEmpty())
                        failTurn(t, err2);
                    else
                        finishWithAction(t, act2);
                });
                return;
            }
        }
        finishWithAction(t, act);
    });
}

void RoomEngine::ask(const TurnPtr& t, AskDone done)
{
    MemberRuntime& a = m_rt[t->id];

    PromptBuilder::BriefInput bi;
    bi.memberId = t->id;
    bi.roomName = m_session->title();
    bi.boostMode = boostMode();
    bi.deep = t->deep;
    const QString brief = PromptBuilder::buildBrief(bi, *m_cfg);

    PromptBuilder::TurnInput ti;
    ti.session = m_session;
    ti.memberId = t->id;
    ti.seen = a.seen;
    ti.reason = t->reason;
    ti.boostWaitSec = (!t->deep && hasBoost(t->id) && boostMode() == QLatin1String("auto")) ? selfBoostWaitSec(a) : 0;

    // 너무 길면 대화 기록을 8개씩 줄인다.
    const int maxChars = m_hub->maxPromptChars(t->id) - int(brief.size()) - 200;
    QString turn;
    for (int hist = m_cfg->historyForPrompt;; hist -= 8) {
        ti.historyCount = hist;
        turn = PromptBuilder::buildTurn(ti, *m_cfg);
        if (turn.size() <= maxChars || hist <= 8)
            break;
    }

    ++a.calls;
    emit memberChanged(t->id);

    AgentCall* call = m_hub->chat(t->id, brief, turn, t->deep.has_value(), this);
    if (!call) {
        QTimer::singleShot(0, this, [done] { done(std::nullopt, QStringLiteral("CLI not available")); });
        return;
    }
    a.call = call;
    const QString model = m_cfg->agents.value(t->id).label(t->deep.has_value());
    connect(call, &AgentCall::finished, this, [this, t, done, model](const AgentReply& r) {
        if (stale(t))
            return;
        MemberRuntime& a = m_rt[t->id];
        a.call = nullptr;
        a.lastMs = r.ms;
        const QString head = QStringLiteral("turn reason=%1 model=%2%3 ms=%4 ok=%5")
                                 .arg(t->reason, model, t->deep ? QStringLiteral(" BOOST") : QString(),
                                      QString::number(r.ms), r.ok ? QStringLiteral("true") : QStringLiteral("false"));
        m_session->log(t->id, head + QLatin1Char('\n') + r.text.left(4000)
                                  + (r.detail.isEmpty() ? QString() : QStringLiteral("\nDETAIL ") + r.detail));
        if (!r.ok) {
            done(std::nullopt, r.detail.isEmpty() ? QStringLiteral("empty reply") : r.detail);
            return;
        }
        const std::optional<AgentAction> act = AgentAction::parse(r.text);
        if (!act)
            m_session->log(t->id, QStringLiteral("could not parse a JSON action"));
        done(act, QString());
    });
}

// 진심모드로 물어보고, 그 호출이 실패하면 평소 설정으로 한 번 더.
void RoomEngine::askRouted(const TurnPtr& t, AskDone done)
{
    if (!t->deep) {
        ask(t, std::move(done));
        return;
    }
    ask(t, [this, t, done](std::optional<AgentAction> act, const QString& err) {
        if (stale(t))
            return;
        if (err.isEmpty()) {
            done(act, err);
            return;
        }
        m_session->log(t->id, QStringLiteral("route BOOST failed -> default: %1").arg(err.left(200)));
        t->deep.reset();
        MemberRuntime& a = m_rt[t->id];
        a.deepNow = false;
        emit memberChanged(t->id);
        if (m_running)
            postSystem(QStringLiteral("error"), Text("engine.boostFailed", nameOf(t->id)));
        ask(t, done);
    });
}

void RoomEngine::startDeep(MemberRuntime& a, const BoostDecision& deep)
{
    a.deepNow = true;
    if (deep.by == QLatin1String("self"))
        a.lastSelfBoostAt = nowMs();
    postSystem(QStringLiteral("boost"), Text("engine.boostOn")
                                            .arg(nameOf(a.id), m_cfg->agents.value(a.id).label(true), deep.reason));
    m_session->log(a.id, QStringLiteral("route BOOST by=%1 reason=%2").arg(deep.by, deep.reason));
    emit memberChanged(a.id);
}

void RoomEngine::finishWithAction(const TurnPtr& t, const std::optional<AgentAction>& act)
{
    if (act && m_running) {
        perform(t, *act, [this, t](bool visible) {
            t->spoke = t->spoke || visible;
            endTurn(t, true);
        });
    } else {
        endTurn(t, true);
    }
}

void RoomEngine::perform(const TurnPtr& t, const AgentAction& act, std::function<void(bool)> done)
{
    bool visible = false;

    if (act.reactId && !act.reactEmoji.isEmpty())
        visible = m_session->applyReaction(act.reactId, t->id, act.reactEmoji) || visible;

    if (!act.noteReplace.trimmed().isEmpty())
        m_session->writeNote(t->id, act.noteReplace);
    if (!act.noteAdd.trimmed().isEmpty())
        m_session->appendNote(t->id, act.noteAdd);

    QStringList msgs = act.action == QLatin1String("pass") ? QStringList() : act.messages;

    // 생각하는 동안 대화가 많이 흘러갔으면 뒷북 수다는 버린다.
    int movedOn = 0;
    for (const ChatMessage& m : m_session->after(t->snapshot))
        if (m.from != t->id && !m.isSystem())
            ++movedOn;
    QString emoticon = Emoticons::isAvailable(t->id, act.emoticon) ? act.emoticon : QString();
    if (!act.emoticon.isEmpty() && emoticon.isEmpty())
        m_session->log(t->id, QStringLiteral("unknown emoticon: ") + act.emoticon);
    if ((!msgs.isEmpty() || !emoticon.isEmpty()) && movedOn >= 3 && t->reason != QLatin1String("urgent") && !t->deep) {
        m_session->log(t->id, QStringLiteral("dropped stale messages (%1 new since snapshot)").arg(movedOn));
        msgs.clear();
        emoticon.clear();
    }

    const qint64 replyTo = m_session->byId(act.replyTo) ? act.replyTo : 0;
    typeOut(t, msgs, replyTo, false, [this, t, emoticon, done, visible](bool posted) {
        // 이모티콘은 말풍선 뒤에 따로 (카톡 스티커처럼)
        if (!emoticon.isEmpty() && !stale(t) && m_running) {
            ChatMessage m;
            m.from = t->id;
            m.emoticon = emoticon;
            m.model = m_cfg->agents.value(t->id).label(t->deep.has_value());
            m.boosted = t->deep.has_value();
            m_session->addMessage(m);
            posted = true;
        }
        done(visible || posted);
    });
}

// 말풍선을 하나씩 "입력 중" 시간을 두고 올린다.
void RoomEngine::typeOut(const TurnPtr& t, QStringList msgs, qint64 replyTo, bool posted, std::function<void(bool)> done)
{
    if (msgs.isEmpty() || stale(t) || !m_running) {
        done(posted);
        return;
    }
    setStatus(m_rt[t->id], QStringLiteral("typing"));
    const QString text = msgs.takeFirst();
    QTimer::singleShot(typingDelay(text), this, [this, t, msgs, text, replyTo, posted, done]() {
        if (stale(t) || !m_running) {
            done(posted);
            return;
        }
        postAgent(t, text, replyTo);
        typeOut(t, msgs, 0, true, done);
    });
}

void RoomEngine::failTurn(const TurnPtr& t, const QString& error)
{
    MemberRuntime& a = m_rt[t->id];
    ++a.fails;
    a.lastError = error.simplified().left(300);
    m_session->log(t->id, QStringLiteral("ERROR ") + a.lastError);
    // 20초, 40초, 80초 … 최대 5분 쉬기
    a.offlineUntil = nowMs() + std::min<qint64>(20000LL << std::min(a.fails - 1, 10), 300000);
    if (a.fails == 1)
        postSystem(QStringLiteral("error"), Text("engine.turnFailed", nameOf(t->id)));
    endTurn(t, false);
}

void RoomEngine::endTurn(const TurnPtr& t, bool ok)
{
    if (stale(t))
        return;
    MemberRuntime& a = m_rt[t->id];
    if (ok) {
        a.fails = 0;
        a.lastError.clear();
    }
    if (t->reason == QLatin1String("idle") && !t->spoke)
        ++a.idleStreak;
    if (t->reason == QLatin1String("spark"))
        sparkDone(a, t->spoke);
    else if (m_spark.who == a.id)
        m_spark.who.clear(); // 뽑힌 멤버가 일반 턴으로 먼저 불려 간 경우

    a.seen = std::max(a.seen, t->snapshot);
    m_session->setSeen(a.id, a.seen);
    a.lastEnd = nowMs();
    a.idleAt = 0;
    a.busy = false;
    a.deepNow = false;
    a.turnReason.clear();
    a.status = QStringLiteral("idle");
    emit memberChanged(a.id);
}

// ---------------------------------------------------------------------------

void RoomEngine::postSystem(const QString& kind, const QString& text)
{
    if (!m_session)
        return;
    ChatMessage m;
    m.from = Members::System;
    m.kind = kind;
    m.text = text;
    m_session->addMessage(m);
}

void RoomEngine::postAgent(const TurnPtr& t, const QString& text, qint64 replyTo)
{
    ChatMessage m;
    m.from = t->id;
    m.text = text;
    m.replyTo = replyTo;
    m.model = m_cfg->agents.value(t->id).label(t->deep.has_value());
    m.boosted = t->deep.has_value();
    m_session->addMessage(m);
}

void RoomEngine::setStatus(MemberRuntime& a, const QString& status)
{
    if (a.status == status)
        return;
    a.status = status;
    emit memberChanged(a.id);
}

int RoomEngine::typingDelay(const QString& text) const
{
    const SpeedPreset sp = speedPreset(m_session ? m_session->speed() : QStringLiteral("normal"));
    return int(std::min(700.0 + text.size() * 40.0, 4000.0) * sp.typing);
}
