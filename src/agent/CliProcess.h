#pragma once

#include <QByteArray>
#include <QElapsedTimer>
#include <QObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTimer>

struct CliResult {
    int exitCode = -1;
    bool timedOut = false;
    bool failedToStart = false;
    QByteArray out;
    QByteArray err;
    qint64 ms = 0;
};

// 헤드리스 CLI 한 번 실행: stdin 을 넣고, 시간 제한을 걸고, stdout/stderr 를 모은다.
// 시간이 지나거나 kill() 하면 자식 프로세스 트리까지 끝낸다.
// finished 는 항상 이벤트 루프를 한 번 거쳐서(queued) 딱 한 번 나온다.
class CliProcess : public QObject {
    Q_OBJECT
public:
    explicit CliProcess(QObject* parent = nullptr);
    ~CliProcess() override;

    void start(const QString& program, const QStringList& args, const QByteArray& input,
               const QString& workDir, int timeoutMs,
               const QProcessEnvironment& env = QProcessEnvironment::systemEnvironment());
    void kill();
    bool isRunning() const;

signals:
    void finished(const CliResult& result);

private:
    void finish(int exitCode, bool failedToStart);
    void killTree();

    QProcess m_proc;
    QTimer m_timer;
    QElapsedTimer m_clock;
    QByteArray m_input;
    QByteArray m_out;
    QByteArray m_err;
    bool m_timedOut = false;
    bool m_done = false;
};
