#include "widget/ReplyPreview.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include "component/IconButton.h"
#include "util/DataManager.h"

ReplyPreview::ReplyPreview(QWidget* parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("replyPreview"));
    auto* lay = new QHBoxLayout(this);
    lay->setContentsMargins(10, 5, 4, 5);

    auto* text = new QVBoxLayout;
    text->setSpacing(0);
    this->author = new QLabel(this);
    this->author->setObjectName(QStringLiteral("replyPreviewAuthor"));
    this->excerpt = new QLabel(this);
    this->excerpt->setObjectName(QStringLiteral("replyPreviewText"));
    text->addWidget(this->author);
    text->addWidget(this->excerpt);
    lay->addLayout(text, 1);

    auto* close = new IconButton(QStringLiteral("close"), Text("composer.clearReply"), this);
    close->setIconSize(QSize(10, 10));
    lay->addWidget(close, 0, Qt::AlignTop);
    connect(close, &QToolButton::clicked, this, &ReplyPreview::cleared);
    hide();
}

void ReplyPreview::setTarget(const QString& author, const QString& excerpt)
{
    this->author->setText(QStringLiteral("↪ %1").arg(author));
    this->excerpt->setText(fontMetrics().elidedText(excerpt, Qt::ElideRight, 520));
}
