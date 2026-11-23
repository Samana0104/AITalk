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
#include "manager/DataManager.h"

namespace AITalk
{

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
    , hub(hub)
    , config(config)
    , router(config)
{
    for (const QString& id : Members::ids())
        this->memberRuntimes[id].id = id;
    this->timer.setInterval(1000);
    connect(&this->timer, &QTimer::timeout, this, &RoomEngine::tick);
    this->timer.start();
}

RoomEngine::~RoomEngine()
{
    abortAll();
}

// ---------------------------------------------------------------------------
// 상태 조회

const MemberRuntime& RoomEngine::runtime(const QString& memberId) const
{
    return this->memberRuntimes.at(memberId);
}

QString RoomEngine::statusOf(const QString& memberId) const
{
    if (!this->hub->isAvailable(memberId))
        return QStringLiteral("missing");
    if (this->activeSession && !this->activeSession->isMemberEnabled(memberId))
        return QStringLiteral("disabled");
    const MemberRuntime& a = this->memberRuntimes.at(memberId);
    if (a.busy)
        return a.status;
    if (a.offlineUntil > nowMs())
        return QStringLiteral("offline");
    return QStringLiteral("idle");
}

QString RoomEngine::modelLabelOf(const QString& memberId) const
{
    return this->config->agents.value(memberId).label(this->memberRuntimes.at(memberId).deepNow);
}

bool RoomEngine::isActive(const QString& memberId) const
{
    return this->activeSession && this->activeSession->isMemberEnabled(memberId) && this->hub->isAvailable(memberId);
}

QString RoomEngine::boostMode() const
{
    return this->activeSession ? this->activeSession->boostMode() : this->config->boostMode;
}

bool RoomEngine::hasBoost(const QString& memberId) const
{
    return boostMode() != QLatin1String("off") && this->config->agents.value(memberId).hasBoost();
}

int RoomEngine::selfBoostWaitSec(const MemberRuntime& a) const
{
    const qint64 left = qint64(this->config->boostSelfCooldownSec) * 1000 - (nowMs() - a.lastSelfBoostAt);
    return left > 0 ? int((left + 999) / 1000) : 0;
}

// 이 멤버가 바로 답해야 하는 메시지인가? 답장·@호출은 그렇다. 방장 메시지도 그렇다,
// 단 방장이 분명히 다른 멤버에게 말하는 경우는 빼고.
bool RoomEngine::isCalled(const ChatMessage& m, const QString& id) const
{
    if (m.isSystem() || m.from == id)
        return false;
    const ChatMessage* r = this->activeSession->byId(m.replyTo);
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
    if (session == this->activeSession)
        return;
    abortAll();
    this->activeSession = session;
    this->running = false;
    this->sleeping = false;
    this->spark = {};
    this->router.reset();
    resetRuntimes();
    emit stateChanged(false, false);
    for (const auto& [id, a] : this->memberRuntimes)
        emit memberChanged(id);
}

void RoomEngine::resetRuntimes()
{
    for (auto& [id, a] : this->memberRuntimes) {
        a = MemberRuntime {};
        a.id = id;
        a.seen = this->activeSession ? this->activeSession->seenOf(id) : 0;
    }
}

void RoomEngine::abortAll()
{
    ++this->epoch;
    for (auto& [id, a] : this->memberRuntimes) {
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
    this->spark.who.clear();
}

void RoomEngine::start()
{
    if (!this->activeSession || this->running)
        return;
    const qint64 now = nowMs();
    const ChatMessage* lastReal = nullptr;
    const QList<ChatMessage>& all = this->activeSession->messages();
    for (qsizetype i = all.size() - 1; i >= 0; --i)
        if (!all.at(i).isSystem()) {
            lastReal = &all.at(i);
            break;
        }
    // 오래 비어 있던 방은 켜자마자 누군가 먼저 말을 꺼낸다.
    this->spark.quick = !lastReal || now - lastReal->ts > 10 * 60000;
    this->running = true;
    this->sleeping = false;
    this->startedAt = now;
    this->lastUserAt = now;

    bool anyone = false;
    for (const QString& id : Members::ids())
        anyone = anyone || isActive(id);
    if (!anyone)
        postSystem(QStringLiteral("error"), Text("engine.noAgents"));

    emit stateChanged(true, false);
}

void RoomEngine::stop()
{
    if (!this->running && !this->sleeping)
        return;
    abortAll();
    this->running = false;
    this->sleeping = false;
    emit stateChanged(false, false);
    for (const auto& [id, a] : this->memberRuntimes)
        emit memberChanged(id);
}

void RoomEngine::sleepRoom()
{
    abortAll();
    this->running = false;
    this->sleeping = true;
    postSystem(QStringLiteral("sleep"), Text("engine.sleep"));
    emit stateChanged(false, true);
    for (const auto& [id, a] : this->memberRuntimes)
        emit memberChanged(id);
}

void RoomEngine::sendUserMessage(const QString& rawText, qint64 replyTo)
{
    if (!this->activeSession)
        return;
    QString text = rawText.trimmed();
    if (text.isEmpty())
        return;

    if (const auto cmd = BoostRouter::parseCommand(text)) {
        const QStringList targets = cmd->ids.isEmpty() ? Members::ids() : cmd->ids;
        QStringList armed;
        for (const QString& id : targets)
            if (hasBoost(id)) {
                this->router.arm(id);
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
        m.replyTo = (replyTo && this->activeSession->byId(replyTo)) ? replyTo : 0;
        this->activeSession->addMessage(m);
    }
    this->lastUserAt = nowMs();
    if (!this->running)
        start();
}

void RoomEngine::reactAsUser(qint64 messageId, const QString& emoji)
{
    if (this->activeSession)
        this->activeSession->applyReaction(messageId, Members::User, emoji);
}

// ---------------------------------------------------------------------------
// 스케줄러

void RoomEngine::tick()
{
    if (!this->running || !this->activeSession)
        return;
    const qint64 now = nowMs();
    if (this->config->autoSleepMinutes > 0 && now - this->lastUserAt > qint64(this->config->autoSleepMinutes) * 60000) {
        sleepRoom();
        return;
    }
    const SpeedPreset sp = speedPreset(this->activeSession->speed());

    int inflight = 0;
    for (const auto& [id, a] : this->memberRuntimes)
        inflight += a.busy ? 1 : 0;

    const ChatMessage* last = this->activeSession->lastMessage();
    const qint64 quietMs = now - std::max(last ? last->ts : 0, this->startedAt);
    int aiBubbles = 0;
    for (const ChatMessage& m : this->activeSession->recent(60))
        if (m.isAgent() && now - m.ts < 60000)
            ++aiBubbles;

    maybeSpark(now, sp);

    QStringList order = Members::ids();
    std::shuffle(order.begin(), order.end(), *QRandomGenerator::global());
    for (const QString& id : order) {
        MemberRuntime& a = this->memberRuntimes[id];
        if (!isActive(id) || a.busy || now < a.offlineUntil)
            continue;

        bool hasFresh = false;
        bool urgent = false;
        for (const ChatMessage& m : this->activeSession->after(a.seen)) {
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
        } else if (!a.wakeAt && !this->config->sparkEnabled) {
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
        if (inflight >= this->config->maxInFlight)
            continue;
        if (a.reason != QLatin1String("urgent") && aiBubbles >= sp.perMin)
            continue;
        if (a.reason == QLatin1String("idle")) {
            bool otherIdle = false;
            for (const auto& [oid, o] : this->memberRuntimes)
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
    const QList<ChatMessage>& all = this->activeSession->messages();
    for (qsizetype i = all.size() - 1; i >= 0; --i)
        if (all.at(i).from == id)
            return all.at(i).ts;
    return 0;
}

MemberRuntime* RoomEngine::pickSparker(qint64 now)
{
    QStringList cands;
    for (const QString& id : Members::ids()) {
        const MemberRuntime& a = this->memberRuntimes[id];
        if (isActive(id) && !a.busy && !a.wakeAt && now >= a.offlineUntil)
            cands << id;
    }
    if (cands.isEmpty())
        return nullptr;
    if (cands.size() > 1)
        cands.removeAll(this->spark.lastBy);
    std::sort(cands.begin(), cands.end(), [this](const QString& x, const QString& y) { return lastSaid(x) < lastSaid(y); });
    const QStringList top = cands.mid(0, 2);
    return &this->memberRuntimes[top.at(QRandomGenerator::global()->bounded(int(top.size())))];
}

void RoomEngine::maybeSpark(qint64 now, const SpeedPreset& sp)
{
    if (!this->config->sparkEnabled)
        return;
    qint64 lastReal = 0;
    const QList<ChatMessage> recent = this->activeSession->recent(60);
    for (auto it = recent.crbegin(); it != recent.crend(); ++it)
        if (!it->isSystem()) {
            lastReal = it->ts;
            break;
        }
    const qint64 activity = std::max(lastReal, this->startedAt);
    if (activity != this->spark.base) {
        this->spark.base = activity;
        this->spark.streak = 0;
        this->spark.wait = this->spark.quick ? qint64(rnd(4, 25) * 1000) : -1;
        this->spark.quick = false;
    }
    if (!this->spark.who.isEmpty()) {
        const MemberRuntime& w = this->memberRuntimes[this->spark.who];
        if (w.busy || w.wakeAt)
            return; // 뽑힌 멤버가 아직 진행 중
        this->spark.who.clear();
    }
    for (const auto& [id, a] : this->memberRuntimes)
        if (a.busy || a.wakeAt)
            return; // 실제로는 조용하지 않음
    if (this->spark.wait < 0) {
        const double secs = std::min(rnd(sp.sparkLo, sp.sparkHi) * std::pow(this->config->sparkBackoff, this->spark.streak),
                                     double(this->config->sparkMaxWaitSec));
        this->spark.wait = qint64(secs * 1000);
    }
    if (now < std::max(this->spark.base, this->spark.lastEnd) + this->spark.wait)
        return;
    MemberRuntime* a = pickSparker(now);
    if (!a)
        return;
    this->spark.who = a->id;
    a->wakeAt = now;
    a->reason = QStringLiteral("spark");
    this->activeSession->log(a->id, QStringLiteral("spark: picked after %1s of quiet (streak %2)").arg((now - this->spark.base) / 1000).arg(this->spark.streak));
}

void RoomEngine::sparkDone(MemberRuntime& a, bool spoke)
{
    this->spark.lastEnd = nowMs();
    this->spark.lastBy = a.id;
    this->spark.wait = -1;
    this->spark.streak = spoke ? 0 : this->spark.streak + 1;
    if (this->spark.who == a.id)
        this->spark.who.clear();
}

// ---------------------------------------------------------------------------
// 한 턴: 읽기(호출) → (필요하면 진심모드로 다시) → 수행(반응·메모·말풍선) → 정리

bool RoomEngine::stale(const TurnPtr& t) const
{
    return t->epoch != this->epoch || !this->activeSession;
}

void RoomEngine::runTurn(MemberRuntime& a)
{
    auto t = std::make_shared<Turn>();
    t->id = a.id;
    t->reason = a.reason;
    t->snapshot = this->activeSession->lastId();
    t->epoch = this->epoch;

    a.busy = true;
    a.turnReason = t->reason;
    a.wakeAt = 0;
    a.reason.clear();
    setStatus(a, QStringLiteral("reading"));

    // 진심모드 여부는 턴마다 새로 정한다.
    if (hasBoost(a.id))
        t->deep = this->router.decide(a.id, *this->activeSession, a.seen, boostMode());
    if (t->deep)
        startDeep(a, *t->deep);

    askRouted(t, [this, t](std::optional<AgentAction> act, const QString& err) {
        if (stale(t))
            return;
        if (!err.isEmpty()) {
            failTurn(t, err);
            return;
        }
        MemberRuntime& a = this->memberRuntimes[t->id];
        const QString selfAsk = act ? act->boost : QString();
        if (!t->deep && !selfAsk.isEmpty() && hasBoost(t->id) && boostMode() == QLatin1String("auto") && this->running) {
            if (selfBoostWaitSec(a) > 0) {
                this->activeSession->log(t->id, QStringLiteral("boost requested while cooling down; answering as usual"));
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
    MemberRuntime& a = this->memberRuntimes[t->id];

    PromptBuilder::BriefInput bi;
    bi.memberId = t->id;
    bi.roomName = this->activeSession->title();
    bi.boostMode = boostMode();
    bi.deep = t->deep;
    const QString brief = PromptBuilder::buildBrief(bi, *this->config);

    PromptBuilder::TurnInput ti;
    ti.session = this->activeSession;
    ti.memberId = t->id;
    ti.seen = a.seen;
    ti.reason = t->reason;
    ti.boostWaitSec = (!t->deep && hasBoost(t->id) && boostMode() == QLatin1String("auto")) ? selfBoostWaitSec(a) : 0;

    // 너무 길면 대화 기록을 8개씩 줄인다.
    const int maxChars = this->hub->maxPromptChars(t->id) - int(brief.size()) - 200;
    QString turn;
    for (int hist = this->config->historyForPrompt;; hist -= 8) {
        ti.historyCount = hist;
        turn = PromptBuilder::buildTurn(ti, *this->config);
        if (turn.size() <= maxChars || hist <= 8)
            break;
    }

    ++a.calls;
    emit memberChanged(t->id);

    AgentCall* call = this->hub->chat(t->id, brief, turn, t->deep.has_value(), this);
    if (!call) {
        QTimer::singleShot(0, this, [done] { done(std::nullopt, QStringLiteral("CLI not available")); });
        return;
    }
    a.call = call;
    const QString model = this->config->agents.value(t->id).label(t->deep.has_value());
    connect(call, &AgentCall::finished, this, [this, t, done, model](const AgentReply& r) {
        if (stale(t))
            return;
        MemberRuntime& a = this->memberRuntimes[t->id];
        a.call = nullptr;
        a.lastMs = r.ms;
        const QString head = QStringLiteral("turn reason=%1 model=%2%3 ms=%4 ok=%5")
                                 .arg(t->reason, model, t->deep ? QStringLiteral(" BOOST") : QString(),
                                      QString::number(r.ms), r.ok ? QStringLiteral("true") : QStringLiteral("false"));
        this->activeSession->log(t->id, head + QLatin1Char('\n') + r.text.left(4000)
                                  + (r.detail.isEmpty() ? QString() : QStringLiteral("\nDETAIL ") + r.detail));
        if (!r.ok) {
            done(std::nullopt, r.detail.isEmpty() ? QStringLiteral("empty reply") : r.detail);
            return;
        }
        const std::optional<AgentAction> act = AgentAction::parse(r.text);
        if (!act)
            this->activeSession->log(t->id, QStringLiteral("could not parse a JSON action"));
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
        this->activeSession->log(t->id, QStringLiteral("route BOOST failed -> default: %1").arg(err.left(200)));
        t->deep.reset();
        MemberRuntime& a = this->memberRuntimes[t->id];
        a.deepNow = false;
        emit memberChanged(t->id);
        if (this->running)
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
                                            .arg(nameOf(a.id), this->config->agents.value(a.id).label(true), deep.reason));
    this->activeSession->log(a.id, QStringLiteral("route BOOST by=%1 reason=%2").arg(deep.by, deep.reason));
    emit memberChanged(a.id);
}

void RoomEngine::finishWithAction(const TurnPtr& t, const std::optional<AgentAction>& act)
{
    if (act && this->running) {
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
        visible = this->activeSession->applyReaction(act.reactId, t->id, act.reactEmoji) || visible;

    if (!act.noteReplace.trimmed().isEmpty())
        this->activeSession->writeNote(t->id, act.noteReplace);
    if (!act.noteAdd.trimmed().isEmpty())
        this->activeSession->appendNote(t->id, act.noteAdd);

    QStringList msgs = act.action == QLatin1String("pass") ? QStringList() : act.messages;

    // 생각하는 동안 대화가 많이 흘러갔으면 뒷북 수다는 버린다.
    int movedOn = 0;
    for (const ChatMessage& m : this->activeSession->after(t->snapshot))
        if (m.from != t->id && !m.isSystem())
            ++movedOn;
    QString emoticon = Emoticons::isAvailable(t->id, act.emoticon) ? act.emoticon : QString();
    if (!act.emoticon.isEmpty() && emoticon.isEmpty())
        this->activeSession->log(t->id, QStringLiteral("unknown emoticon: ") + act.emoticon);
    if ((!msgs.isEmpty() || !emoticon.isEmpty()) && movedOn >= 3 && t->reason != QLatin1String("urgent") && !t->deep) {
        this->activeSession->log(t->id, QStringLiteral("dropped stale messages (%1 new since snapshot)").arg(movedOn));
        msgs.clear();
        emoticon.clear();
    }

    const qint64 replyTo = this->activeSession->byId(act.replyTo) ? act.replyTo : 0;
    typeOut(t, msgs, replyTo, false, [this, t, emoticon, done, visible](bool posted) {
        // 이모티콘은 말풍선 뒤에 따로 (카톡 스티커처럼)
        if (!emoticon.isEmpty() && !stale(t) && this->running) {
            ChatMessage m;
            m.from = t->id;
            m.emoticon = emoticon;
            m.model = this->config->agents.value(t->id).label(t->deep.has_value());
            m.boosted = t->deep.has_value();
            this->activeSession->addMessage(m);
            posted = true;
        }
        done(visible || posted);
    });
}

// 말풍선을 하나씩 "입력 중" 시간을 두고 올린다.
void RoomEngine::typeOut(const TurnPtr& t, QStringList msgs, qint64 replyTo, bool posted, std::function<void(bool)> done)
{
    if (msgs.isEmpty() || stale(t) || !this->running) {
        done(posted);
        return;
    }
    setStatus(this->memberRuntimes[t->id], QStringLiteral("typing"));
    const QString text = msgs.takeFirst();
    QTimer::singleShot(typingDelay(text), this, [this, t, msgs, text, replyTo, posted, done]() {
        if (stale(t) || !this->running) {
            done(posted);
            return;
        }
        postAgent(t, text, replyTo);
        typeOut(t, msgs, 0, true, done);
    });
}

void RoomEngine::failTurn(const TurnPtr& t, const QString& error)
{
    MemberRuntime& a = this->memberRuntimes[t->id];
    ++a.fails;
    a.lastError = error.simplified().left(300);
    this->activeSession->log(t->id, QStringLiteral("ERROR ") + a.lastError);
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
    MemberRuntime& a = this->memberRuntimes[t->id];
    if (ok) {
        a.fails = 0;
        a.lastError.clear();
    }
    if (t->reason == QLatin1String("idle") && !t->spoke)
        ++a.idleStreak;
    if (t->reason == QLatin1String("spark"))
        sparkDone(a, t->spoke);
    else if (this->spark.who == a.id)
        this->spark.who.clear(); // 뽑힌 멤버가 일반 턴으로 먼저 불려 간 경우

    a.seen = std::max(a.seen, t->snapshot);
    this->activeSession->setSeen(a.id, a.seen);
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
    if (!this->activeSession)
        return;
    ChatMessage m;
    m.from = Members::System;
    m.kind = kind;
    m.text = text;
    this->activeSession->addMessage(m);
}

void RoomEngine::postAgent(const TurnPtr& t, const QString& text, qint64 replyTo)
{
    ChatMessage m;
    m.from = t->id;
    m.text = text;
    m.replyTo = replyTo;
    m.model = this->config->agents.value(t->id).label(t->deep.has_value());
    m.boosted = t->deep.has_value();
    this->activeSession->addMessage(m);
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
    const SpeedPreset sp = speedPreset(this->activeSession ? this->activeSession->speed() : QStringLiteral("normal"));
    return int(std::min(700.0 + text.size() * 40.0, 4000.0) * sp.typing);
}

} // namespace AITalk
