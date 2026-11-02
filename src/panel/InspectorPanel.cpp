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
    , app(app)
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
        this->cards.insert(id, card);
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
    this->noteMember = new QComboBox(notePage);
    for (const MemberInfo& m : Members::all())
        this->noteMember->addItem(m.name, m.id);
    auto* hint = new QLabel(Text("inspector.notesHint"), notePage);
    hint->setObjectName(QStringLiteral("dialogHint"));
    hint->setWordWrap(true);
    this->note = new QPlainTextEdit(notePage);
    this->note->setObjectName(QStringLiteral("noteEditor"));
    this->note->setPlaceholderText(Text("inspector.notesEmpty"));
    this->saveNoteButton = new QPushButton(Text("inspector.saveNote"), notePage);
    this->saveNoteButton->setEnabled(false);
    nl->addWidget(this->noteMember);
    nl->addWidget(hint);
    nl->addWidget(this->note, 1);
    nl->addWidget(this->saveNoteButton, 0, Qt::AlignRight);
    tabs->addTab(notePage, Text("inspector.tabNotes"));

    connect(this->noteMember, &QComboBox::currentIndexChanged, this, &InspectorPanel::loadNote);
    connect(this->note, &QPlainTextEdit::textChanged, this, [this] {
        this->noteDirty = true;
        this->saveNoteButton->setEnabled(true);
    });
    connect(this->saveNoteButton, &QPushButton::clicked, this, &InspectorPanel::saveNote);

    connect(this->app->engine(), &RoomEngine::memberChanged, this, &InspectorPanel::refreshMember);
    connect(this->app->sessions(), &SessionManager::currentChanged, this, &InspectorPanel::bindSession);
    connect(this->app, &Application::configChanged, this, [this] {
        for (const QString& id : Members::ids())
            refreshMember(id);
    });
    bindSession(this->app->sessions()->current());
}

void InspectorPanel::bindSession(ChatSession* session)
{
    disconnect(this->noteConnection);
    this->session = session;
    if (session) {
        this->noteConnection = connect(session, &ChatSession::noteChanged, this, [this](const QString& id) {
            if (id == this->noteMember->currentData().toString() && !this->noteDirty)
                loadNote();
        });
    }
    loadNote();
    for (const QString& id : Members::ids())
        refreshMember(id);
}

void InspectorPanel::refreshMember(const QString& memberId)
{
    MemberCard* card = this->cards.value(memberId);
    if (!card)
        return;
    const RoomEngine* engine = this->app->engine();
    const MemberRuntime& rt = engine->runtime(memberId);
    MemberCard::State s;
    s.status = engine->statusOf(memberId);
    s.model = engine->modelLabelOf(memberId);
    s.cliPath = this->app->hub()->binOf(memberId);
    s.calls = rt.calls;
    s.lastMs = rt.lastMs;
    s.lastError = rt.lastError;
    card->setState(s);
}

void InspectorPanel::loadNote()
{
    const QSignalBlocker block(this->note);
    this->note->setPlainText(this->session ? this->session->readNote(this->noteMember->currentData().toString()) : QString());
    this->noteDirty = false;
    this->saveNoteButton->setEnabled(false);
}

void InspectorPanel::saveNote()
{
    if (!this->session)
        return;
    this->noteDirty = false;
    this->saveNoteButton->setEnabled(false);
    this->session->writeNote(this->noteMember->currentData().toString(), this->note->toPlainText());
}
