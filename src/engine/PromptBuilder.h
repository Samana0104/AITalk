#pragma once

#include <QString>
#include <optional>

#include "engine/BoostRouter.h"

class ChatSession;
struct AppConfig;

// 멤버 한 명의 한 턴 프롬프트를 만든다. 매 호출은 독립적이다:
// brief(방 규칙·응답 형식, res/prompts/ko/brief.md) + turn(메모·대화 기록·상황).
namespace PromptBuilder {

struct BriefInput {
    QString memberId;
    QString roomName;
    QString boostMode = QStringLiteral("auto");
    std::optional<BoostDecision> deep; // 이번 턴이 진심모드면 이유
};

struct TurnInput {
    const ChatSession* session = nullptr;
    QString memberId;
    QString reason;          // new | urgent | idle | spark
    qint64 seen = 0;         // 이 멤버가 마지막으로 읽은 메시지 id
    int historyCount = 40;
    int boostWaitSec = 0;    // 진심모드를 다시 직접 켤 수 있을 때까지 남은 초
};

QString buildBrief(const BriefInput& in, const AppConfig& cfg);
QString buildTurn(const TurnInput& in, const AppConfig& cfg);

}
