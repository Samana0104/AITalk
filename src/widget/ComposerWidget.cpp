#include "widget/ComposerWidget.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QPushButton>
#include <QRegularExpression>
#include <QTextBlock>
#include <QVBoxLayout>

#include "component/AutoResizeTextEdit.h"
#include "core/Member.h"
#include "style/ThemeManager.h"
#include "util/DataManager.h"
#include "widget/MentionPopup.h"
#include "widget/ReplyPreview.h"

ComposerWidget::ComposerWidget(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("composer"));
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(20, 6, 20, 16);
    lay->setSpacing(6);

    this->reply = new ReplyPreview(this);
    lay->addWidget(this->reply);
    connect(this->reply, &ReplyPreview::cleared, this, &ComposerWidget::clearReply);

    this->box = new QFrame(this);
    this->box->setObjectName(QStringLiteral("composerBox"));
    auto* boxLay = new QHBoxLayout(this->box);
    boxLay->setContentsMargins(12, 6, 6, 6);
    boxLay->setSpacing(8);

    this->input = new AutoResizeTextEdit(this->box);
    this->input->setObjectName(QStringLiteral("composerInput"));
    this->input->setPlaceholderText(Text("composer.placeholder"));
    this->input->installEventFilter(this);
    boxLay->addWidget(this->input, 1, Qt::AlignVCenter);

    this->sendButton = new QPushButton(this->box);
    this->sendButton->setObjectName(QStringLiteral("sendButton"));
    this->sendButton->setProperty("variant", "primary");
    this->sendButton->setFixedSize(34, 34);
    this->sendButton->setCursor(Qt::PointingHandCursor);
    this->sendButton->setToolTip(Text("composer.sendTip"));
    this->sendButton->setIconSize(QSize(16, 16));
    auto refreshIcon = [this] { this->sendButton->setIcon(ThemeManager::instance().icon(QStringLiteral("send"), QStringLiteral("accent-ink"))); };
    refreshIcon();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, refreshIcon);
    boxLay->addWidget(this->sendButton, 0, Qt::AlignBottom);
    lay->addWidget(this->box);

    connect(this->sendButton, &QPushButton::clicked, this, &ComposerWidget::send);
    connect(this->input, &AutoResizeTextEdit::submitRequested, this, &ComposerWidget::send);
    connect(this->input, &QPlainTextEdit::textChanged, this, &ComposerWidget::updateMention);
    connect(this->input, &QPlainTextEdit::cursorPositionChanged, this, &ComposerWidget::updateMention);
}

void ComposerWidget::setReplyTarget(qint64 messageId, const QString& author, const QString& excerpt)
{
    this->replyTo = messageId;
    this->reply->setTarget(author, excerpt);
    this->reply->show();
    focusInput();
}

void ComposerWidget::clearReply()
{
    this->replyTo = 0;
    this->reply->hide();
}

void ComposerWidget::focusInput()
{
    this->input->setFocus();
}

void ComposerWidget::setHint(const QString& placeholder)
{
    this->input->setPlaceholderText(placeholder);
}

void ComposerWidget::send()
{
    if (this->mention && this->mention->isVisible()) {
        acceptMention(this->mention->currentName());
        return;
    }
    const QString text = this->input->toPlainText().trimmed();
    if (text.isEmpty())
        return;
    emit sendRequested(text, this->replyTo);
    this->input->clear();
    clearReply();
}

// ---- @멘션 ----

void ComposerWidget::updateMention()
{
    static const QRegularExpression trigger(QStringLiteral(R"(@([^\s@]*)$)"));
    const QTextCursor cur = this->input->textCursor();
    const QString before = cur.block().text().left(cur.positionInBlock());
    const QRegularExpressionMatch m = trigger.match(before);
    if (!m.hasMatch()) {
        closeMention();
        return;
    }
    const QString q = m.captured(1).toLower();
    QList<MentionPopup::Candidate> cands;
    for (const MemberInfo& info : Members::all()) {
        bool hit = q.isEmpty() || info.name.toLower().startsWith(q);
        for (const QString& a : info.aliases)
            hit = hit || a.startsWith(q);
        if (hit)
            cands.append({ info.id, info.name, info.color });
    }
    if (cands.isEmpty()) {
        closeMention();
        return;
    }
    if (!this->mention) {
        // 입력칸 위로 떠야 하니 창(최상위 위젯)의 자식으로 만든다.
        this->mention = new MentionPopup(window());
        connect(this->mention, &MentionPopup::picked, this, &ComposerWidget::acceptMention);
    }
    this->mention->setCandidates(cands);
    positionMention();
    this->mention->show();
    this->mention->raise();
}

void ComposerWidget::positionMention()
{
    const QPoint topLeft = this->box->mapTo(window(), QPoint(0, 0));
    this->mention->move(topLeft.x() + 8, topLeft.y() - this->mention->height() - 6);
}

void ComposerWidget::closeMention()
{
    if (this->mention)
        this->mention->hide();
}

void ComposerWidget::acceptMention(const QString& name)
{
    if (name.isEmpty())
        return;
    QTextCursor cur = this->input->textCursor();
    const QString before = cur.block().text().left(cur.positionInBlock());
    const qsizetype at = before.lastIndexOf(QLatin1Char('@'));
    if (at >= 0) {
        cur.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, int(before.size() - at));
        cur.insertText(QStringLiteral("@%1 ").arg(name));
        this->input->setTextCursor(cur);
    }
    closeMention();
    focusInput();
}

bool ComposerWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == this->input && event->type() == QEvent::KeyPress && this->mention && this->mention->isVisible()) {
        auto* key = static_cast<QKeyEvent*>(event);
        switch (key->key()) {
        case Qt::Key_Up:
            this->mention->moveSelection(-1);
            return true;
        case Qt::Key_Down:
            this->mention->moveSelection(1);
            return true;
        case Qt::Key_Tab:
        case Qt::Key_Return:
        case Qt::Key_Enter:
            acceptMention(this->mention->currentName());
            return true;
        case Qt::Key_Escape:
            closeMention();
            return true;
        default:
            break;
        }
    }
    if (watched == this->input && event->type() == QEvent::FocusOut)
        closeMention();
    return QWidget::eventFilter(watched, event);
}
