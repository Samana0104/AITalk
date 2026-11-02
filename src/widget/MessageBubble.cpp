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
    , m_msg(message)
    , m_ctx(ctx)
{
    if (m_msg.isSystem())
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
    auto* label = new QLabel(m_msg.text, this);
    label->setObjectName(QStringLiteral("systemNotice"));
    label->setWordWrap(true);
    label->setAlignment(Qt::AlignCenter);
    if (!m_msg.kind.isEmpty())
        label->setProperty("tone", m_msg.kind);
    label->setToolTip(QDateTime::fromMSecsSinceEpoch(m_msg.ts).toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
    lay->addStretch(1);
    lay->addWidget(label);
    lay->addStretch(1);
}

void MessageBubble::buildChat()
{
    const bool mine = m_msg.isUser();

    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(16, 4, 16, 4);
    row->setSpacing(10);

    if (!mine) {
        auto* avatar = new AvatarWidget(44, this);
        avatar->setIdentity(m_msg.from, Members::displayName(m_msg.from, m_ctx.userName), Members::colorOf(m_msg.from));
        row->addWidget(avatar, 0, Qt::AlignTop);
    } else {
        row->addStretch(1);
    }

    auto* body = new QVBoxLayout;
    body->setSpacing(3);

    // 이름 · 시간 · 모델 + (마우스 올리면) 답장·복사 버튼
    auto* header = new QHBoxLayout;
    header->setSpacing(6);
    const QString time = QDateTime::fromMSecsSinceEpoch(m_msg.ts).toString(QStringLiteral("HH:mm"));
    QString meta = time;
    if (!m_msg.model.isEmpty())
        meta += QStringLiteral(" · ") + m_msg.model;
    if (m_msg.boosted)
        meta += QStringLiteral(" · 🔥");
    auto* metaLabel = new QLabel(meta, this);
    metaLabel->setObjectName(QStringLiteral("messageMeta"));

    m_actions = new QWidget(this);
    auto* actLay = new QHBoxLayout(m_actions);
    actLay->setContentsMargins(0, 0, 0, 0);
    actLay->setSpacing(0);
    auto* replyBtn = new IconButton(QStringLiteral("reply"), Text("message.reply"), m_actions);
    auto* copyBtn = new IconButton(QStringLiteral("copy"), Text("message.copy"), m_actions);
    for (IconButton* b : { replyBtn, copyBtn }) {
        b->setIconSize(QSize(13, 13));
        b->setTone(QStringLiteral("muted"));
        actLay->addWidget(b);
    }
    connect(replyBtn, &QToolButton::clicked, this, [this] { emit replyRequested(m_msg.id); });
    connect(copyBtn, &QToolButton::clicked, this, &MessageBubble::copyText);
    QSizePolicy keep = m_actions->sizePolicy();
    keep.setRetainSizeWhenHidden(true);
    m_actions->setSizePolicy(keep);
    m_actions->hide();

    if (mine) {
        header->addStretch(1);
        header->addWidget(m_actions);
        header->addWidget(metaLabel);
    } else {
        auto* name = new QLabel(Members::displayName(m_msg.from, m_ctx.userName), this);
        name->setObjectName(QStringLiteral("messageName"));
        name->setStyleSheet(QStringLiteral("color: %1;").arg(Members::colorOf(m_msg.from).name()));
        header->addWidget(name);
        header->addWidget(metaLabel);
        header->addWidget(m_actions);
        header->addStretch(1);
    }
    body->addLayout(header);

    // 인용 답장
    if (!m_ctx.replyAuthor.isEmpty()) {
        m_replyFrame = new QFrame(this);
        m_replyFrame->setObjectName(QStringLiteral("replyContext"));
        auto* rl = new QVBoxLayout(m_replyFrame);
        rl->setContentsMargins(8, 3, 8, 3);
        auto* rt = new QLabel(QStringLiteral("↪ %1: %2").arg(m_ctx.replyAuthor, m_ctx.replyExcerpt), m_replyFrame);
        rt->setObjectName(QStringLiteral("replyContextText"));
        rt->setWordWrap(true);
        rl->addWidget(rt);
        body->addWidget(m_replyFrame, 0, mine ? Qt::AlignRight : Qt::AlignLeft);
    }

    m_bubble = m_msg.emoticon.isEmpty() ? buildTextBubble(mine) : buildEmoticon();
    m_bubble->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_bubble, &QWidget::customContextMenuRequested, this, [this](const QPoint& p) {
        showMenu(m_bubble->mapToGlobal(p));
    });
    body->addWidget(m_bubble, 0, mine ? Qt::AlignRight : Qt::AlignLeft);

    // 반응
    m_reactionRow = new QWidget(this);
    m_reactionLayout = new QHBoxLayout(m_reactionRow);
    m_reactionLayout->setContentsMargins(0, 0, 0, 0);
    m_reactionLayout->setSpacing(4);
    body->addWidget(m_reactionRow, 0, mine ? Qt::AlignRight : Qt::AlignLeft);
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
    bubble->setProperty("boosted", m_msg.boosted);
    auto* bl = new QVBoxLayout(bubble);
    bl->setContentsMargins(12, 8, 12, 8);
    m_text = new QLabel(bubble);
    m_text->setObjectName(QStringLiteral("bubbleText"));
    m_text->setWordWrap(true);
    m_text->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
    m_text->setOpenExternalLinks(true);
    m_text->setContextMenuPolicy(Qt::NoContextMenu); // 우클릭은 말풍선 메뉴로
    // AI 답은 마크다운으로, 방장 글은 쓴 그대로 보여 준다.
    m_text->setTextFormat(mine ? Qt::PlainText : Qt::MarkdownText);
    m_text->setText(m_msg.text);
    bl->addWidget(m_text);
    return bubble;
}

