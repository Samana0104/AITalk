#include "widget/MessageBubble.h"

#include <QClipboard>
#include <QDateTime>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QtMath>

#include "component/AvatarWidget.h"
#include "component/IconButton.h"
#include "core/Emoticons.h"
#include "core/Member.h"
#include "util/DataManager.h"

MessageBubble::MessageBubble(const ChatMessage& message, const Context& ctx, QWidget* parent)
    : QWidget(parent)
    , chatMessage(message)
    , context(ctx)
{
    if (this->chatMessage.isSystem())
        buildSystem();
    else
        buildChat();
}

const QStringList& MessageBubble::quickReactions()
{
    static const QStringList list { "👍", "😂", "❤️", "😮", "😢", "🔥" };
    return list;
}

void MessageBubble::buildSystem()
{
    auto* lay = new QHBoxLayout(this);
    lay->setContentsMargins(16, 6, 16, 6);
    auto* label = new QLabel(this->chatMessage.text, this);
    label->setObjectName(QStringLiteral("systemNotice"));
    label->setWordWrap(true);
    label->setAlignment(Qt::AlignCenter);
    if (!this->chatMessage.kind.isEmpty())
        label->setProperty("tone", this->chatMessage.kind);
    label->setToolTip(QDateTime::fromMSecsSinceEpoch(this->chatMessage.ts).toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
    lay->addStretch(1);
    lay->addWidget(label);
    lay->addStretch(1);
}

void MessageBubble::buildChat()
{
    const bool mine = this->chatMessage.isUser();

    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(16, 4, 16, 4);
    row->setSpacing(10);

    if (!mine) {
        auto* avatar = new AvatarWidget(44, this);
        avatar->setIdentity(this->chatMessage.from, Members::displayName(this->chatMessage.from, this->context.userName), Members::colorOf(this->chatMessage.from));
        row->addWidget(avatar, 0, Qt::AlignTop);
    } else {
        row->addStretch(1);
    }

    auto* body = new QVBoxLayout;
    body->setSpacing(3);

    // 이름 · 시간 · 모델 + (마우스 올리면) 답장·복사 버튼
    auto* header = new QHBoxLayout;
    header->setSpacing(6);
    const QString time = QDateTime::fromMSecsSinceEpoch(this->chatMessage.ts).toString(QStringLiteral("HH:mm"));
    QString meta = time;
    if (!this->chatMessage.model.isEmpty())
        meta += QStringLiteral(" · ") + this->chatMessage.model;
    if (this->chatMessage.boosted)
        meta += QStringLiteral(" · 🔥");
    auto* metaLabel = new QLabel(meta, this);
    metaLabel->setObjectName(QStringLiteral("messageMeta"));

    this->actions = new QWidget(this);
    auto* actLay = new QHBoxLayout(this->actions);
    actLay->setContentsMargins(0, 0, 0, 0);
    actLay->setSpacing(0);
    auto* replyBtn = new IconButton(QStringLiteral("reply"), Text("message.reply"), this->actions);
    auto* copyBtn = new IconButton(QStringLiteral("copy"), Text("message.copy"), this->actions);
    for (IconButton* b : { replyBtn, copyBtn }) {
        b->setIconSize(QSize(13, 13));
        b->setTone(QStringLiteral("muted"));
        actLay->addWidget(b);
    }
    connect(replyBtn, &QToolButton::clicked, this, [this] { emit replyRequested(this->chatMessage.id); });
    connect(copyBtn, &QToolButton::clicked, this, &MessageBubble::copyText);
    QSizePolicy keep = this->actions->sizePolicy();
    keep.setRetainSizeWhenHidden(true);
    this->actions->setSizePolicy(keep);
    this->actions->hide();

    if (mine) {
        header->addStretch(1);
        header->addWidget(this->actions);
        header->addWidget(metaLabel);
    } else {
        auto* name = new QLabel(Members::displayName(this->chatMessage.from, this->context.userName), this);
        name->setObjectName(QStringLiteral("messageName"));
        name->setStyleSheet(QStringLiteral("color: %1;").arg(Members::colorOf(this->chatMessage.from).name()));
        header->addWidget(name);
        header->addWidget(metaLabel);
        header->addWidget(this->actions);
        header->addStretch(1);
    }
    body->addLayout(header);

    // 인용 답장
    if (!this->context.replyAuthor.isEmpty()) {
        this->replyFrame = new QFrame(this);
        this->replyFrame->setObjectName(QStringLiteral("replyContext"));
        auto* rl = new QVBoxLayout(this->replyFrame);
        rl->setContentsMargins(8, 3, 8, 3);
        auto* rt = new QLabel(QStringLiteral("↪ %1: %2").arg(this->context.replyAuthor, this->context.replyExcerpt), this->replyFrame);
        rt->setObjectName(QStringLiteral("replyContextText"));
        rt->setWordWrap(true);
        rl->addWidget(rt);
        body->addWidget(this->replyFrame, 0, mine ? Qt::AlignRight : Qt::AlignLeft);
    }

    this->bubble = this->chatMessage.emoticon.isEmpty() ? buildTextBubble(mine) : buildEmoticon();
    this->bubble->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this->bubble, &QWidget::customContextMenuRequested, this, [this](const QPoint& p) {
        showMenu(this->bubble->mapToGlobal(p));
    });
    body->addWidget(this->bubble, 0, mine ? Qt::AlignRight : Qt::AlignLeft);

    // 반응
    this->reactionRow = new QWidget(this);
    this->reactionLayout = new QHBoxLayout(this->reactionRow);
    this->reactionLayout->setContentsMargins(0, 0, 0, 0);
    this->reactionLayout->setSpacing(4);
    body->addWidget(this->reactionRow, 0, mine ? Qt::AlignRight : Qt::AlignLeft);
    rebuildReactions();

    row->addLayout(body);
    if (!mine)
        row->addStretch(1);
}

