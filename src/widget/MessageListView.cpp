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
constexpr int fixedRows = 2; // 레이아웃 앞쪽 고정 항목: [stretch][이전 대화 버튼]
}

MessageListView::MessageListView(QWidget* parent)
    : QScrollArea(parent)
{
    setObjectName(QStringLiteral("messageScroll"));
    setWidgetResizable(true);
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    this->canvas = new QWidget(this);
    this->canvas->setObjectName(QStringLiteral("messageCanvas"));
    this->layout = new QVBoxLayout(this->canvas);
    this->layout->setContentsMargins(0, 12, 0, 12);
    this->layout->setSpacing(2);
    this->layout->addStretch(1); // 메시지가 적을 때 아래에 붙도록

    this->loadMore = new QPushButton(Text("messages.loadOlder"), this->canvas);
    this->loadMore->setObjectName(QStringLiteral("loadMore"));
    this->loadMore->setCursor(Qt::PointingHandCursor);
    this->loadMore->hide();
    this->layout->addWidget(this->loadMore, 0, Qt::AlignHCenter);
    connect(this->loadMore, &QPushButton::clicked, this, &MessageListView::loadOlder);

    setWidget(this->canvas);

    // "맨 아래 따라가기"는 사용자가 직접 스크롤할 때만 바꾼다. 레이아웃이 자리 잡으면서
    // 생기는 값 변화까지 보면 처음 열 때 중간에 멈춰 버린다.
    QScrollBar* bar = verticalScrollBar();
    auto userScrolled = [this, bar](int pos) {
        if (this->anchorFromBottom < 0)
            this->stickToBottom = pos >= bar->maximum() - 40;
    };
    connect(bar, &QScrollBar::actionTriggered, this, [bar, userScrolled](int) { userScrolled(bar->sliderPosition()); });
    connect(bar, &QScrollBar::sliderMoved, this, userScrolled);
    connect(bar, &QScrollBar::rangeChanged, this, [this, bar](int, int max) {
        if (this->anchorFromBottom >= 0) {
            bar->setValue(max - this->anchorFromBottom);
            this->anchorFromBottom = -1;
        } else if (this->stickToBottom) {
            bar->setValue(max);
        }
    });
}

void MessageListView::setSession(ChatSession* session, const QString& userName)
{
    clear();
    this->session = session;
    this->userName = userName;
    if (!session)
        return;
    const QList<ChatMessage>& all = session->messages();
    this->firstIndex = qMax(0, int(all.size()) - pageSize);
    for (qsizetype i = this->firstIndex; i < all.size(); ++i)
        this->layout->addWidget(makeBubble(all.at(i)));
    updateLoadMore();
    scrollToBottom();
}

void MessageListView::setUserName(const QString& userName)
{
    if (userName == this->userName)
        return;
    setSession(this->session, userName); // 이름이 바뀌면 다시 그린다
}

void MessageListView::clear()
{
    for (MessageBubble* b : std::as_const(this->rows)) {
        this->layout->removeWidget(b);
        b->hide();
        b->deleteLater();
    }
    this->rows.clear();
    this->firstIndex = 0;
    this->stickToBottom = true;
    this->anchorFromBottom = -1;
    this->loadMore->hide();
}

MessageBubble* MessageListView::makeBubble(const ChatMessage& m)
{
    MessageBubble::Context ctx;
    ctx.userName = this->userName;
    if (this->session) {
        if (const ChatMessage* r = this->session->byId(m.replyTo)) {
            static const QRegularExpression markdownMarks(QStringLiteral("[*_`#>~]+"));
            ctx.replyAuthor = Members::displayName(r->from, this->userName);
            ctx.replyExcerpt = r->displayText().remove(markdownMarks).simplified().left(60);
        }
    }
    auto* b = new MessageBubble(m, ctx, this->canvas);
    b->setMaxBubbleWidth(bubbleWidth());
    connect(b, &MessageBubble::replyRequested, this, &MessageListView::replyRequested);
    connect(b, &MessageBubble::reactionRequested, this, &MessageListView::reactionRequested);
    this->rows.insert(m.id, b);
    return b;
}

void MessageListView::appendMessage(const ChatMessage& message)
{
    if (this->rows.contains(message.id))
        return;
    this->layout->addWidget(makeBubble(message));
}

void MessageListView::updateMessage(const ChatMessage& message)
{
    if (MessageBubble* b = this->rows.value(message.id))
        b->updateMessage(message);
}

void MessageListView::scrollToBottom()
{
    this->stickToBottom = true;
    QTimer::singleShot(0, this, [this] { verticalScrollBar()->setValue(verticalScrollBar()->maximum()); });
}

void MessageListView::loadOlder()
{
    if (!this->session || this->firstIndex <= 0)
        return;
    const QList<ChatMessage>& all = this->session->messages();
    const int from = qMax(0, this->firstIndex - pageSize);
    QScrollBar* bar = verticalScrollBar();
    this->anchorFromBottom = bar->maximum() - bar->value();
    this->stickToBottom = false;
    for (int i = this->firstIndex - 1; i >= from; --i)
        this->layout->insertWidget(fixedRows, makeBubble(all.at(i)));
    this->firstIndex = from;
    updateLoadMore();
}

void MessageListView::updateLoadMore()
{
    this->loadMore->setVisible(this->firstIndex > 0);
}

int MessageListView::bubbleWidth() const
{
    return qMax(240, int(viewport()->width() * 0.68));
}

void MessageListView::resizeEvent(QResizeEvent* event)
{
    QScrollArea::resizeEvent(event);
    const int w = bubbleWidth();
    for (MessageBubble* b : std::as_const(this->rows))
        b->setMaxBubbleWidth(w);
}
