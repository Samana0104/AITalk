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
    m_list = new QListWidget(this);
    m_list->setObjectName(QStringLiteral("mentionList"));
    m_list->setFocusPolicy(Qt::NoFocus);
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->setIconSize(QSize(22, 22));
    lay->addWidget(m_list);
    connect(m_list, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        emit picked(item->data(Qt::UserRole).toString());
    });
    hide();
}

void MentionPopup::setCandidates(const QList<Candidate>& candidates)
{
    m_list->clear();
    for (const Candidate& c : candidates) {
        const QIcon icon(AvatarWidget::pixmapFor(c.id, 22, devicePixelRatioF()));
        auto* item = new QListWidgetItem(icon, QStringLiteral("@%1").arg(c.name), m_list);
        item->setData(Qt::UserRole, c.name);
    }
    if (m_list->count())
        m_list->setCurrentRow(0);
    const int rowH = m_list->sizeHintForRow(0) > 0 ? m_list->sizeHintForRow(0) : 28;
    setFixedSize(220, rowH * int(candidates.size()) + 12);
}

bool MentionPopup::hasCandidates() const
{
    return m_list->count() > 0;
}

void MentionPopup::moveSelection(int delta)
{
    const int n = m_list->count();
    if (!n)
        return;
    m_list->setCurrentRow((m_list->currentRow() + delta + n) % n);
}

QString MentionPopup::currentName() const
{
    const QListWidgetItem* item = m_list->currentItem();
    return item ? item->data(Qt::UserRole).toString() : QString();
}
