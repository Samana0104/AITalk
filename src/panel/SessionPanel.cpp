#include "panel/SessionPanel.h"

#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QVBoxLayout>

#include "component/ChipButton.h"
#include "core/Member.h"
#include "storage/ChatSession.h"
#include "storage/SessionManager.h"
#include "style/ThemeManager.h"
#include "util/DataManager.h"
#include "widget/SessionItemWidget.h"

namespace {
constexpr int sessionRole = Qt::UserRole;
constexpr int messageRole = Qt::UserRole + 1;

QString previewOf(const ChatSession* s)
{
    static const QRegularExpression markdownMarks(QStringLiteral("[*_`#>~]+"));
    const QList<ChatMessage>& all = s->messages();
    for (qsizetype i = all.size() - 1; i >= 0; --i) {
        const ChatMessage& m = all.at(i);
        if (!m.isSystem() && !m.displayText().isEmpty())
            return QStringLiteral("%1: %2").arg(Members::displayName(m.from, Text("common.me")),
                                                m.displayText().remove(markdownMarks).simplified());
    }
    return {};
}
}

SessionPanel::SessionPanel(SessionManager* sessions, QWidget* parent)
    : QWidget(parent)
    , sessions(sessions)
{
    setObjectName(QStringLiteral("sessionPanel"));
    setAttribute(Qt::WA_StyledBackground);
    setMinimumWidth(200);

    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(10, 12, 10, 8);
    lay->setSpacing(8);

    auto* head = new QHBoxLayout;
    auto* title = new QLabel(Text("sessions.title"), this);
    title->setObjectName(QStringLiteral("panelTitle"));
    auto* newBtn = new ChipButton(QStringLiteral("plus"), Text("sessions.new"), this);
    head->addWidget(title);
    head->addStretch(1);
    head->addWidget(newBtn);
    lay->addLayout(head);

    this->search = new QLineEdit(this);
    this->search->setPlaceholderText(Text("sessions.search"));
    this->search->setClearButtonEnabled(true);
    auto refreshSearchIcon = [this] {
        const QList<QAction*> acts = this->search->actions();
        for (QAction* a : acts)
            if (a->objectName() == QLatin1String("searchIcon"))
                this->search->removeAction(a);
        QAction* a = this->search->addAction(ThemeManager::instance().icon(QStringLiteral("search"), QStringLiteral("muted")), QLineEdit::LeadingPosition);
        a->setObjectName(QStringLiteral("searchIcon"));
    };
    refreshSearchIcon();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, refreshSearchIcon);
    lay->addWidget(this->search);

    this->list = new QListWidget(this);
    this->list->setObjectName(QStringLiteral("sessionList"));
    this->list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->list->setContextMenuPolicy(Qt::CustomContextMenu);
    this->list->setSpacing(1);
    lay->addWidget(this->list, 1);

    auto* foot = new QHBoxLayout;
    auto* doctor = new QPushButton(Text("sessions.doctor"), this);
    doctor->setProperty("variant", "text");
    auto* rescan = new QPushButton(Text("sessions.rescan"), this);
    rescan->setProperty("variant", "text");
    rescan->setToolTip(Text("sessions.rescanTip"));
    foot->addWidget(doctor);
    foot->addWidget(rescan);
    foot->addStretch(1);
    lay->addLayout(foot);

    this->rebuildTimer.setSingleShot(true);
    this->rebuildTimer.setInterval(150);
    connect(&this->rebuildTimer, &QTimer::timeout, this, &SessionPanel::rebuild);

    connect(newBtn, &QPushButton::clicked, this, [this] {
        this->search->clear();
        this->sessions->setCurrent(this->sessions->createSession());
    });
    connect(this->search, &QLineEdit::textChanged, this, &SessionPanel::scheduleRebuild);
    connect(this->list, &QListWidget::itemClicked, this, &SessionPanel::onItemClicked);
    connect(this->list, &QWidget::customContextMenuRequested, this, &SessionPanel::showContextMenu);
    connect(doctor, &QPushButton::clicked, this, &SessionPanel::doctorRequested);
    connect(rescan, &QPushButton::clicked, this, &SessionPanel::refreshCliRequested);
    connect(this->sessions, &SessionManager::sessionsChanged, this, &SessionPanel::scheduleRebuild);
    connect(this->sessions, &SessionManager::currentChanged, this, &SessionPanel::scheduleRebuild);

    rebuild();
}

void SessionPanel::scheduleRebuild()
{
    this->rebuildTimer.start();
}

void SessionPanel::rebuild()
{
    const QString q = this->search->text().trimmed();
    q.isEmpty() ? rebuildSessions() : rebuildSearch(q);
}

void SessionPanel::rebuildSessions()
{
    this->list->clear();
    for (ChatSession* s : this->sessions->sessions()) {
        auto* item = new QListWidgetItem(this->list);
        item->setData(sessionRole, s->id());
        auto* w = new SessionItemWidget(this->list);
        w->setContent(s->title(), previewOf(s), s->updatedAt());
        item->setSizeHint(w->sizeHint());
        this->list->setItemWidget(item, w);
        if (s == this->sessions->current())
            this->list->setCurrentItem(item);
    }
}

void SessionPanel::rebuildSearch(const QString& query)
{
    this->list->clear();
    const QList<SessionManager::SearchHit> hits = this->sessions->search(query);
    for (const SessionManager::SearchHit& h : hits) {
        auto* item = new QListWidgetItem(this->list);
        item->setData(sessionRole, h.session->id());
        item->setData(messageRole, h.messageId);
        auto* w = new SessionItemWidget(this->list);
        w->setContent(h.session->title(), h.excerpt, h.session->updatedAt());
        item->setSizeHint(w->sizeHint());
        this->list->setItemWidget(item, w);
    }
    if (hits.isEmpty()) {
        auto* item = new QListWidgetItem(Text("sessions.noResults"), this->list);
        item->setFlags(Qt::NoItemFlags);
    }
}

void SessionPanel::onItemClicked(QListWidgetItem* item)
{
    if (ChatSession* s = this->sessions->find(item->data(sessionRole).toString()))
        this->sessions->setCurrent(s);
}

void SessionPanel::showContextMenu(const QPoint& pos)
{
    QListWidgetItem* item = this->list->itemAt(pos);
    ChatSession* s = item ? this->sessions->find(item->data(sessionRole).toString()) : nullptr;
    if (!s)
        return;
    QMenu menu(this);
    menu.addAction(Text("sessions.rename"), this, [this, s] { renameSession(s); });
    menu.addAction(Text("common.exportMarkdown"), this, [this, s] { emit exportRequested(s); });
    menu.addSeparator();
    menu.addAction(Text("common.delete"), this, [this, s] { deleteSession(s); });
    menu.exec(this->list->viewport()->mapToGlobal(pos));
}

void SessionPanel::renameSession(ChatSession* session)
{
    bool ok = false;
    const QString t = QInputDialog::getText(this, Text("sessions.rename"), Text("sessions.nameLabel"), QLineEdit::Normal, session->title(), &ok);
    if (ok)
        this->sessions->renameSession(session, t);
}

void SessionPanel::deleteSession(ChatSession* session)
{
    const auto answer = QMessageBox::question(this, Text("sessions.deleteTitle"),
        Text("sessions.deleteConfirm").arg(session->title()));
    if (answer == QMessageBox::Yes)
        this->sessions->removeSession(session);
}
