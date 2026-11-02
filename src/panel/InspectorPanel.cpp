#include "panel/InspectorPanel.h"

#include <QComboBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QVBoxLayout>

#include "agent/AgentHub.h"
#include "app/Application.h"
#include "core/Member.h"
#include "engine/RoomEngine.h"
#include "storage/ChatSession.h"
#include "storage/SessionManager.h"
#include "util/DataManager.h"
#include "widget/MemberCard.h"

InspectorPanel::InspectorPanel(Application* app, QWidget* parent)
    : QWidget(parent)
    , m_app(app)
{
    setObjectName(QStringLiteral("inspectorPanel"));
    setAttribute(Qt::WA_StyledBackground);
    setMinimumWidth(240);

    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(10, 10, 10, 10);
    auto* tabs = new QTabWidget(this);
    tabs->setDocumentMode(true);
    lay->addWidget(tabs);

    // ---- 멤버 ----
    auto* scroll = new QScrollArea(tabs);
    scroll->setObjectName(QStringLiteral("inspectorScroll"));
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* canvas = new QWidget(scroll);
    canvas->setObjectName(QStringLiteral("inspectorCanvas"));
    auto* cards = new QVBoxLayout(canvas);
    cards->setContentsMargins(0, 8, 0, 0);
    cards->setSpacing(8);
    for (const QString& id : Members::ids()) {
        auto* card = new MemberCard(id, canvas);
        m_cards.insert(id, card);
        cards->addWidget(card);
    }
    cards->addStretch(1);
    scroll->setWidget(canvas);
    tabs->addTab(scroll, Text("inspector.tabMembers"));

    // ---- 메모 ----
    auto* notePage = new QWidget(tabs);
    auto* nl = new QVBoxLayout(notePage);
    nl->setContentsMargins(0, 8, 0, 0);
    nl->setSpacing(8);
    m_noteMember = new QComboBox(notePage);
    for (const MemberInfo& m : Members::all())
        m_noteMember->addItem(m.name, m.id);
    auto* hint = new QLabel(Text("inspector.notesHint"), notePage);
    hint->setObjectName(QStringLiteral("dialogHint"));
    hint->setWordWrap(true);
    m_note = new QPlainTextEdit(notePage);
    m_note->setObjectName(QStringLiteral("noteEditor"));
    m_note->setPlaceholderText(Text("inspector.notesEmpty"));
    m_saveNote = new QPushButton(Text("inspector.saveNote"), notePage);
    m_saveNote->setEnabled(false);
    nl->addWidget(m_noteMember);
    nl->addWidget(hint);
    nl->addWidget(m_note, 1);
    nl->addWidget(m_saveNote, 0, Qt::AlignRight);
    tabs->addTab(notePage, Text("inspector.tabNotes"));

    connect(m_noteMember, &QComboBox::currentIndexChanged, this, &InspectorPanel::loadNote);
    connect(m_note, &QPlainTextEdit::textChanged, this, [this] {
        m_noteDirty = true;
        m_saveNote->setEnabled(true);
    });
    connect(m_saveNote, &QPushButton::clicked, this, &InspectorPanel::saveNote);

    connect(m_app->engine(), &RoomEngine::memberChanged, this, &InspectorPanel::refreshMember);
    connect(m_app->sessions(), &SessionManager::currentChanged, this, &InspectorPanel::bindSession);
    connect(m_app, &Application::configChanged, this, [this] {
        for (const QString& id : Members::ids())
            refreshMember(id);
    });
    bindSession(m_app->sessions()->current());
}

void InspectorPanel::bindSession(ChatSession* session)
{
    disconnect(m_noteConn);
    m_session = session;
    if (session) {
        m_noteConn = connect(session, &ChatSession::noteChanged, this, [this](const QString& id) {
            if (id == m_noteMember->currentData().toString() && !m_noteDirty)
                loadNote();
        });
    }
    loadNote();
    for (const QString& id : Members::ids())
        refreshMember(id);
}

void InspectorPanel::refreshMember(const QString& memberId)
{
    MemberCard* card = m_cards.value(memberId);
    if (!card)
        return;
    const RoomEngine* engine = m_app->engine();
    const MemberRuntime& rt = engine->runtime(memberId);
    MemberCard::State s;
    s.status = engine->statusOf(memberId);
    s.model = engine->modelLabelOf(memberId);
    s.cliPath = m_app->hub()->binOf(memberId);
    s.calls = rt.calls;
    s.lastMs = rt.lastMs;
    s.lastError = rt.lastError;
    card->setState(s);
}

void InspectorPanel::loadNote()
{
    const QSignalBlocker block(m_note);
    m_note->setPlainText(m_session ? m_session->readNote(m_noteMember->currentData().toString()) : QString());
    m_noteDirty = false;
    m_saveNote->setEnabled(false);
}

void InspectorPanel::saveNote()
{
    if (!m_session)
        return;
    m_noteDirty = false;
    m_saveNote->setEnabled(false);
    m_session->writeNote(m_noteMember->currentData().toString(), m_note->toPlainText());
}
