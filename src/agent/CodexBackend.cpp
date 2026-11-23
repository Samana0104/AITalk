#include "agent/CodexBackend.h"

#include <QDir>
#include <QFile>

namespace AITalk
{

AgentCall* CodexBackend::start(const QString& bin, const AgentRequest& req, QObject* parent)
{
    const QString outFile = QDir(req.workDir).filePath(BackendUtil::uniqueName(QStringLiteral("reply-"), QStringLiteral(".txt")));
    const QString effort = req.effort.isEmpty() ? QStringLiteral("low") : req.effort;

    const QStringList args {
        QStringLiteral("exec"),
        QStringLiteral("-m"), req.model,
        QStringLiteral("-c"), QStringLiteral("model_reasoning_effort=\"%1\"").arg(effort),
        QStringLiteral("--skip-git-repo-check"),
        QStringLiteral("--ignore-user-config"),
        QStringLiteral("--ephemeral"),
        QStringLiteral("-s"), QStringLiteral("read-only"),
        QStringLiteral("--disable"), QStringLiteral("shell_tool"),
        QStringLiteral("--disable"), QStringLiteral("computer_use"),
        QStringLiteral("--disable"), QStringLiteral("browser_use"),
        QStringLiteral("--disable"), QStringLiteral("apps"),
        QStringLiteral("-c"), QStringLiteral("web_search=\"disabled\""),
        QStringLiteral("-c"), QStringLiteral("windows.sandbox=\"unelevated\""),
        QStringLiteral("--color"), QStringLiteral("never"),
        QStringLiteral("-o"), outFile,
        QStringLiteral("-"),
    };
    const QString input = req.brief + QStringLiteral("\n\n=====\n\n") + req.turn;

    auto* call = new ProcessAgentCall([outFile](const CliResult& r) {
        QString text;
        QFile f(outFile);
        if (f.open(QIODevice::ReadOnly)) {
            text = QString::fromUtf8(f.readAll());
            f.close();
            f.remove();
        }
        return AgentReply::fromProcess(r, text);
    }, parent);
    call->run(bin, args, input.toUtf8(), req.workDir, req.timeoutMs);
    return call;
}

} // namespace AITalk
