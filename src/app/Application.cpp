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
    this->window.reset();
    if (this->roomEngine)
        this->roomEngine->stop();
}

bool Application::initialize()
{
    const QString cfgFile = AppPaths::configFile();
    this->appConfig = AppConfig::load(cfgFile);

    // 문자열 표는 작고(수 KB) 첫 위젯보다 먼저 있어야 하므로 여기서 바로 읽는다.
    DataManager& data = DataManager::instance();
    QString err;
    if (!data.loadStringTable(QStringLiteral(":/i18n/strings.csv"), &err))
        qWarning().noquote() << "string table:" << err;
    data.setFallbackLanguage(QStringLiteral("ko"));
    data.setLanguage(this->appConfig.language);

    if (!QFile::exists(cfgFile)) {
        // 처음 실행: 이름 기본값도 고른 언어로
        this->appConfig.userName = Text("config.defaultUserName");
        this->appConfig.defaultRoomName = Text("config.defaultRoomName");
        this->appConfig.save(cfgFile);
    }

    ThemeManager::instance().apply(this->appConfig.theme, this->appConfig.fontSize);

    this->agentHub = new AgentHub(this);
    this->agentHub->setConfig(&this->appConfig);

    this->sessionManager = new SessionManager(AppPaths::sessionsDir(), &this->appConfig, this);
    this->sessionManager->load();

    this->roomEngine = new RoomEngine(this->agentHub, &this->appConfig, this);
    connect(this->sessionManager, &SessionManager::currentChanged, this->roomEngine, &RoomEngine::attach);
    connect(this->agentHub, &AgentHub::binsChanged, this->roomEngine, [this] {
        for (const QString& id : Members::ids())
            emit this->roomEngine->memberChanged(id);
    });
    this->roomEngine->attach(this->sessionManager->current());

    connect(qApp, &QCoreApplication::aboutToQuit, this, [this] { this->roomEngine->stop(); });

    this->window = std::make_unique<MainWindow>(this);
    setupTray();
    this->window->show();
    return true;
}

void Application::updateConfig(const AppConfig& config)
{
    const bool themeChanged = config.theme != this->appConfig.theme || config.fontSize != this->appConfig.fontSize;
    this->appConfig = config;
    this->appConfig.save(AppPaths::configFile());
    if (themeChanged)
        ThemeManager::instance().apply(this->appConfig.theme, this->appConfig.fontSize);
    this->agentHub->setConfig(&this->appConfig); // CLI 경로 다시 탐지
    emit configChanged();
}

void Application::showMainWindow()
{
    if (!this->window)
        return;
    this->window->show();
    this->window->setWindowState(this->window->windowState() & ~Qt::WindowMinimized);
    this->window->raise();
    this->window->activateWindow();
}

void Application::quit()
{
    if (this->trayIcon)
        this->trayIcon->hide();
    qApp->quit();
}

void Application::setupTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable())
        return;
    this->trayIcon = new QSystemTrayIcon(QIcon(QStringLiteral(":/icons/app.svg")), this);
    this->trayIcon->setToolTip(QStringLiteral("AITalk"));

    auto* menu = new QMenu(this->window.get());
    menu->addAction(Text("tray.open"), this, &Application::showMainWindow);
    QAction* power = menu->addAction(Text("room.powerOn"));
    connect(power, &QAction::triggered, this, [this] {
        this->roomEngine->isRunning() ? this->roomEngine->stop() : this->roomEngine->start();
    });
    connect(this->roomEngine, &RoomEngine::stateChanged, power, [power](bool running, bool) {
        power->setText(running ? Text("room.powerOff") : Text("room.powerOn"));
    });
    menu->addSeparator();
    menu->addAction(Text("tray.quit"), this, &Application::quit);
    this->trayIcon->setContextMenu(menu);

    connect(this->trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
            showMainWindow();
    });
    this->trayIcon->show();
}
