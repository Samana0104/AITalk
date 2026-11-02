#pragma once

#include <QObject>
#include <memory>

#include "core/AppConfig.h"

class AgentHub;
class MainWindow;
class QSystemTrayIcon;
class RoomEngine;
class SessionManager;

// 앱 전체의 조립 지점. 설정·서비스(AgentHub, SessionManager, RoomEngine)를 만들고
// 메인 창과 트레이 아이콘을 띄운다. UI 쪽은 이 객체를 통해 서비스에 접근한다.
class Application : public QObject {
    Q_OBJECT
public:
    explicit Application(QObject* parent = nullptr);
    ~Application() override;

    bool initialize();

    const AppConfig& config() const { return this->appConfig; }
    void updateConfig(const AppConfig& config);

    AgentHub* hub() const { return this->agentHub; }
    SessionManager* sessions() const { return this->sessionManager; }
    RoomEngine* engine() const { return this->roomEngine; }
    QSystemTrayIcon* tray() const { return this->trayIcon; }

    void showMainWindow();
    void quit();

signals:
    void configChanged();

private:
    void setupTray();

    AppConfig appConfig;
    AgentHub* agentHub = nullptr;
    SessionManager* sessionManager = nullptr;
    RoomEngine* roomEngine = nullptr;
    QSystemTrayIcon* trayIcon = nullptr;
    std::unique_ptr<MainWindow> window;
};
