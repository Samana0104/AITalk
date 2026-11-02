#include "agent/CliProcess.h"

namespace {
#ifdef Q_OS_WIN
constexpr unsigned long kCreateNoWindow = 0x08000000; // CREATE_NO_WINDOW: 콘솔 창을 띄우지 않는다

void hideConsole(QProcess& p)
{
    p.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* a) {
        a->flags |= kCreateNoWindow;
    });
}
#endif
}

CliProcess::CliProcess(QObject* parent)
    : QObject(parent)
{
#ifdef Q_OS_WIN
    hideConsole(m_proc);
#endif
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, [this] {
        m_timedOut = true;
        killTree();
    });
    connect(&m_proc, &QProcess::readyReadStandardOutput, this, [this] { m_out += m_proc.readAllStandardOutput(); });
    connect(&m_proc, &QProcess::readyReadStandardError, this, [this] { m_err += m_proc.readAllStandardError(); });
    connect(&m_proc, &QProcess::started, this, [this] {
        m_proc.write(m_input);
        m_proc.closeWriteChannel();
    });
    connect(&m_proc, &QProcess::finished, this, [this](int code, QProcess::ExitStatus) { finish(code, false); });
    connect(&m_proc, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart) {
            m_err += m_proc.errorString().toUtf8();
            finish(-1, true);
        }
    });
}

CliProcess::~CliProcess()
{
    if (m_proc.state() != QProcess::NotRunning) {
        m_proc.disconnect(this);
        killTree();
        m_proc.waitForFinished(2000);
    }
}

void CliProcess::start(const QString& program, const QStringList& args, const QByteArray& input,
                       const QString& workDir, int timeoutMs, const QProcessEnvironment& env)
{
    m_input = input;
    m_proc.setProgram(program);
    m_proc.setArguments(args);
    m_proc.setWorkingDirectory(workDir);
    m_proc.setProcessEnvironment(env);
    m_clock.start();
    m_timer.start(timeoutMs);
    m_proc.start(QIODevice::ReadWrite);
}

bool CliProcess::isRunning() const
{
    return m_proc.state() != QProcess::NotRunning;
}

void CliProcess::kill()
{
    killTree();
}

void CliProcess::killTree()
{
    if (m_proc.state() == QProcess::NotRunning)
        return;
#ifdef Q_OS_WIN
    // CLI 들은 자식 프로세스(node, 샌드박스 등)를 띄우므로 트리째 끝낸다.
    QProcess taskkill;
    taskkill.setProgram(QStringLiteral("taskkill"));
    taskkill.setArguments({ QStringLiteral("/PID"), QString::number(m_proc.processId()), QStringLiteral("/T"), QStringLiteral("/F") });
    hideConsole(taskkill);
    taskkill.startDetached();
#endif
    m_proc.kill();
}

void CliProcess::finish(int exitCode, bool failedToStart)
{
    if (m_done)
        return;
    m_done = true;
    m_timer.stop();
    m_out += m_proc.readAllStandardOutput();
    m_err += m_proc.readAllStandardError();

    CliResult r;
    r.exitCode = m_timedOut ? -2 : exitCode;
    r.timedOut = m_timedOut;
    r.failedToStart = failedToStart;
    r.out = m_out;
    r.err = m_err;
    r.ms = m_clock.isValid() ? m_clock.elapsed() : 0;
    QMetaObject::invokeMethod(this, [this, r] { emit finished(r); }, Qt::QueuedConnection);
}
