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
constexpr int kSessionRole = Qt::UserRole;
constexpr int kMessageRole = Qt::UserRole + 1;

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
    , m_sessions(sessions)
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

    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(Text("sessions.search"));
    m_search->setClearButtonEnabled(true);
    auto refreshSearchIcon = [this] {
        const QList<QAction*> acts = m_search->actions();
        for (QAction* a : acts)
            if (a->objectName() == QLatin1String("searchIcon"))
                m_search->removeAction(a);
        QAction* a = m_search->addAction(ThemeManager::instance().icon(QStringLiteral("search"), QStringLiteral("muted")), QLineEdit::LeadingPosition);
        a->setObjectName(QStringLiteral("searchIcon"));
    };
    refreshSearchIcon();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, refreshSearchIcon);
    lay->addWidget(m_search);

    m_list = new QListWidget(this);
    m_list->setObjectName(QStringLiteral("sessionList"));
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->setContextMenuPolicy(Qt::CustomContextMenu);
    m_list->setSpacing(1);
    lay->addWidget(m_list, 1);

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

    m_rebuildTimer.setSingleShot(true);
    m_rebuildTimer.setInterval(150);
    connect(&m_rebuildTimer, &QTimer::timeout, this, &SessionPanel::rebuild);

    connect(newBtn, &QPushButton::clicked, this, [this] {
        m_search->clear();
        m_sessions->setCurrent(m_sessions->createSession());
    });
    connect(m_search, &QLineEdit::textChanged, this, &SessionPanel::scheduleRebuild);
    connect(m_list, &QListWidget::itemClicked, this, &SessionPanel::onItemClicked);
    connect(m_list, &QWidget::customContextMenuRequested, this, &SessionPanel::showContextMenu);
    connect(doctor, &QPushButton::clicked, this, &SessionPanel::doctorRequested);
    connect(rescan, &QPushButton::clicked, this, &SessionPanel::refreshCliRequested);
    connect(m_sessions, &SessionManager::sessionsChanged, this, &SessionPanel::scheduleRebuild);
    connect(m_sessions, &SessionManager::currentChanged, this, &SessionPanel::scheduleRebuild);

    rebuild();
}

void SessionPanel::scheduleRebuild()
{
    m_rebuildTimer.start();
}

void SessionPanel::rebuild()
{
    const QString q = m_search->text().trimmed();
    q.isEmpty() ? rebuildSessions() : rebuildSearch(q);
}

void SessionPanel::rebuildSessions()
{
    m_list->clear();
    for (ChatSession* s : m_sessions->sessions()) {
        auto* item = new QListWidgetItem(m_list);
        item->setData(kSessionRole, s->id());
        auto* w = new SessionItemWidget(m_list);
        w->setContent(s->title(), previewOf(s), s->updatedAt());
        item->setSizeHint(w->sizeHint());
        m_list->setItemWidget(item, w);
        if (s == m_sessions->current())
            m_list->setCurrentItem(item);
    }
}

void SessionPanel::rebuildSearch(const QString& query)
{
    m_list->clear();
    const QList<SessionManager::SearchHit> hits = m_sessions->search(query);
    for (const SessionManager::SearchHit& h : hits) {
        auto* item = new QListWidgetItem(m_list);
        item->setData(kSessionRole, h.session->id());
        item->setData(kMessageRole, h.messageId);
        auto* w = new SessionItemWidget(m_list);
        w->setContent(h.session->title(), h.excerpt, h.session->updatedAt());
        item->setSizeHint(w->sizeHint());
        m_list->setItemWidget(item, w);
    }
    if (hits.isEmpty()) {
        auto* item = new QListWidgetItem(Text("sessions.noResults"), m_list);
        item->setFlags(Qt::NoItemFlags);
    }
}

void SessionPanel::onItemClicked(QListWidgetItem* item)
{
    if (ChatSession* s = m_sessions->find(item->data(kSessionRole).toString()))
        m_sessions->setCurrent(s);
}

void SessionPanel::showContextMenu(const QPoint& pos)
{
    QListWidgetItem* item = m_list->itemAt(pos);
    ChatSession* s = item ? m_sessions->find(item->data(kSessionRole).toString()) : nullptr;
    if (!s)
        return;
    QMenu menu(this);
    menu.addAction(Text("sessions.rename"), this, [this, s] { renameSession(s); });
    menu.addAction(Text("common.exportMarkdown"), this, [this, s] { emit exportRequested(s); });
    menu.addSeparator();
    menu.addAction(Text("common.delete"), this, [this, s] { deleteSession(s); });
    menu.exec(m_list->viewport()->mapToGlobal(pos));
}

void SessionPanel::renameSession(ChatSession* session)
{
    bool ok = false;
    const QString t = QInputDialog::getText(this, Text("sessions.rename"), Text("sessions.nameLabel"), QLineEdit::Normal, session->title(), &ok);
    if (ok)
        m_sessions->renameSession(session, t);
}

void SessionPanel::deleteSession(ChatSession* session)
{
    const auto answer = QMessageBox::question(this, Text("sessions.deleteTitle"),
        Text("sessions.deleteConfirm").arg(session->title()));
    if (answer == QMessageBox::Yes)
        m_sessions->removeSession(session);
}
