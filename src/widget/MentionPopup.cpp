#include "widget/MentionPopup.h"

#include <QListWidget>
#include <QVBoxLayout>

#include "component/AvatarWidget.h"

MentionPopup::MentionPopup(QWidget* parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("mentionPopup"));
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(4, 4, 4, 4);
    this->list = new QListWidget(this);
    this->list->setObjectName(QStringLiteral("mentionList"));
    this->list->setFocusPolicy(Qt::NoFocus);
    this->list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->list->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->list->setIconSize(QSize(22, 22));
    lay->addWidget(this->list);
    connect(this->list, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        emit picked(item->data(Qt::UserRole).toString());
    });
    hide();
}

void MentionPopup::setCandidates(const QList<Candidate>& candidates)
{
    this->list->clear();
    for (const Candidate& c : candidates) {
        const QIcon icon(AvatarWidget::pixmapFor(c.id, 22, devicePixelRatioF()));
        auto* item = new QListWidgetItem(icon, QStringLiteral("@%1").arg(c.name), this->list);
        item->setData(Qt::UserRole, c.name);
    }
    if (this->list->count())
        this->list->setCurrentRow(0);
    const int rowH = this->list->sizeHintForRow(0) > 0 ? this->list->sizeHintForRow(0) : 28;
    setFixedSize(220, rowH * int(candidates.size()) + 12);
}

bool MentionPopup::hasCandidates() const
{
    return this->list->count() > 0;
}

void MentionPopup::moveSelection(int delta)
{
    const int n = this->list->count();
    if (!n)
        return;
    this->list->setCurrentRow((this->list->currentRow() + delta + n) % n);
}

QString MentionPopup::currentName() const
{
    const QListWidgetItem* item = this->list->currentItem();
    return item ? item->data(Qt::UserRole).toString() : QString();
}
