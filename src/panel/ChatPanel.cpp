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
    , m_app(app)
{
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    m_header = new RoomHeader(this);
    m_messages = new MessageListView(this);
    m_typing = new TypingIndicator(this);
    m_composer = new ComposerWidget(this);
    lay->addWidget(m_header);
    lay->addWidget(m_messages, 1);
    lay->addWidget(m_typing);
    lay->addWidget(m_composer);

    RoomEngine* engine = m_app->engine();

    // ---- 헤더 ----
    connect(m_header, &RoomHeader::renameRequested, this, [this](const QString& t) {
        if (m_session)
            m_session->setTitle(t);
    });
    connect(m_header, &RoomHeader::powerToggled, engine, [engine](bool on) { on ? engine->start() : engine->stop(); });
    connect(m_header, &RoomHeader::speedChanged, this, [this](const QString& s) {
        if (m_session)
            m_session->setSpeed(s);
    });
    connect(m_header, &RoomHeader::boostModeChanged, this, [this](const QString& m) {
        if (m_session)
            m_session->setBoostMode(m);
    });
    connect(m_header, &RoomHeader::exportRequested, this, [this] {
        if (m_session)
            emit exportRequested(m_session);
    });
    connect(m_header, &RoomHeader::inspectorToggled, this, &ChatPanel::inspectorToggled);
    connect(m_header, &RoomHeader::agentClicked, this, &ChatPanel::showAgentMenu);

    // ---- 메시지 / 입력 ----
    connect(m_messages, &MessageListView::replyRequested, this, &ChatPanel::startReply);
    connect(m_messages, &MessageListView::reactionRequested, engine, &RoomEngine::reactAsUser);
    connect(m_composer, &ComposerWidget::sendRequested, this, [this, engine](const QString& text, qint64 replyTo) {
        engine->sendUserMessage(text, replyTo);
        m_messages->scrollToBottom();
    });

    // ---- 엔진 / 세션 ----
    connect(engine, &RoomEngine::stateChanged, this, [this](bool running, bool sleeping) {
        m_header->setRunning(running, sleeping);
        m_composer->setHint(running ? Text("composer.placeholder")
                                    : Text("composer.placeholderOff"));
    });
    connect(engine, &RoomEngine::memberChanged, this, &ChatPanel::refreshMember);
    connect(m_app->sessions(), &SessionManager::currentChanged, this, &ChatPanel::bindSession);
    connect(m_app, &Application::configChanged, this, [this] {
        m_messages->setUserName(m_app->config().userName);
        for (const QString& id : Members::ids())
            refreshMember(id);
    });

    bindSession(m_app->sessions()->current());
    m_header->setRunning(engine->isRunning(), engine->isSleeping());
}

void ChatPanel::setInspectorVisible(bool visible)
{
    m_header->setInspectorVisible(visible);
}

void ChatPanel::bindSession(ChatSession* session)
{
    for (const QMetaObject::Connection& c : std::as_const(m_sessionConns))
        disconnect(c);
    m_sessionConns.clear();
    m_session = session;
    m_composer->clearReply();
    m_messages->setSession(session, m_app->config().userName);
    if (!session)
        return;

    m_sessionConns << connect(session, &ChatSession::messageAdded, m_messages, &MessageListView::appendMessage);
    m_sessionConns << connect(session, &ChatSession::messageUpdated, m_messages, &MessageListView::updateMessage);
    m_sessionConns << connect(session, &ChatSession::metaChanged, this, [this] {
        refreshRoom();
        for (const QString& id : Members::ids())
            refreshMember(id);
    });
    refreshRoom();
    for (const QString& id : Members::ids())
        refreshMember(id);
    m_composer->focusInput();
}

void ChatPanel::refreshRoom()
{
    if (m_session)
        m_header->setRoom(m_session->title(), m_session->speed(), m_session->boostMode());
}

void ChatPanel::refreshMember(const QString& memberId)
{
    const RoomEngine* engine = m_app->engine();
    m_header->setAgentState(memberId, engine->statusOf(memberId), engine->modelLabelOf(memberId),
                            engine->runtime(memberId).deepNow);
    refreshTyping();
}

void ChatPanel::refreshTyping()
{
    const RoomEngine* engine = m_app->engine();
    QList<QPair<QString, QString>> entries;
    for (const QString& id : Members::ids()) {
        const QString st = engine->statusOf(id);
        if (st == QLatin1String("typing"))
            entries.append({ Members::displayName(id, {}), Text("status.typing") });
        else if (st == QLatin1String("reading"))
            entries.append({ Members::displayName(id, {}), engine->runtime(id).deepNow ? Text("status.thinkingHard") : Text("status.reading") });
    }
    m_typing->setEntries(entries);
}

void ChatPanel::showAgentMenu(const QString& memberId)
{
    if (!m_session)
        return;
    const AgentHub* hub = m_app->hub();
    const RoomEngine* engine = m_app->engine();
    const QString name = Members::displayName(memberId, {});

    QMenu menu(this);
    QAction* join = menu.addAction(Text("agentMenu.join").arg(name));
    join->setCheckable(true);
    join->setChecked(m_session->isMemberEnabled(memberId));
    join->setEnabled(hub->isAvailable(memberId));
    connect(join, &QAction::toggled, this, [this, memberId](bool on) {
        if (m_session)
            m_session->setMemberEnabled(memberId, on);
    });

    menu.addSeparator();
    const AgentConfig a = m_app->config().agents.value(memberId);
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

    if (AgentChip* chip = m_header->chip(memberId))
        menu.exec(chip->mapToGlobal(QPoint(0, chip->height() + 4)));
    else
        menu.exec(QCursor::pos());
}

void ChatPanel::startReply(qint64 messageId)
{
    if (!m_session)
        return;
    const ChatMessage* m = m_session->byId(messageId);
    if (!m)
        return;
    m_composer->setReplyTarget(messageId, Members::displayName(m->from, m_app->config().userName), m->displayText().simplified());
}
