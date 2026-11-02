#pragma once

#include <QObject>
#include <QPointer>
#include <QString>
#include <QTimer>
#include <functional>
#include <map>
#include <memory>
#include <optional>

#include "core/AgentAction.h"
#include "core/ChatMessage.h"
#include "engine/BoostRouter.h"
#include "storage/ChatSession.h"

class AgentCall;
class AgentHub;
struct AppConfig;
struct SpeedPreset;

// 방에 붙어 있는 동안 멤버 한 명의 스케줄 상태.
struct MemberRuntime {
    QString id;
    bool busy = false;
    QString status = QStringLiteral("idle"); // idle | reading | typing
    qint64 seen = 0;            // 마지막으로 읽은 메시지 id
    qint64 wakeAt = 0;          // 다음 턴 시작 예정 시각 (0: 없음)
    QString reason;             // 깨어나는 이유: new | urgent | idle | spark
    QString turnReason;         // 진행 중인 턴의 이유
    qint64 idleAt = 0;
    int idleStreak = 0;
    qint64 lastEnd = 0;
    int fails = 0;
    qint64 offlineUntil = 0;    // 실패 후 쉬는 중
    QString lastError;
    int calls = 0;
    qint64 lastMs = 0;
    qint64 lastSelfBoostAt = 0;
    bool deepNow = false;       // 진심모드 턴 진행 중
    QPointer<AgentCall> call;
};

// 단톡방 스케줄러 (ai-chatroom 서버의 대화 방식 포팅).
//  - 1초마다 tick: 새 메시지를 본 멤버는 몇 초 "읽은" 뒤 말하거나(say) 넘긴다(pass).
//    방장 메시지·@호출·답장이면 1~3초 안에 반응한다.
//  - 동시에 생각하는 멤버 수, 분당 말풍선 수, 멤버별 쿨다운으로 시끄러움을 조절한다.
//  - 한동안 조용하면 가장 오래 말 안 한 멤버 한 명을 골라 침묵을 깨게 한다(spark).
//  - 방장이 오래 말이 없으면 방이 잠든다. 말을 걸면 깨어난다.
//  - 매 턴은 독립 호출: brief + 메모 + 최근 대화 → JSON 액션 하나.
class RoomEngine : public QObject {
    Q_OBJECT
public:
    RoomEngine(AgentHub* hub, const AppConfig* config, QObject* parent = nullptr);
    ~RoomEngine() override;

    ChatSession* session() const { return this->activeSession; }
    bool isRunning() const { return this->running; }
    bool isSleeping() const { return this->sleeping; }

    const MemberRuntime& runtime(const QString& memberId) const;
    // idle | reading | typing | offline | disabled | missing
    QString statusOf(const QString& memberId) const;
    QString modelLabelOf(const QString& memberId) const;

public slots:
    void attach(ChatSession* session);
    void start();
    void stop();
    void sendUserMessage(const QString& text, qint64 replyTo = 0);
    void reactAsUser(qint64 messageId, const QString& emoji);

signals:
    void stateChanged(bool running, bool sleeping);
    void memberChanged(const QString& memberId);

private:
    struct Turn;
    using TurnPtr = std::shared_ptr<Turn>;
    using AskDone = std::function<void(std::optional<AgentAction> action, const QString& error)>;

    void tick();
    void sleepRoom();
    void abortAll();
    void resetRuntimes();
    bool isActive(const QString& memberId) const;
    bool isCalled(const ChatMessage& m, const QString& memberId) const;
    QString boostMode() const;
    bool hasBoost(const QString& memberId) const;
    int selfBoostWaitSec(const MemberRuntime& a) const;

    // 침묵 깨기
    void maybeSpark(qint64 now, const SpeedPreset& sp);
    MemberRuntime* pickSparker(qint64 now);
    void sparkDone(MemberRuntime& a, bool spoke);
    qint64 lastSaid(const QString& memberId) const;

    // 한 턴
    void runTurn(MemberRuntime& a);
    void ask(const TurnPtr& t, AskDone done);
    void askRouted(const TurnPtr& t, AskDone done);
    void startDeep(MemberRuntime& a, const BoostDecision& deep);
    void finishWithAction(const TurnPtr& t, const std::optional<AgentAction>& act);
    void perform(const TurnPtr& t, const AgentAction& act, std::function<void(bool)> done);
    void typeOut(const TurnPtr& t, QStringList msgs, qint64 replyTo, bool posted, std::function<void(bool)> done);
    void failTurn(const TurnPtr& t, const QString& error);
    void endTurn(const TurnPtr& t, bool ok);
    bool stale(const TurnPtr& t) const;

    void postSystem(const QString& kind, const QString& text);
    void postAgent(const TurnPtr& t, const QString& text, qint64 replyTo);
    void setStatus(MemberRuntime& a, const QString& status);
    int typingDelay(const QString& text) const;

    struct SparkState {
        qint64 base = -1;
        int streak = 0;
        qint64 wait = -1;
        qint64 lastEnd = 0;
        QString lastBy;
        QString who;
        bool quick = false;
    };

    AgentHub* hub;
    const AppConfig* config;
    QPointer<ChatSession> activeSession;
    BoostRouter router;
    QTimer timer;
    std::map<QString, MemberRuntime> memberRuntimes;
    SparkState spark;
    bool running = false;
    bool sleeping = false;
    qint64 startedAt = 0;
    qint64 lastUserAt = 0;
    quint64 epoch = 0; // 방을 끄거나 세션을 바꾸면 올라간다. 그 전 턴의 콜백은 무시된다.
};
