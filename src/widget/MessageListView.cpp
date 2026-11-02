#include "widget/MessageListView.h"

#include <QPushButton>
#include <QRegularExpression>
#include <QScrollBar>
#include <QTimer>
#include <QVBoxLayout>

#include "core/Member.h"
#include "storage/ChatSession.h"
#include "util/DataManager.h"
#include "widget/MessageBubble.h"

namespace {
constexpr int kFixedRows = 2; // 레이아웃 앞쪽 고정 항목: [stretch][이전 대화 버튼]
}

MessageListView::MessageListView(QWidget* parent)
    : QScrollArea(parent)
{
    setObjectName(QStringLiteral("messageScroll"));
    setWidgetResizable(true);
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_canvas = new QWidget(this);
    m_canvas->setObjectName(QStringLiteral("messageCanvas"));
    m_layout = new QVBoxLayout(m_canvas);
    m_layout->setContentsMargins(0, 12, 0, 12);
    m_layout->setSpacing(2);
    m_layout->addStretch(1); // 메시지가 적을 때 아래에 붙도록

    m_loadMore = new QPushButton(Text("messages.loadOlder"), m_canvas);
    m_loadMore->setObjectName(QStringLiteral("loadMore"));
    m_loadMore->setCursor(Qt::PointingHandCursor);
    m_loadMore->hide();
    m_layout->addWidget(m_loadMore, 0, Qt::AlignHCenter);
    connect(m_loadMore, &QPushButton::clicked, this, &MessageListView::loadOlder);

    setWidget(m_canvas);

    // "맨 아래 따라가기"는 사용자가 직접 스크롤할 때만 바꾼다. 레이아웃이 자리 잡으면서
    // 생기는 값 변화까지 보면 처음 열 때 중간에 멈춰 버린다.
    QScrollBar* bar = verticalScrollBar();
    auto userScrolled = [this, bar](int pos) {
        if (m_anchorFromBottom < 0)
            m_stick = pos >= bar->maximum() - 40;
    };
    connect(bar, &QScrollBar::actionTriggered, this, [bar, userScrolled](int) { userScrolled(bar->sliderPosition()); });
    connect(bar, &QScrollBar::sliderMoved, this, userScrolled);
    connect(bar, &QScrollBar::rangeChanged, this, [this, bar](int, int max) {
        if (m_anchorFromBottom >= 0) {
            bar->setValue(max - m_anchorFromBottom);
            m_anchorFromBottom = -1;
        } else if (m_stick) {
            bar->setValue(max);
        }
    });
}

void MessageListView::setSession(ChatSession* session, const QString& userName)
{
    clear();
    m_session = session;
    m_userName = userName;
    if (!session)
        return;
    const QList<ChatMessage>& all = session->messages();
    m_firstIndex = qMax(0, int(all.size()) - kPageSize);
    for (qsizetype i = m_firstIndex; i < all.size(); ++i)
        m_layout->addWidget(makeBubble(all.at(i)));
    updateLoadMore();
    scrollToBottom();
}

void MessageListView::setUserName(const QString& userName)
{
    if (userName == m_userName)
        return;
    setSession(m_session, userName); // 이름이 바뀌면 다시 그린다
}

void MessageListView::clear()
{
    for (MessageBubble* b : std::as_const(m_rows)) {
        m_layout->removeWidget(b);
        b->hide();
        b->deleteLater();
    }
    m_rows.clear();
    m_firstIndex = 0;
    m_stick = true;
    m_anchorFromBottom = -1;
    m_loadMore->hide();
}

MessageBubble* MessageListView::makeBubble(const ChatMessage& m)
{
    MessageBubble::Context ctx;
    ctx.userName = m_userName;
    if (m_session) {
        if (const ChatMessage* r = m_session->byId(m.replyTo)) {
            static const QRegularExpression markdownMarks(QStringLiteral("[*_`#>~]+"));
            ctx.replyAuthor = Members::displayName(r->from, m_userName);
            ctx.replyExcerpt = r->displayText().remove(markdownMarks).simplified().left(60);
        }
    }
    auto* b = new MessageBubble(m, ctx, m_canvas);
    b->setMaxBubbleWidth(bubbleWidth());
    connect(b, &MessageBubble::replyRequested, this, &MessageListView::replyRequested);
    connect(b, &MessageBubble::reactionRequested, this, &MessageListView::reactionRequested);
    m_rows.insert(m.id, b);
    return b;
}

void MessageListView::appendMessage(const ChatMessage& message)
{
    if (m_rows.contains(message.id))
        return;
    m_layout->addWidget(makeBubble(message));
}

void MessageListView::updateMessage(const ChatMessage& message)
{
    if (MessageBubble* b = m_rows.value(message.id))
        b->updateMessage(message);
}

void MessageListView::scrollToBottom()
{
    m_stick = true;
    QTimer::singleShot(0, this, [this] { verticalScrollBar()->setValue(verticalScrollBar()->maximum()); });
}

void MessageListView::loadOlder()
{
    if (!m_session || m_firstIndex <= 0)
        return;
    const QList<ChatMessage>& all = m_session->messages();
    const int from = qMax(0, m_firstIndex - kPageSize);
    QScrollBar* bar = verticalScrollBar();
    m_anchorFromBottom = bar->maximum() - bar->value();
    m_stick = false;
    for (int i = m_firstIndex - 1; i >= from; --i)
        m_layout->insertWidget(kFixedRows, makeBubble(all.at(i)));
    m_firstIndex = from;
    updateLoadMore();
}

void MessageListView::updateLoadMore()
{
    m_loadMore->setVisible(m_firstIndex > 0);
}

int MessageListView::bubbleWidth() const
{
    return qMax(240, int(viewport()->width() * 0.68));
}

void MessageListView::resizeEvent(QResizeEvent* event)
{
    QScrollArea::resizeEvent(event);
    const int w = bubbleWidth();
    for (MessageBubble* b : std::as_const(m_rows))
        b->setMaxBubbleWidth(w);
}
