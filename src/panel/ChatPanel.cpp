#include "panel/ChatPanel.h"

#include <QAction>
#include <QMenu>
#include <QVBoxLayout>

#include "agent/AgentHub.h"
#include "app/Application.h"
#include "core/Member.h"
#include "engine/RoomEngine.h"
#include "storage/ChatSession.h"
#include "storage/SessionManager.h"
#include "util/DataManager.h"
#include "widget/AgentChip.h"
#include "widget/ComposerWidget.h"
#include "widget/MessageListView.h"
#include "widget/RoomHeader.h"
#include "widget/TypingIndicator.h"

ChatPanel::ChatPanel(Application* app, QWidget* parent)
    : QWidget(parent)
    , app(app)
{
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    this->header = new RoomHeader(this);
    this->messages = new MessageListView(this);
    this->typing = new TypingIndicator(this);
    this->composer = new ComposerWidget(this);
    lay->addWidget(this->header);
    lay->addWidget(this->messages, 1);
    lay->addWidget(this->typing);
    lay->addWidget(this->composer);

    RoomEngine* engine = this->app->engine();

    // ---- 헤더 ----
    connect(this->header, &RoomHeader::renameRequested, this, [this](const QString& t) {
        if (this->session)
            this->session->setTitle(t);
    });
    connect(this->header, &RoomHeader::powerToggled, engine, [engine](bool on) { on ? engine->start() : engine->stop(); });
    connect(this->header, &RoomHeader::speedChanged, this, [this](const QString& s) {
        if (this->session)
            this->session->setSpeed(s);
    });
    connect(this->header, &RoomHeader::boostModeChanged, this, [this](const QString& m) {
        if (this->session)
            this->session->setBoostMode(m);
    });
    connect(this->header, &RoomHeader::exportRequested, this, [this] {
        if (this->session)
            emit exportRequested(this->session);
    });
    connect(this->header, &RoomHeader::inspectorToggled, this, &ChatPanel::inspectorToggled);
    connect(this->header, &RoomHeader::agentClicked, this, &ChatPanel::showAgentMenu);

    // ---- 메시지 / 입력 ----
    connect(this->messages, &MessageListView::replyRequested, this, &ChatPanel::startReply);
    connect(this->messages, &MessageListView::reactionRequested, engine, &RoomEngine::reactAsUser);
    connect(this->composer, &ComposerWidget::sendRequested, this, [this, engine](const QString& text, qint64 replyTo) {
        engine->sendUserMessage(text, replyTo);
        this->messages->scrollToBottom();
    });

    // ---- 엔진 / 세션 ----
    connect(engine, &RoomEngine::stateChanged, this, [this](bool running, bool sleeping) {
        this->header->setRunning(running, sleeping);
        this->composer->setHint(running ? Text("composer.placeholder")
                                    : Text("composer.placeholderOff"));
    });
    connect(engine, &RoomEngine::memberChanged, this, &ChatPanel::refreshMember);
    connect(this->app->sessions(), &SessionManager::currentChanged, this, &ChatPanel::bindSession);
    connect(this->app, &Application::configChanged, this, [this] {
        this->messages->setUserName(this->app->config().userName);
        for (const QString& id : Members::ids())
            refreshMember(id);
    });

    bindSession(this->app->sessions()->current());
    this->header->setRunning(engine->isRunning(), engine->isSleeping());
}

void ChatPanel::setInspectorVisible(bool visible)
{
    this->header->setInspectorVisible(visible);
}

void ChatPanel::bindSession(ChatSession* session)
{
    for (const QMetaObject::Connection& c : std::as_const(this->sessionConnections))
        disconnect(c);
    this->sessionConnections.clear();
    this->session = session;
    this->composer->clearReply();
    this->messages->setSession(session, this->app->config().userName);
    if (!session)
        return;

    this->sessionConnections << connect(session, &ChatSession::messageAdded, this->messages, &MessageListView::appendMessage);
    this->sessionConnections << connect(session, &ChatSession::messageUpdated, this->messages, &MessageListView::updateMessage);
    this->sessionConnections << connect(session, &ChatSession::metaChanged, this, [this] {
        refreshRoom();
        for (const QString& id : Members::ids())
            refreshMember(id);
    });
    refreshRoom();
    for (const QString& id : Members::ids())
        refreshMember(id);
    this->composer->focusInput();
}

void ChatPanel::refreshRoom()
{
    if (this->session)
        this->header->setRoom(this->session->title(), this->session->speed(), this->session->boostMode());
}

void ChatPanel::refreshMember(const QString& memberId)
{
    const RoomEngine* engine = this->app->engine();
    this->header->setAgentState(memberId, engine->statusOf(memberId), engine->modelLabelOf(memberId),
                            engine->runtime(memberId).deepNow);
    refreshTyping();
}

void ChatPanel::refreshTyping()
{
    const RoomEngine* engine = this->app->engine();
    QList<QPair<QString, QString>> entries;
    for (const QString& id : Members::ids()) {
        const QString st = engine->statusOf(id);
        if (st == QLatin1String("typing"))
            entries.append({ Members::displayName(id, {}), Text("status.typing") });
        else if (st == QLatin1String("reading"))
            entries.append({ Members::displayName(id, {}), engine->runtime(id).deepNow ? Text("status.thinkingHard") : Text("status.reading") });
    }
    this->typing->setEntries(entries);
}

void ChatPanel::showAgentMenu(const QString& memberId)
{
    if (!this->session)
        return;
    const AgentHub* hub = this->app->hub();
    const RoomEngine* engine = this->app->engine();
    const QString name = Members::displayName(memberId, {});

    QMenu menu(this);
    QAction* join = menu.addAction(Text("agentMenu.join").arg(name));
    join->setCheckable(true);
    join->setChecked(this->session->isMemberEnabled(memberId));
    join->setEnabled(hub->isAvailable(memberId));
    connect(join, &QAction::toggled, this, [this, memberId](bool on) {
        if (this->session)
            this->session->setMemberEnabled(memberId, on);
    });

    menu.addSeparator();
    const AgentConfig a = this->app->config().agents.value(memberId);
    menu.addAction(Text("agentMenu.model").arg(a.label(false)))->setEnabled(false);
    if (a.hasBoost())
        menu.addAction(Text("agentMenu.boost").arg(a.label(true)))->setEnabled(false);
    const QString bin = hub->binOf(memberId);
    menu.addAction(bin.isEmpty() ? Text("agentMenu.noCli") : Text("agentMenu.cli").arg(bin))->setEnabled(false);
    const QString err = engine->runtime(memberId).lastError;
    if (!err.isEmpty())
        menu.addAction(Text("agentMenu.lastError").arg(err.left(80)))->setEnabled(false);
    menu.addSeparator();
    menu.addAction(Text("agentMenu.settings"), this, [this, memberId] { emit agentSettingsRequested(memberId); });

    if (AgentChip* chip = this->header->chip(memberId))
        menu.exec(chip->mapToGlobal(QPoint(0, chip->height() + 4)));
    else
        menu.exec(QCursor::pos());
}

void ChatPanel::startReply(qint64 messageId)
{
    if (!this->session)
        return;
    const ChatMessage* m = this->session->byId(messageId);
    if (!m)
        return;
    this->composer->setReplyTarget(messageId, Members::displayName(m->from, this->app->config().userName), m->displayText().simplified());
}
