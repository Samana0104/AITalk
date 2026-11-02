#pragma once

#include "agent/AgentBackend.h"

// Codex CLI (ChatGPT): `codex exec` 를 읽기 전용 샌드박스·도구 끔으로 호출하고
// 마지막 답을 -o 파일로 받는다.
class CodexBackend : public AgentBackend {
public:
    QString cliKey() const override { return QStringLiteral("codex"); }
    AgentCall* start(const QString& bin, const AgentRequest& req, QObject* parent) override;
};
