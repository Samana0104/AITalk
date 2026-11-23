#pragma once

#include <QHash>
#include <QString>
#include <QStringList>
#include <optional>

namespace AITalk
{

class ChatSession;
struct AppConfig;

struct BoostDecision {
    QString by;     // command | user | context | self
    QString reason;
};

// 턴마다 평소 모델로 갈지 진심모드(boost) 모델로 갈지 정한다. 애매하면 평소 모델.
// 강한 것부터:
//   1. 방장의 `/boost @멤버` 명령
//   2. 방장이 그 멤버에게 "진지하게", "seriously" 같은 말을 함
//   3. (auto 모드만) 그 멤버에게 코드·계산·다단계 추론 요청이 옴. AI 끼리의 요청은 쿨다운.
// 멤버가 답에 "boost" 를 넣어 직접 켜는 경우는 RoomEngine 이 처리한다.
class BoostRouter {
public:
    explicit BoostRouter(const AppConfig* config) : config(config) {}

    // mode: auto | manual | off
    std::optional<BoostDecision> decide(const QString& memberId, const ChatSession& session, qint64 seen, const QString& mode);

    struct Command {
        QStringList ids;
        QString rest;
    };
    // "/boost @Gemini @Claude 나머지 말" -> {ids, rest}. 명령이 아니면 nullopt.
    static std::optional<Command> parseCommand(const QString& text);

    void arm(const QString& memberId);
    void reset();

private:
    const AppConfig* config;
    QHash<QString, qint64> pending;  // /boost 로 예약된 멤버 -> 시각
    QHash<QString, qint64> lastAutoBoostAt; // AI 요청으로 켠 마지막 시각
};

} // namespace AITalk
