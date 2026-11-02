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

    const AppConfig& config() const { return m_config; }
    void updateConfig(const AppConfig& config);

    AgentHub* hub() const { return m_hub; }
    SessionManager* sessions() const { return m_sessions; }
    RoomEngine* engine() const { return m_engine; }
    QSystemTrayIcon* tray() const { return m_tray; }

    void showMainWindow();
    void quit();

signals:
    void configChanged();

private:
    void setupTray();

    AppConfig m_config;
    AgentHub* m_hub = nullptr;
    SessionManager* m_sessions = nullptr;
    RoomEngine* m_engine = nullptr;
    QSystemTrayIcon* m_tray = nullptr;
    std::unique_ptr<MainWindow> m_window;
};