// 이모티콘: 말풍선 없이 그림만 (CodePet 처럼)
QFrame* MessageBubble::buildEmoticon()
{
    constexpr int kEmoticonSize = 100;
    auto* frame = new QFrame(this);
    frame->setObjectName(QStringLiteral("emoticonFrame"));
    auto* lay = new QVBoxLayout(frame);
    lay->setContentsMargins(0, 0, 0, 0);
    auto* img = new QLabel(frame);
    const qreal dpr = devicePixelRatioF();
    QPixmap pm(Emoticons::resourcePath(m_msg.from, m_msg.emoticon));
    if (!pm.isNull()) {
        pm = pm.scaled(QSize(kEmoticonSize, kEmoticonSize) * dpr, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        pm.setDevicePixelRatio(dpr);
        img->setPixmap(pm);
    } else {
        img->setText(m_msg.displayText());
    }
    img->setToolTip(m_msg.emoticon);
    lay->addWidget(img);
    return frame;
}

void MessageBubble::updateMessage(const ChatMessage& message)
{
    m_msg = message;
    if (m_reactionRow)
        rebuildReactions();
}

void MessageBubble::setMaxBubbleWidth(int width)
{
    if (m_replyFrame)
        m_replyFrame->setMaximumWidth(width);
    if (!m_bubble || !m_text) // 시스템 알림·이모티콘은 폭 조절 없음
        return;
    m_bubble->setMaximumWidth(width);

    // 줄바꿈 QLabel 은 폭을 대충(약 80자) 잡으므로, 글의 자연스러운 폭을 직접 재서
    // 짧은 글은 딱 맞게, 긴 글은 최대 폭까지 쓰게 한다.
    if (m_idealTextWidth < 0) {
        QTextDocument doc;
        doc.setDefaultFont(m_text->font());
        doc.setDocumentMargin(0);
        if (m_text->textFormat() == Qt::MarkdownText)
            doc.setMarkdown(m_msg.text);
        else
            doc.setPlainText(m_msg.text);
        m_idealTextWidth = qCeil(doc.idealWidth()) + 4;
    }
    const QMargins pad = m_bubble->layout()->contentsMargins();
    const int inner = width - pad.left() - pad.right() - 2;
    m_text->setMinimumWidth(qMin(m_idealTextWidth, inner));
}

void MessageBubble::rebuildReactions()
{
    // 반응 버튼을 누른 결과로 여기 오기도 하니 바로 지우지 않고 deleteLater.
    while (QLayoutItem* item = m_reactionLayout->takeAt(0)) {
        if (QWidget* w = item->widget()) {
            w->hide();
            w->deleteLater();
        }
        delete item;
    }
    for (auto it = m_msg.reactions.cbegin(); it != m_msg.reactions.cend(); ++it) {
        if (it.value().isEmpty())
            continue;
        QStringList names;
        for (const QString& w : it.value())
            names << Members::displayName(w, m_ctx.userName);
        auto* pill = new QPushButton(QStringLiteral("%1 %2").arg(it.key()).arg(it.value().size()), m_reactionRow);
        pill->setObjectName(QStringLiteral("reactionPill"));
        pill->setProperty("mine", it.value().contains(Members::User));
        pill->setToolTip(names.join(QStringLiteral(", ")));
        pill->setCursor(Qt::PointingHandCursor);
        const QString emoji = it.key();
        connect(pill, &QPushButton::clicked, this, [this, emoji] { emit reactionRequested(m_msg.id, emoji); });
        m_reactionLayout->addWidget(pill);
    }
    m_reactionRow->setVisible(m_reactionLayout->count() > 0);
}

void MessageBubble::showMenu(const QPoint& globalPos)
{
    QMenu menu(this);
    menu.addAction(Text("message.reply"), this, [this] { emit replyRequested(m_msg.id); });
    menu.addAction(Text("message.copy"), this, &MessageBubble::copyText);
    if (m_text && m_text->hasSelectedText())
        menu.addAction(Text("message.copySelection"), this, [this] {
            QGuiApplication::clipboard()->setText(m_text->selectedText());
        });
    QMenu* react = menu.addMenu(Text("message.react"));
    for (const QString& e : quickReactions())
        react->addAction(e, this, [this, e] { emit reactionRequested(m_msg.id, e); });
    menu.exec(globalPos);
}

void MessageBubble::copyText()
{
    QGuiApplication::clipboard()->setText(m_msg.displayText());
}

void MessageBubble::enterEvent(QEnterEvent* event)
{
    if (m_actions)
        m_actions->show();
    QWidget::enterEvent(event);
}

void MessageBubble::leaveEvent(QEvent* event)
{
    if (m_actions)
        m_actions->hide();
    QWidget::leaveEvent(event);
}
