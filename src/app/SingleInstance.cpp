#include "app/SingleInstance.h"

#include <QDebug>
#include <QDir>
#include <QLocalSocket>

namespace AITalk
{

namespace
{

constexpr int connectTimeoutMs = 250;

} // namespace

SingleInstance::SingleInstance(const QString& serverName, QObject* parent)
    : QObject(parent)
    , serverName(serverName)
    , lockFile(QDir::temp().filePath(serverName + QStringLiteral(".lock")))
{
    this->lockFile.setStaleLockTime(0);
    connect(&this->server, &QLocalServer::newConnection, this, &SingleInstance::HandleIncomingConnection);
}

bool SingleInstance::TryAcquire()
{
    // Windows 의 QLocalServer 는 같은 파이프 이름으로도 중복 listen 이 가능하므로
    // QLockFile 로 단일 인스턴스를 먼저 원자적으로 보장한다.
    if(!this->lockFile.tryLock(0))
    {
        NotifyRunningInstance();
        return false;
    }

    // 비정상 종료로 남은 소켓 파일을 정리한 뒤 다시 연다.
    QLocalServer::removeServer(this->serverName);
    if(!this->server.listen(this->serverName))
    {
        qWarning().noquote() << "single instance listen failed:" << this->server.errorString();
    }
    return true;
}

bool SingleInstance::NotifyRunningInstance() const
{
    QLocalSocket socket;
    socket.connectToServer(this->serverName);
    if(!socket.waitForConnected(connectTimeoutMs))
    {
        return false;
    }

    socket.write("show\n");
    socket.flush();
    socket.waitForBytesWritten(connectTimeoutMs);
    socket.disconnectFromServer();
    return true;
}

void SingleInstance::HandleIncomingConnection()
{
    bool hasConnection = false;
    while(QLocalSocket* client = this->server.nextPendingConnection())
    {
        hasConnection = true;
        connect(client, &QLocalSocket::disconnected, client, &QObject::deleteLater);
        client->disconnectFromServer();
    }

    if(hasConnection)
    {
        emit ActivateRequested();
    }
}

} // namespace AITalk