QFrame* MessageBubble::buildTextBubble(bool mine)
{
    auto* bubble = new QFrame(this);
    bubble->setObjectName(QStringLiteral("bubble"));
    bubble->setProperty("role", mine ? "user" : "agent");
    bubble->setProperty("boosted", this->chatMessage.boosted);
    auto* bl = new QVBoxLayout(bubble);
    bl->setContentsMargins(12, 8, 12, 8);
    this->text = new QLabel(bubble);
    this->text->setObjectName(QStringLiteral("bubbleText"));
    this->text->setWordWrap(true);
    this->text->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
    this->text->setOpenExternalLinks(true);
    this->text->setContextMenuPolicy(Qt::NoContextMenu); // 우클릭은 말풍선 메뉴로
    // AI 답은 마크다운으로, 방장 글은 쓴 그대로 보여 준다.
    this->text->setTextFormat(mine ? Qt::PlainText : Qt::MarkdownText);
    this->text->setText(this->chatMessage.text);
    bl->addWidget(this->text);
    return bubble;
}

// 이모티콘: 말풍선 없이 그림만 (CodePet 처럼)
QFrame* MessageBubble::buildEmoticon()
{
    constexpr int emoticonSize = 100;
    auto* frame = new QFrame(this);
    frame->setObjectName(QStringLiteral("emoticonFrame"));
    auto* lay = new QVBoxLayout(frame);
    lay->setContentsMargins(0, 0, 0, 0);
    auto* img = new QLabel(frame);
    const qreal dpr = devicePixelRatioF();
    QPixmap pm(Emoticons::resourcePath(this->chatMessage.from, this->chatMessage.emoticon));
    if (!pm.isNull()) {
        pm = pm.scaled(QSize(emoticonSize, emoticonSize) * dpr, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        pm.setDevicePixelRatio(dpr);
        img->setPixmap(pm);
    } else {
        img->setText(this->chatMessage.displayText());
    }
    img->setToolTip(this->chatMessage.emoticon);
    lay->addWidget(img);
    return frame;
}

void MessageBubble::updateMessage(const ChatMessage& message)
{
    this->chatMessage = message;
    if (this->reactionRow)
        rebuildReactions();
}

void MessageBubble::setMaxBubbleWidth(int width)
{
    if (this->replyFrame)
        this->replyFrame->setMaximumWidth(width);
    if (!this->bubble || !this->text) // 시스템 알림·이모티콘은 폭 조절 없음
        return;
    this->bubble->setMaximumWidth(width);

    // 줄바꿈 QLabel 은 폭을 대충(약 80자) 잡으므로, 글의 자연스러운 폭을 직접 재서
    // 짧은 글은 딱 맞게, 긴 글은 최대 폭까지 쓰게 한다.
    if (this->idealTextWidth < 0) {
        QTextDocument doc;
        doc.setDefaultFont(this->text->font());
        doc.setDocumentMargin(0);
        if (this->text->textFormat() == Qt::MarkdownText)
            doc.setMarkdown(this->chatMessage.text);
        else
            doc.setPlainText(this->chatMessage.text);
        this->idealTextWidth = qCeil(doc.idealWidth()) + 4;
    }
    const QMargins pad = this->bubble->layout()->contentsMargins();
    const int inner = width - pad.left() - pad.right() - 2;
    this->text->setMinimumWidth(qMin(this->idealTextWidth, inner));
}

void MessageBubble::rebuildReactions()
{
    // 반응 버튼을 누른 결과로 여기 오기도 하니 바로 지우지 않고 deleteLater.
    while (QLayoutItem* item = this->reactionLayout->takeAt(0)) {
        if (QWidget* w = item->widget()) {
            w->hide();
            w->deleteLater();
        }
        delete item;
    }
    for (auto it = this->chatMessage.reactions.cbegin(); it != this->chatMessage.reactions.cend(); ++it) {
        if (it.value().isEmpty())
            continue;
        QStringList names;
        for (const QString& w : it.value())
            names << Members::displayName(w, this->context.userName);
        auto* pill = new QPushButton(QStringLiteral("%1 %2").arg(it.key()).arg(it.value().size()), this->reactionRow);
        pill->setObjectName(QStringLiteral("reactionPill"));
        pill->setProperty("mine", it.value().contains(Members::User));
        pill->setToolTip(names.join(QStringLiteral(", ")));
        pill->setCursor(Qt::PointingHandCursor);
        const QString emoji = it.key();
        connect(pill, &QPushButton::clicked, this, [this, emoji] { emit reactionRequested(this->chatMessage.id, emoji); });
        this->reactionLayout->addWidget(pill);
    }
    this->reactionRow->setVisible(this->reactionLayout->count() > 0);
}

void MessageBubble::showMenu(const QPoint& globalPos)
{
    QMenu menu(this);
    menu.addAction(Text("message.reply"), this, [this] { emit replyRequested(this->chatMessage.id); });
    menu.addAction(Text("message.copy"), this, &MessageBubble::copyText);
    if (this->text && this->text->hasSelectedText())
        menu.addAction(Text("message.copySelection"), this, [this] {
            QGuiApplication::clipboard()->setText(this->text->selectedText());
        });
    QMenu* react = menu.addMenu(Text("message.react"));
    for (const QString& e : quickReactions())
        react->addAction(e, this, [this, e] { emit reactionRequested(this->chatMessage.id, e); });
    menu.exec(globalPos);
}

void MessageBubble::copyText()
{
    QGuiApplication::clipboard()->setText(this->chatMessage.displayText());
}

void MessageBubble::enterEvent(QEnterEvent* event)
{
    if (this->actions)
        this->actions->show();
    QWidget::enterEvent(event);
}

void MessageBubble::leaveEvent(QEvent* event)
{
    if (this->actions)
        this->actions->hide();
    QWidget::leaveEvent(event);
}
