#include "ui/UiBridge.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QGuiApplication>
#include <QQmlEngine>
#include <QStandardPaths>
#include <QSystemTrayIcon>

#include "agent/AgentHub.h"
#include "app/AppPaths.h"
#include "app/Application.h"
#include "core/Member.h"
#include "dialog/DoctorDialog.h"
#include "dialog/SettingsDialog.h"
#include "engine/RoomEngine.h"
#include "storage/ChatExporter.h"
#include "storage/ChatSession.h"
#include "manager/SessionManager.h"
#include "manager/ThemeManager.h"
#include "manager/DataManager.h"

UiBridge::UiBridge(Application* app, QObject* parent)
    : QObject(parent)
    , app(app)
    , messageModel(new MessageListModel(this))
    , sessionModel(new SessionListModel(app->sessions(), this))
    , memberModel(new MemberListModel(app->engine(), app->hub(), this))
{
    instance = this;

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &UiBridge::ThemeChanged);
    connect(app, &Application::configChanged, this, [this]
    {
        this->messageModel->SetUserName(UserName());
        this->memberModel->RefreshAll();
        emit ConfigChanged();
    });
    connect(app->engine(), &RoomEngine::stateChanged, this, &UiBridge::RoomStateChanged);
    connect(app->engine(), &RoomEngine::memberChanged, this->memberModel, &MemberListModel::Refresh);
    connect(app->sessions(), &SessionManager::currentChanged, this, &UiBridge::BindSession);
    BindSession(app->sessions()->current());
}

UiBridge* UiBridge::create(QQmlEngine* qmlEngine, QJSEngine* jsEngine)
{
    Q_UNUSED(qmlEngine);
    Q_UNUSED(jsEngine);
    // Application 이 소유한다. QML 엔진이 지우지 않게 한다.
    QJSEngine::setObjectOwnership(instance, QJSEngine::CppOwnership);
    return instance;
}

void UiBridge::BindSession(ChatSession* session)
{
    for(const QMetaObject::Connection& connection : std::as_const(this->sessionConnections))
    {
        disconnect(connection);
    }
    this->sessionConnections.clear();
    this->messageModel->SetSession(session, UserName());
    if(session)
    {
        this->sessionConnections << connect(session, &ChatSession::metaChanged, this, [this]
        {
            this->memberModel->RefreshAll();
            emit RoomChanged();
        });
        this->sessionConnections << connect(session, &ChatSession::noteChanged, this, &UiBridge::noteChanged);
    }
    this->memberModel->RefreshAll();
    emit RoomChanged();
    emit sessionChanged();
}

ChatSession* UiBridge::CurrentSession() const
{
    return this->app->sessions()->current();
}

QVariantMap UiBridge::Colors() const
{
    return ThemeManager::instance().Tokens();
}

bool UiBridge::IsDark() const
{
    return ThemeManager::instance().theme() == QLatin1String("dark");
}

int UiBridge::FontSize() const
{
    return qBound(8, this->app->config().fontSize, 18);
}

QString UiBridge::UserName() const
{
    return this->app->config().userName;
}

bool UiBridge::IsRunning() const
{
    return this->app->engine()->isRunning();
}

bool UiBridge::IsSleeping() const
{
    return this->app->engine()->isSleeping();
}

QString UiBridge::RoomTitle() const
{
    return CurrentSession() ? CurrentSession()->title() : QString();
}

QString UiBridge::RoomSpeed() const
{
    return CurrentSession() ? CurrentSession()->speed() : QString();
}

QString UiBridge::RoomBoostMode() const
{
    return CurrentSession() ? CurrentSession()->boostMode() : QString();
}

QUrl UiBridge::UiSettingsUrl() const
{
    return QUrl::fromLocalFile(AppPaths::uiSettingsFile());
}

QString UiBridge::Localize(const QString& key) const
{
    return Text(key);
}

void UiBridge::SendMessage(const QString& text, qint64 replyTo)
{
    const QString trimmed = text.trimmed();
    if(!trimmed.isEmpty())
    {
        this->app->engine()->sendUserMessage(trimmed, replyTo);
    }
}

void UiBridge::React(qint64 messageId, const QString& emoji)
{
    this->app->engine()->reactAsUser(messageId, emoji);
}

void UiBridge::SetRunning(bool isOn)
{
    isOn ? this->app->engine()->start() : this->app->engine()->stop();
}

void UiBridge::RenameRoom(const QString& title)
{
    const QString trimmed = title.trimmed();
    if(CurrentSession() && !trimmed.isEmpty())
    {
        CurrentSession()->setTitle(trimmed);
    }
}

void UiBridge::SetSpeed(const QString& speed)
{
    if(CurrentSession())
    {
        CurrentSession()->setSpeed(speed);
    }
}

void UiBridge::SetBoostMode(const QString& mode)
{
    if(CurrentSession())
    {
        CurrentSession()->setBoostMode(mode);
    }
}

void UiBridge::SetMemberEnabled(const QString& memberId, bool isEnabled)
{
    if(CurrentSession())
    {
        CurrentSession()->setMemberEnabled(memberId, isEnabled);
    }
}

