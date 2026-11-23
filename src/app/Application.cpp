#include "app/Application.h"

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QMenu>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSystemTrayIcon>

#include "agent/AgentHub.h"
#include "app/AppPaths.h"
#include "core/Member.h"
#include "engine/RoomEngine.h"
#include "manager/SessionManager.h"
#include "manager/ThemeManager.h"
#include "ui/AvatarImageProvider.h"
#include "ui/IconImageProvider.h"
#include "ui/UiBridge.h"
#include "manager/DataManager.h"

namespace AITalk
{

Application::Application(QObject* parent)
    : QObject(parent)
{
}

Application::~Application()
{
    // QML 창이 서비스보다 먼저 사라져야 시그널이 죽은 화면으로 가지 않는다.
    this->qmlEngine.reset();
    if (this->roomEngine)
        this->roomEngine->stop();
}

bool Application::initialize()
{
    const QString cfgFile = AppPaths::configFile();
    this->appConfig = AppConfig::load(cfgFile);

    // 언어별 문자열 표를 첫 위젯보다 먼저 읽는다.
    DataManager& data = DataManager::instance();
    QString err;
    if(!data.LoadLocales(QStringLiteral(":/locale"), &err))
    {
        qWarning().noquote() << "string table:" << err;
        return false;
    }
    data.setFallbackLanguage(QStringLiteral("ko-kr"));
    data.setLanguage(this->appConfig.language);

    // 처음 실행: 이름 기본값도 고른 언어로. 배포용 config/config.json 은 이름을 비워 둔다.
    const bool hasConfigFile = QFile::exists(cfgFile);
    const bool isUserNameMissing = !hasConfigFile || this->appConfig.userName.isEmpty();
    const bool isRoomNameMissing = !hasConfigFile || this->appConfig.defaultRoomName.isEmpty();
    if(isUserNameMissing)
    {
        this->appConfig.userName = Text("config.defaultUserName");
    }
    if(isRoomNameMissing)
    {
        this->appConfig.defaultRoomName = Text("config.defaultRoomName");
    }
    if(isUserNameMissing || isRoomNameMissing)
    {
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

    this->uiBridge = new UiBridge(this, this);
    this->qmlEngine = std::make_unique<QQmlApplicationEngine>();
    this->qmlEngine->addImageProvider(QStringLiteral("icon"), new IconImageProvider);
    this->qmlEngine->addImageProvider(QStringLiteral("avatar"), new AvatarImageProvider);
    this->qmlEngine->loadFromModule("AITalk", "Main");
    if(!MainWindow())
    {
        qWarning() << "failed to load qml/Main.qml";
        return false;
    }
    setupTray();
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

QQuickWindow* Application::MainWindow() const
{
    if(!this->qmlEngine || this->qmlEngine->rootObjects().isEmpty())
    {
        return nullptr;
    }
    return qobject_cast<QQuickWindow*>(this->qmlEngine->rootObjects().constFirst());
}

void Application::showMainWindow()
{
    QQuickWindow* window = MainWindow();
    if(!window)
    {
        return;
    }
    window->show();
    window->setWindowStates(window->windowStates() & ~Qt::WindowMinimized);
    window->raise();
    window->requestActivate();
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

    this->trayMenu = std::make_unique<QMenu>();
    QMenu* menu = this->trayMenu.get();
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

} // namespace AITalk
