#include "agent/CliProcess.h"

namespace AITalk
{

namespace {
#ifdef Q_OS_WIN
constexpr unsigned long createNoWindow = 0x08000000; // CREATE_NO_WINDOW: 콘솔 창을 띄우지 않는다

void hideConsole(QProcess& p)
{
    p.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* a) {
        a->flags |= createNoWindow;
    });
}
#endif
}

CliProcess::CliProcess(QObject* parent)
    : QObject(parent)
{
#ifdef Q_OS_WIN
    hideConsole(this->process);
#endif
    this->timer.setSingleShot(true);
    connect(&this->timer, &QTimer::timeout, this, [this] {
        this->timedOut = true;
        killTree();
    });
    connect(&this->process, &QProcess::readyReadStandardOutput, this, [this] { this->standardOutput += this->process.readAllStandardOutput(); });
    connect(&this->process, &QProcess::readyReadStandardError, this, [this] { this->standardError += this->process.readAllStandardError(); });
    connect(&this->process, &QProcess::started, this, [this] {
        this->process.write(this->input);
        this->process.closeWriteChannel();
    });
    connect(&this->process, &QProcess::finished, this, [this](int code, QProcess::ExitStatus) { finish(code, false); });
    connect(&this->process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart) {
            this->standardError += this->process.errorString().toUtf8();
            finish(-1, true);
        }
    });
}

CliProcess::~CliProcess()
{
    if (this->process.state() != QProcess::NotRunning) {
        this->process.disconnect(this);
        killTree();
        this->process.waitForFinished(2000);
    }
}

void CliProcess::start(const QString& program, const QStringList& args, const QByteArray& input,
                       const QString& workDir, int timeoutMs, const QProcessEnvironment& env)
{
    this->input = input;
    this->process.setProgram(program);
    this->process.setArguments(args);
    this->process.setWorkingDirectory(workDir);
    this->process.setProcessEnvironment(env);
    this->clock.start();
    this->timer.start(timeoutMs);
    this->process.start(QIODevice::ReadWrite);
}

bool CliProcess::isRunning() const
{
    return this->process.state() != QProcess::NotRunning;
}

void CliProcess::kill()
{
    killTree();
}

void CliProcess::killTree()
{
    if (this->process.state() == QProcess::NotRunning)
        return;
#ifdef Q_OS_WIN
    // CLI 들은 자식 프로세스(node, 샌드박스 등)를 띄우므로 트리째 끝낸다.
    QProcess taskkill;
    taskkill.setProgram(QStringLiteral("taskkill"));
    taskkill.setArguments({ QStringLiteral("/PID"), QString::number(this->process.processId()), QStringLiteral("/T"), QStringLiteral("/F") });
    hideConsole(taskkill);
    taskkill.startDetached();
#endif
    this->process.kill();
}

void CliProcess::finish(int exitCode, bool failedToStart)
{
    if (this->done)
        return;
    this->done = true;
    this->timer.stop();
    this->standardOutput += this->process.readAllStandardOutput();
    this->standardError += this->process.readAllStandardError();

    CliResult r;
    r.exitCode = this->timedOut ? -2 : exitCode;
    r.timedOut = this->timedOut;
    r.failedToStart = failedToStart;
    r.out = this->standardOutput;
    r.err = this->standardError;
    r.ms = this->clock.isValid() ? this->clock.elapsed() : 0;
    QMetaObject::invokeMethod(this, [this, r] { emit finished(r); }, Qt::QueuedConnection);
}

} // namespace AITalk