QVariantMap UiBridge::AgentInfo(const QString& memberId) const
{
    const AgentHub* hub = this->app->hub();
    const AgentConfig agent = this->app->config().agents.value(memberId);
    const QString name = Members::displayName(memberId, {});
    const QString bin = hub->binOf(memberId);
    const QString error = this->app->engine()->runtime(memberId).lastError;
    return {
        { QStringLiteral("joinText"), Text("agentMenu.join").arg(name) },
        { QStringLiteral("isEnabled"), CurrentSession() && CurrentSession()->isMemberEnabled(memberId) },
        { QStringLiteral("isAvailable"), hub->isAvailable(memberId) },
        { QStringLiteral("modelText"), Text("agentMenu.model").arg(agent.label(false)) },
        { QStringLiteral("boostText"), agent.hasBoost() ? Text("agentMenu.boost").arg(agent.label(true)) : QString() },
        { QStringLiteral("cliText"), bin.isEmpty() ? Text("agentMenu.noCli") : Text("agentMenu.cli").arg(bin) },
        { QStringLiteral("errorText"), error.isEmpty() ? QString() : Text("agentMenu.lastError").arg(error.left(80)) },
    };
}

QVariantList UiBridge::MentionCandidates(const QString& query) const
{
    const QString lowered = query.toLower();
    QVariantList out;
    for(const MemberInfo& info : Members::all())
    {
        bool isHit = lowered.isEmpty() || info.name.toLower().startsWith(lowered);
        for(const QString& alias : info.aliases)
        {
            isHit = isHit || alias.startsWith(lowered);
        }
        if(isHit)
        {
            out.append(QVariantMap {
                { QStringLiteral("memberId"), info.id },
                { QStringLiteral("name"), info.name },
                { QStringLiteral("memberColor"), info.color },
            });
        }
    }
    return out;
}

QString UiBridge::ReadNote(const QString& memberId) const
{
    return CurrentSession() ? CurrentSession()->readNote(memberId) : QString();
}

void UiBridge::WriteNote(const QString& memberId, const QString& text)
{
    if(CurrentSession())
    {
        CurrentSession()->writeNote(memberId, text);
    }
}

void UiBridge::CreateSession()
{
    SessionManager* sessions = this->app->sessions();
    sessions->setCurrent(sessions->createSession());
}

void UiBridge::SelectSession(const QString& sessionId)
{
    SessionManager* sessions = this->app->sessions();
    if(ChatSession* session = sessions->find(sessionId))
    {
        sessions->setCurrent(session);
    }
}

void UiBridge::RenameSession(const QString& sessionId, const QString& title)
{
    SessionManager* sessions = this->app->sessions();
    const QString trimmed = title.trimmed();
    if(ChatSession* session = sessions->find(sessionId); session && !trimmed.isEmpty())
    {
        sessions->renameSession(session, trimmed);
    }
}

void UiBridge::DeleteSession(const QString& sessionId)
{
    SessionManager* sessions = this->app->sessions();
    if(ChatSession* session = sessions->find(sessionId))
    {
        sessions->removeSession(session);
    }
}

QString UiBridge::SessionTitle(const QString& sessionId) const
{
    const ChatSession* session = this->app->sessions()->find(sessionId);
    return session ? session->title() : QString();
}

void UiBridge::ExportSession(const QString& sessionId)
{
    const ChatSession* session = sessionId.isEmpty() ? CurrentSession() : this->app->sessions()->find(sessionId);
    if(!session)
    {
        return;
    }
    const QString documents = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString file = QFileDialog::getSaveFileName(nullptr, Text("common.exportMarkdown"),
                                                      QDir(documents).filePath(ChatExporter::suggestedFileName(*session)),
                                                      QStringLiteral("Markdown (*.md)"));
    if(file.isEmpty())
    {
        return;
    }
    QString error;
    if(ChatExporter::exportMarkdown(*session, this->app->config().userName, file, &error))
    {
        emit toastRequested(Text("toast.exported").arg(QDir::toNativeSeparators(file)));
    }
    else
    {
        emit toastRequested(Text("toast.exportFailed").arg(error));
    }
}

void UiBridge::CopyText(const QString& text)
{
    QGuiApplication::clipboard()->setText(text);
}

void UiBridge::OpenUrl(const QString& url)
{
    QDesktopServices::openUrl(QUrl(url));
}

// shortcut: 설정·진단은 아직 위젯 다이얼로그다. QML 로 옮기면 이 두 함수와 Widgets 의존을 정리한다.
void UiBridge::OpenSettings(const QString& agentId)
{
    SettingsDialog dialog(this->app->config(), this->app->hub(), nullptr);
    if(!agentId.isEmpty())
    {
        dialog.focusAgent(agentId);
    }
    if(dialog.exec() == QDialog::Accepted)
    {
        const bool isLanguageChanged = dialog.editedConfig().language != this->app->config().language;
        this->app->updateConfig(dialog.editedConfig());
        // 문자열은 화면을 만들 때 박히므로 언어는 다시 시작해야 바뀐다.
        emit toastRequested(isLanguageChanged ? Text("toast.restartForLanguage") : Text("toast.settingsSaved"));
    }
}

void UiBridge::OpenDoctor()
{
    DoctorDialog dialog(this->app->hub(), nullptr);
    dialog.exec();
}

void UiBridge::RescanCli()
{
    this->app->hub()->refreshBins();
    emit toastRequested(Text("toast.cliRescanned"));
}

bool UiBridge::RequestClose()
{
    QSystemTrayIcon* tray = this->app->tray();
    if(this->app->config().closeToTray && tray && tray->isVisible())
    {
        if(!this->isTrayHintShown)
        {
            tray->showMessage(QStringLiteral("AITalk"), Text("tray.stillRunning"));
            this->isTrayHintShown = true;
        }
        return false;
    }
    this->app->quit();
    return true;
}
