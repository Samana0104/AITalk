#include "widget/RoomHeader.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "component/ChipButton.h"
#include "component/ElidedLabel.h"
#include "component/IconButton.h"
#include "core/Member.h"
#include "util/DataManager.h"
#include "widget/AgentChip.h"

namespace {
// 스타일시트 여백 때문에 기본 크기 계산이 글자를 자르므로 가장 긴 항목에 맞춘다.
void fitCombo(QComboBox* c)
{
    int w = 0;
    for (int i = 0; i < c->count(); ++i)
        w = qMax(w, c->fontMetrics().horizontalAdvance(c->itemText(i)));
    c->setMinimumWidth(w + 44);
}
}

RoomHeader::RoomHeader(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("roomHeader"));
    setAttribute(Qt::WA_StyledBackground);

    // 1줄: 제목 · 상태 · 참가자 칩 · 패널 토글
    // 2줄: 방 켜기 · 속도 · 진심모드 · 내보내기
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(16, 8, 10, 8);
    lay->setSpacing(6);

    auto* titleRow = new QHBoxLayout;
    titleRow->setSpacing(8);
    m_titleStack = new QStackedWidget(this);
    m_title = new ElidedLabel(QString(), m_titleStack);
    m_title->setObjectName(QStringLiteral("roomTitle"));
    m_title->setToolTip(Text("room.renameTip"));
    m_titleEdit = new QLineEdit(m_titleStack);
    m_titleStack->addWidget(m_title);
    m_titleStack->addWidget(m_titleEdit);
    m_titleStack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_titleStack->setFixedHeight(28);
    m_state = new QLabel(this);
    m_state->setObjectName(QStringLiteral("roomState"));
    titleRow->addWidget(m_titleStack, 1);
    titleRow->addWidget(m_state);
    titleRow->addSpacing(8);
    for (const QString& id : Members::ids()) {
        auto* chip = new AgentChip(id, this);
        connect(chip, &QPushButton::clicked, this, [this, id] { emit agentClicked(id); });
        m_chips.insert(id, chip);
        titleRow->addWidget(chip);
    }
    m_inspector = new IconButton(QStringLiteral("sidebar-right"), Text("room.inspectorTip"), this);
    m_inspector->setCheckable(true);
    m_inspector->setChecked(true);
    titleRow->addWidget(m_inspector);
    lay->addLayout(titleRow);

    auto* controls = new QHBoxLayout;
    controls->setSpacing(6);
    m_power = new ChipButton(QStringLiteral("power"), Text("room.powerOn"), this);
    m_power->setCheckable(true);
    m_speed = new QComboBox(this);
    m_speed->addItem(Text("speed.slow"), QStringLiteral("slow"));
    m_speed->addItem(Text("room.speedNormal"), QStringLiteral("normal"));
    m_speed->addItem(Text("speed.fast"), QStringLiteral("fast"));
    m_speed->setToolTip(Text("room.speedTip"));
    m_boost = new QComboBox(this);
    m_boost->addItem(Text("room.boostAuto"), QStringLiteral("auto"));
    m_boost->addItem(Text("room.boostManual"), QStringLiteral("manual"));
    m_boost->addItem(Text("room.boostOff"), QStringLiteral("off"));
    m_boost->setToolTip(Text("room.boostTip"));
    fitCombo(m_speed);
    fitCombo(m_boost);
    auto* exportBtn = new ChipButton(QStringLiteral("export"), Text("room.export"), this);
    exportBtn->setToolTip(Text("room.exportTip"));
    controls->addWidget(m_power);
    controls->addWidget(m_speed);
    controls->addWidget(m_boost);
    controls->addWidget(exportBtn);
    controls->addStretch(1);
    lay->addLayout(controls);

    connect(m_title, &ElidedLabel::doubleClicked, this, &RoomHeader::beginRename);
    connect(m_titleEdit, &QLineEdit::editingFinished, this, &RoomHeader::commitRename);
    connect(m_power, &QPushButton::clicked, this, [this](bool on) { emit powerToggled(on); });
    connect(m_speed, &QComboBox::currentIndexChanged, this, [this] {
        if (!m_updating)
            emit speedChanged(m_speed->currentData().toString());
    });
    connect(m_boost, &QComboBox::currentIndexChanged, this, [this] {
        if (!m_updating)
            emit boostModeChanged(m_boost->currentData().toString());
    });
    connect(exportBtn, &QPushButton::clicked, this, &RoomHeader::exportRequested);
    connect(m_inspector, &QToolButton::toggled, this, &RoomHeader::inspectorToggled);

    setRunning(false, false);
}

void RoomHeader::setRoom(const QString& title, const QString& speed, const QString& boostMode)
{
    m_updating = true;
    m_title->setText(title);
    m_speed->setCurrentIndex(qMax(0, m_speed->findData(speed)));
    m_boost->setCurrentIndex(qMax(0, m_boost->findData(boostMode)));
    m_updating = false;
}

void RoomHeader::setRunning(bool running, bool sleeping)
{
    m_power->setChecked(running);
    m_power->setText(running ? Text("room.powerOnState") : Text("room.powerOn"));
    m_state->setText(running ? Text("room.stateRunning") : sleeping ? Text("room.stateSleeping") : Text("room.stateOff"));
}

void RoomHeader::setAgentState(const QString& memberId, const QString& status, const QString& model, bool boosting)
{
    if (AgentChip* c = m_chips.value(memberId))
        c->setState(status, model, boosting);
}

void RoomHeader::setInspectorVisible(bool visible)
{
    const QSignalBlocker block(m_inspector);
    m_inspector->setChecked(visible);
}

void RoomHeader::beginRename()
{
    m_titleEdit->setText(m_title->text());
    m_titleStack->setCurrentWidget(m_titleEdit);
    m_titleEdit->setFocus();
    m_titleEdit->selectAll();
}

void RoomHeader::commitRename()
{
    if (m_titleStack->currentWidget() != m_titleEdit)
        return;
    m_titleStack->setCurrentWidget(m_title);
    const QString t = m_titleEdit->text().trimmed();
    if (!t.isEmpty() && t != m_title->text())
        emit renameRequested(t);
}
