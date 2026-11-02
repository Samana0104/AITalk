#include "component/AutoResizeTextEdit.h"

#include <QAbstractTextDocumentLayout>
#include <QKeyEvent>
#include <QTextDocument>

AutoResizeTextEdit::AutoResizeTextEdit(QWidget* parent)
    : QPlainTextEdit(parent)
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setLineWrapMode(QPlainTextEdit::WidgetWidth);
    setTabChangesFocus(true);
    connect(document()->documentLayout(), &QAbstractTextDocumentLayout::documentSizeChanged, this, &AutoResizeTextEdit::updateHeight);
    connect(this, &QPlainTextEdit::textChanged, this, &AutoResizeTextEdit::updateHeight);
    updateHeight();
}

void AutoResizeTextEdit::setMaxLines(int lines)
{
    m_maxLines = qMax(1, lines);
    updateHeight();
}

void AutoResizeTextEdit::keyPressEvent(QKeyEvent* event)
{
    const bool enter = event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter;
    if (enter && !(event->modifiers() & Qt::ShiftModifier)) {
        emit submitRequested();
        return;
    }
    QPlainTextEdit::keyPressEvent(event);
}

void AutoResizeTextEdit::resizeEvent(QResizeEvent* event)
{
    QPlainTextEdit::resizeEvent(event);
    updateHeight();
}

void AutoResizeTextEdit::updateHeight()
{
    // QPlainTextDocumentLayout 의 높이 단위는 "줄" 이다. 최대 줄 수를 넘을 때만 스크롤바.
    const int raw = int(document()->documentLayout()->documentSize().height());
    const int lines = qBound(1, raw, m_maxLines);
    setVerticalScrollBarPolicy(raw > m_maxLines ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
    const QMargins m = contentsMargins();
    const int h = lines * fontMetrics().lineSpacing() + int(document()->documentMargin() * 2) + m.top() + m.bottom() + frameWidth() * 2;
    if (height() != h)
        setFixedHeight(h);
}
