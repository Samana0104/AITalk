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

    m_reply = new ReplyPreview(this);
    lay->addWidget(m_reply);
    connect(m_reply, &ReplyPreview::cleared, this, &ComposerWidget::clearReply);

    m_box = new QFrame(this);
    m_box->setObjectName(QStringLiteral("composerBox"));
    auto* boxLay = new QHBoxLayout(m_box);
    boxLay->setContentsMargins(12, 6, 6, 6);
    boxLay->setSpacing(8);

    m_input = new AutoResizeTextEdit(m_box);
    m_input->setObjectName(QStringLiteral("composerInput"));
    m_input->setPlaceholderText(Text("composer.placeholder"));
    m_input->installEventFilter(this);
    boxLay->addWidget(m_input, 1, Qt::AlignVCenter);

    m_send = new QPushButton(m_box);
    m_send->setObjectName(QStringLiteral("sendButton"));
    m_send->setProperty("variant", "primary");
    m_send->setFixedSize(34, 34);
    m_send->setCursor(Qt::PointingHandCursor);
    m_send->setToolTip(Text("composer.sendTip"));
    m_send->setIconSize(QSize(16, 16));
    auto refreshIcon = [this] { m_send->setIcon(ThemeManager::instance().icon(QStringLiteral("send"), QStringLiteral("accent-ink"))); };
    refreshIcon();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, refreshIcon);
    boxLay->addWidget(m_send, 0, Qt::AlignBottom);
    lay->addWidget(m_box);

    connect(m_send, &QPushButton::clicked, this, &ComposerWidget::send);
    connect(m_input, &AutoResizeTextEdit::submitRequested, this, &ComposerWidget::send);
    connect(m_input, &QPlainTextEdit::textChanged, this, &ComposerWidget::updateMention);
    connect(m_input, &QPlainTextEdit::cursorPositionChanged, this, &ComposerWidget::updateMention);
}

void ComposerWidget::setReplyTarget(qint64 messageId, const QString& author, const QString& excerpt)
{
    m_replyTo = messageId;
    m_reply->setTarget(author, excerpt);
    m_reply->show();
    focusInput();
}

void ComposerWidget::clearReply()
{
    m_replyTo = 0;
    m_reply->hide();
}

void ComposerWidget::focusInput()
{
    m_input->setFocus();
}

void ComposerWidget::setHint(const QString& placeholder)
{
    m_input->setPlaceholderText(placeholder);
}

void ComposerWidget::send()
{
    if (m_mention && m_mention->isVisible()) {
        acceptMention(m_mention->currentName());
        return;
    }
    const QString text = m_input->toPlainText().trimmed();
    if (text.isEmpty())
        return;
    emit sendRequested(text, m_replyTo);
    m_input->clear();
    clearReply();
}

// ---- @멘션 ----

void ComposerWidget::updateMention()
{
    static const QRegularExpression trigger(QStringLiteral(R"(@([^\s@]*)$)"));
    const QTextCursor cur = m_input->textCursor();
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
    if (!m_mention) {
        // 입력칸 위로 떠야 하니 창(최상위 위젯)의 자식으로 만든다.
        m_mention = new MentionPopup(window());
        connect(m_mention, &MentionPopup::picked, this, &ComposerWidget::acceptMention);
    }
    m_mention->setCandidates(cands);
    positionMention();
    m_mention->show();
    m_mention->raise();
}

void ComposerWidget::positionMention()
{
    const QPoint topLeft = m_box->mapTo(window(), QPoint(0, 0));
    m_mention->move(topLeft.x() + 8, topLeft.y() - m_mention->height() - 6);
}

void ComposerWidget::closeMention()
{
    if (m_mention)
        m_mention->hide();
}

void ComposerWidget::acceptMention(const QString& name)
{
    if (name.isEmpty())
        return;
    QTextCursor cur = m_input->textCursor();
    const QString before = cur.block().text().left(cur.positionInBlock());
    const qsizetype at = before.lastIndexOf(QLatin1Char('@'));
    if (at >= 0) {
        cur.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, int(before.size() - at));
        cur.insertText(QStringLiteral("@%1 ").arg(name));
        m_input->setTextCursor(cur);
    }
    closeMention();
    focusInput();
}

bool ComposerWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_input && event->type() == QEvent::KeyPress && m_mention && m_mention->isVisible()) {
        auto* key = static_cast<QKeyEvent*>(event);
        switch (key->key()) {
        case Qt::Key_Up:
            m_mention->moveSelection(-1);
            return true;
        case Qt::Key_Down:
            m_mention->moveSelection(1);
            return true;
        case Qt::Key_Tab:
        case Qt::Key_Return:
        case Qt::Key_Enter:
            acceptMention(m_mention->currentName());
            return true;
        case Qt::Key_Escape:
            closeMention();
            return true;
        default:
            break;
        }
    }
    if (watched == m_input && event->type() == QEvent::FocusOut)
        closeMention();
    return QWidget::eventFilter(watched, event);
}
