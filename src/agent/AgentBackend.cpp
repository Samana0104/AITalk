#include "agent/AgentBackend.h"

#include <QSaveFile>
#include <QUuid>

AgentReply AgentReply::fromProcess(const CliResult& r, const QString& text)
{
    AgentReply reply;
    reply.text = text;
    reply.ms = r.ms;
    reply.ok = r.exitCode == 0 && !r.timedOut && !text.trimmed().isEmpty();
    if (!reply.ok) {
        reply.detail = QStringLiteral("exit=%1%2 %3")
                           .arg(r.exitCode)
                           .arg(r.timedOut ? QStringLiteral(" (timeout)") : QString())
                           .arg(QString::fromUtf8(r.err.right(600)));
    }
    return reply;
}

ProcessAgentCall::ProcessAgentCall(Parser parser, QObject* parent)
    : AgentCall(parent)
    , parser(std::move(parser))
    , process(new CliProcess(this))
{
    connect(this->process, &CliProcess::finished, this, [this](const CliResult& r) {
        if (!this->aborted)
            emit finished(this->parser(r));
        deleteLater();
    });
}

void ProcessAgentCall::run(const QString& program, const QStringList& args, const QByteArray& input,
                           const QString& workDir, int timeoutMs, const QProcessEnvironment& env)
{
    if (program.isEmpty()) {
        CliResult r;
        r.failedToStart = true;
        r.err = "CLI not found";
        QMetaObject::invokeMethod(this, [this, r] {
            if (!this->aborted)
                emit finished(this->parser(r));
            deleteLater();
        }, Qt::QueuedConnection);
        return;
    }
    this->process->start(program, args, input, workDir, timeoutMs, env);
}

void ProcessAgentCall::abort()
{
    this->aborted = true;
    if (this->process->isRunning())
        this->process->kill(); // 끝나면 finished 핸들러에서 deleteLater
    else
        deleteLater();
}

namespace BackendUtil {

bool writeText(const QString& path, const QString& text)
{
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return false;
    f.write(text.toUtf8());
    return f.commit();
}

QString uniqueName(const QString& prefix, const QString& suffix)
{
    return prefix + QUuid::createUuid().toString(QUuid::WithoutBraces) + suffix;
}

}
