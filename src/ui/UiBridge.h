#pragma once

#include <QList>
#include <QObject>
#include <QUrl>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include "ui/MemberListModel.h"
#include "ui/MessageListModel.h"
#include "ui/SessionListModel.h"

class QJSEngine;
class QQmlEngine;

namespace AITalk
{

class Application;
class ChatSession;

// QML 쪽 창구 (QML 싱글턴 App). 서비스(Application)의 상태·동작을 QML 속성과 호출로 풀어 준다.
class UiBridge : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(App)
    QML_SINGLETON
    Q_PROPERTY(QVariantMap colors READ Colors NOTIFY ThemeChanged)
    Q_PROPERTY(bool isDark READ IsDark NOTIFY ThemeChanged)
    Q_PROPERTY(int fontSize READ FontSize NOTIFY ThemeChanged)
    Q_PROPERTY(QString userName READ UserName NOTIFY ConfigChanged)
    Q_PROPERTY(bool isRunning READ IsRunning NOTIFY RoomStateChanged)
    Q_PROPERTY(bool isSleeping READ IsSleeping NOTIFY RoomStateChanged)
    Q_PROPERTY(QString roomTitle READ RoomTitle NOTIFY RoomChanged)
    Q_PROPERTY(QString roomSpeed READ RoomSpeed NOTIFY RoomChanged)
    Q_PROPERTY(QString roomBoostMode READ RoomBoostMode NOTIFY RoomChanged)
    // QML 이 타입을 찾도록 네임스페이스까지 적는다.
    Q_PROPERTY(AITalk::MessageListModel* messageModel READ MessageModel CONSTANT)
    Q_PROPERTY(AITalk::SessionListModel* sessionModel READ SessionModel CONSTANT)
    Q_PROPERTY(AITalk::MemberListModel* memberModel READ MemberModel CONSTANT)
    Q_PROPERTY(QUrl uiSettingsUrl READ UiSettingsUrl CONSTANT)

public:
    explicit UiBridge(Application* app, QObject* parent = nullptr);

    // QML 엔진이 싱글턴을 처음 쓸 때 부른다. Application 이 먼저 만든 객체를 돌려준다.
    static UiBridge* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

    QVariantMap Colors() const;
    bool IsDark() const;
    int FontSize() const;
    QString UserName() const;
    bool IsRunning() const;
    bool IsSleeping() const;
    QString RoomTitle() const;
    QString RoomSpeed() const;
    QString RoomBoostMode() const;
    MessageListModel* MessageModel() const { return this->messageModel; }
    SessionListModel* SessionModel() const { return this->sessionModel; }
    MemberListModel* MemberModel() const { return this->memberModel; }
    QUrl UiSettingsUrl() const;

    Q_INVOKABLE QString Localize(const QString& key) const;

    // 방
    Q_INVOKABLE void SendMessage(const QString& text, qint64 replyTo);
    Q_INVOKABLE void React(qint64 messageId, const QString& emoji);
    Q_INVOKABLE void SetRunning(bool isOn);
    Q_INVOKABLE void RenameRoom(const QString& title);
    Q_INVOKABLE void SetSpeed(const QString& speed);
    Q_INVOKABLE void SetBoostMode(const QString& mode);
    Q_INVOKABLE void SetMemberEnabled(const QString& memberId, bool isEnabled);
    Q_INVOKABLE QVariantMap AgentInfo(const QString& memberId) const;
    Q_INVOKABLE QVariantList MentionCandidates(const QString& query) const;
    Q_INVOKABLE QString ReadNote(const QString& memberId) const;
    Q_INVOKABLE void WriteNote(const QString& memberId, const QString& text);

    // 세션
    Q_INVOKABLE void CreateSession();
    Q_INVOKABLE void SelectSession(const QString& sessionId);
    Q_INVOKABLE void RenameSession(const QString& sessionId, const QString& title);
    Q_INVOKABLE void DeleteSession(const QString& sessionId);
    Q_INVOKABLE void ExportSession(const QString& sessionId);
    Q_INVOKABLE QString SessionTitle(const QString& sessionId) const;

    // 앱
    Q_INVOKABLE void CopyText(const QString& text);
    Q_INVOKABLE void OpenUrl(const QString& url);
    Q_INVOKABLE void OpenSettings(const QString& agentId = QString());
    Q_INVOKABLE void OpenDoctor();
    Q_INVOKABLE void RescanCli();
    // 창 닫기 버튼. 트레이로 숨겨야 하면 false (창은 QML 이 숨긴다).
    Q_INVOKABLE bool RequestClose();

signals:
    void ThemeChanged();
    void ConfigChanged();
    void RoomStateChanged();
    void RoomChanged();
    // 아래 셋은 QML 이 onSessionChanged 처럼 핸들러로 받는다. QML 핸들러는 대문자로 시작하는
    // signal 과 연결되지 않으므로 이 셋만 camelCase 로 둔다.
    void sessionChanged();
    void noteChanged(const QString& memberId);
    void toastRequested(const QString& text);

private:
    void BindSession(ChatSession* session);
    ChatSession* CurrentSession() const;

private:
    static inline UiBridge* instance = nullptr;

    Application* app;
    MessageListModel* messageModel;
    SessionListModel* sessionModel;
    MemberListModel* memberModel;
    QList<QMetaObject::Connection> sessionConnections;
    bool isTrayHintShown = false;
};

} // namespace AITalk
