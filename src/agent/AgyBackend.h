#pragma once

#include "agent/AgentBackend.h"

namespace AITalk
{

// Antigravity CLI (Gemini): 프롬프트를 명령줄 인자(-p)로 넘기고 JSON 출력을 읽는다.
// 매 턴 새 대화가 agy 기록에 쌓이지 않게 대화 하나를 몇 턴(rotate) 재사용한다.
class AgyBackend : public AgentBackend {
public:
    explicit AgyBackend(int rotate = 4) : rotate(rotate) {}

    QString cliKey() const override { return QStringLiteral("agy"); }
    int maxPromptChars() const override { return 26000; }
    AgentCall* start(const QString& bin, const AgentRequest& req, QObject* parent) override;

    void setRotate(int rotate) { this->rotate = qMax(1, rotate); }

private:
    QString conversationId;
    int turns = 0;
    int rotate;
};

} // namespace AITalk
