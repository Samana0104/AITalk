#pragma once

#include <QLocalServer>
#include <QLockFile>
#include <QObject>
#include <QString>

namespace AITalk
{

// 로컬 IPC(Named Pipe / Unix 소켓)로 앱 중복 실행을 막고 기존 인스턴스에 창 활성화를 요청한다.
class SingleInstance : public QObject
{
    Q_OBJECT

public:
    explicit SingleInstance(const QString& serverName, QObject* parent = nullptr);

    // 이 프로세스가 유일한 인스턴스면 true. 이미 실행 중이면 기존 인스턴스에 신호를 보내고 false.
    bool TryAcquire();

signals:
    void ActivateRequested();

private:
    bool NotifyRunningInstance() const;
    void HandleIncomingConnection();

private:
    QString serverName;
    QLockFile lockFile;
    QLocalServer server;
};

} // namespace AITalk
