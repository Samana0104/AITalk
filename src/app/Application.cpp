#include "app/Application.h"

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QMenu>
#include <QSystemTrayIcon>

#include "agent/AgentHub.h"
#include "app/AppPaths.h"
#include "core/Member.h"
#include "engine/RoomEngine.h"
#include "storage/SessionManager.h"
#include "style/ThemeManager.h"
#include "util/DataManager.h"
#include "window/MainWindow.h"

Application::Application(QObject* parent)
    : QObject(parent)
{
}

Application::~Application()
{
    // 창(위젯)이 서비스보다 먼저 사라져야 시그널이 죽은 위젯으로 가지 않는다.
    m_window.reset();
    if (m_engine)
        m_engine->stop();
}

bool Application::initialize()
{
    const QString cfgFile = AppPaths::configFile();
    m_config = AppConfig::load(cfgFile);

    // 문자열 표는 작고(수 KB) 첫 위젯보다 먼저 있어야 하므로 여기서 바로 읽는다.
    DataManager& data = DataManager::instance();
    QString err;
    if (!data.loadStringTable(QStringLiteral(":/i18n/strings.csv"), &err))
        qWarning().noquote() << "string table:" << err;
    data.setFallbackLanguage(QStringLiteral("ko"));
    data.setLanguage(m_config.language);

    if (!QFile::exists(cfgFile)) {
        // 처음 실행: 이름 기본값도 고른 언어로
        m_config.userName = Text("config.defaultUserName");
        m_config.defaultRoomName = Text("config.defaultRoomName");
        m_config.save(cfgFile);
    }

    ThemeManager::instance().apply(m_config.theme, m_config.fontSize);

    m_hub = new AgentHub(this);
    m_hub->setConfig(&m_config);

    m_sessions = new SessionManager(AppPaths::sessionsDir(), &m_config, this);
    m_sessions->load();

    m_engine = new RoomEngine(m_hub, &m_config, this);
    connect(m_sessions, &SessionManager::currentChanged, m_engine, &RoomEngine::attach);
    connect(m_hub, &AgentHub::binsChanged, m_engine, [this] {
        for (const QString& id : Members::ids())
            emit m_engine->memberChanged(id);
    });
    m_engine->attach(m_sessions->current());

    connect(qApp, &QCoreApplication::aboutToQuit, this, [this] { m_engine->stop(); });

    m_window = std::make_unique<MainWindow>(this);
    setupTray();
    m_window->show();
    return true;
}

void Application::updateConfig(const AppConfig& config)
{
    const bool themeChanged = config.theme != m_config.theme || config.fontSize != m_config.fontSize;
    m_config = config;
    m_config.save(AppPaths::configFile());
    if (themeChanged)
        ThemeManager::instance().apply(m_config.theme, m_config.fontSize);
    m_hub->setConfig(&m_config); // CLI 경로 다시 탐지
    emit configChanged();
}

void Application::showMainWindow()
{
    if (!m_window)
        return;
    m_window->show();
    m_window->setWindowState(m_window->windowState() & ~Qt::WindowMinimized);
    m_window->raise();
    m_window->activateWindow();
}

void Application::quit()
{
    if (m_tray)
        m_tray->hide();
    qApp->quit();
}

void Application::setupTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable())
        return;
    m_tray = new QSystemTrayIcon(QIcon(QStringLiteral(":/icons/app.svg")), this);
    m_tray->setToolTip(QStringLiteral("AITalk"));

    auto* menu = new QMenu(m_window.get());
    menu->addAction(Text("tray.open"), this, &Application::showMainWindow);
    QAction* power = menu->addAction(Text("room.powerOn"));
    connect(power, &QAction::triggered, this, [this] {
        m_engine->isRunning() ? m_engine->stop() : m_engine->start();
    });
    connect(m_engine, &RoomEngine::stateChanged, power, [power](bool running, bool) {
        power->setText(running ? Text("room.powerOff") : Text("room.powerOn"));
    });
    menu->addSeparator();
    menu->addAction(Text("tray.quit"), this, &Application::quit);
    m_tray->setContextMenu(menu);

    connect(m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
            showMainWindow();
    });
    m_tray->show();
}
