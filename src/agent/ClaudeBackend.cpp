#include "agent/ClaudeBackend.h"

#include <QDir>

AgentCall* ClaudeBackend::start(const QString& bin, const AgentRequest& req, QObject* parent)
{
    const QString sysFile = QDir(req.workDir).filePath(QStringLiteral("system.md"));
    BackendUtil::writeText(sysFile, req.brief);

    QStringList args {
        QStringLiteral("-p"),
        QStringLiteral("--model"), req.model,
        QStringLiteral("--tools"), QString(),          // 도구 전부 끔
        QStringLiteral("--strict-mcp-config"),
        QStringLiteral("--no-session-persistence"),
        QStringLiteral("--setting-sources"), QString(), // 사용자 설정 무시
        QStringLiteral("--system-prompt-file"), sysFile,
    };
    if (!req.effort.isEmpty())
        args << QStringLiteral("--effort") << req.effort;

    auto* call = new ProcessAgentCall([](const CliResult& r) {
        return AgentReply::fromProcess(r, QString::fromUtf8(r.out));
    }, parent);
    call->run(bin, args, req.turn.toUtf8(), req.workDir, req.timeoutMs);
    return call;
}
