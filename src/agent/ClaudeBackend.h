#pragma once

#include "agent/AgentBackend.h"

// Claude Code: `claude -p` 를 도구 전부 끈 채로 호출. 시스템 프롬프트는 파일로, 턴은 stdin 으로.
class ClaudeBackend : public AgentBackend {
public:
    QString cliKey() const override { return QStringLiteral("claude"); }
    AgentCall* start(const QString& bin, const AgentRequest& req, QObject* parent) override;
};
