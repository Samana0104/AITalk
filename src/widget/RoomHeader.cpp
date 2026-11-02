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
    this->titleStack = new QStackedWidget(this);
    this->title = new ElidedLabel(QString(), this->titleStack);
    this->title->setObjectName(QStringLiteral("roomTitle"));
    this->title->setToolTip(Text("room.renameTip"));
    this->titleEdit = new QLineEdit(this->titleStack);
    this->titleStack->addWidget(this->title);
    this->titleStack->addWidget(this->titleEdit);
    this->titleStack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    this->titleStack->setFixedHeight(28);
    this->state = new QLabel(this);
    this->state->setObjectName(QStringLiteral("roomState"));
    titleRow->addWidget(this->titleStack, 1);
    titleRow->addWidget(this->state);
    titleRow->addSpacing(8);
    for (const QString& id : Members::ids()) {
        auto* chip = new AgentChip(id, this);
        connect(chip, &QPushButton::clicked, this, [this, id] { emit agentClicked(id); });
        this->chips.insert(id, chip);
        titleRow->addWidget(chip);
    }
    this->inspector = new IconButton(QStringLiteral("sidebar-right"), Text("room.inspectorTip"), this);
    this->inspector->setCheckable(true);
    this->inspector->setChecked(true);
    titleRow->addWidget(this->inspector);
    lay->addLayout(titleRow);

    auto* controls = new QHBoxLayout;
    controls->setSpacing(6);
    this->power = new ChipButton(QStringLiteral("power"), Text("room.powerOn"), this);
    this->power->setCheckable(true);
    this->speed = new QComboBox(this);
    this->speed->addItem(Text("speed.slow"), QStringLiteral("slow"));
    this->speed->addItem(Text("room.speedNormal"), QStringLiteral("normal"));
    this->speed->addItem(Text("speed.fast"), QStringLiteral("fast"));
    this->speed->setToolTip(Text("room.speedTip"));
    this->boost = new QComboBox(this);
    this->boost->addItem(Text("room.boostAuto"), QStringLiteral("auto"));
    this->boost->addItem(Text("room.boostManual"), QStringLiteral("manual"));
    this->boost->addItem(Text("room.boostOff"), QStringLiteral("off"));
    this->boost->setToolTip(Text("room.boostTip"));
    fitCombo(this->speed);
    fitCombo(this->boost);
    auto* exportBtn = new ChipButton(QStringLiteral("export"), Text("room.export"), this);
    exportBtn->setToolTip(Text("room.exportTip"));
    controls->addWidget(this->power);
    controls->addWidget(this->speed);
    controls->addWidget(this->boost);
    controls->addWidget(exportBtn);
    controls->addStretch(1);
    lay->addLayout(controls);

    connect(this->title, &ElidedLabel::doubleClicked, this, &RoomHeader::beginRename);
    connect(this->titleEdit, &QLineEdit::editingFinished, this, &RoomHeader::commitRename);
    connect(this->power, &QPushButton::clicked, this, [this](bool on) { emit powerToggled(on); });
    connect(this->speed, &QComboBox::currentIndexChanged, this, [this] {
        if (!this->updating)
            emit speedChanged(this->speed->currentData().toString());
    });
    connect(this->boost, &QComboBox::currentIndexChanged, this, [this] {
        if (!this->updating)
            emit boostModeChanged(this->boost->currentData().toString());
    });
    connect(exportBtn, &QPushButton::clicked, this, &RoomHeader::exportRequested);
    connect(this->inspector, &QToolButton::toggled, this, &RoomHeader::inspectorToggled);

    setRunning(false, false);
}

void RoomHeader::setRoom(const QString& title, const QString& speed, const QString& boostMode)
{
    this->updating = true;
    this->title->setText(title);
    this->speed->setCurrentIndex(qMax(0, this->speed->findData(speed)));
    this->boost->setCurrentIndex(qMax(0, this->boost->findData(boostMode)));
    this->updating = false;
}

void RoomHeader::setRunning(bool running, bool sleeping)
{
    this->power->setChecked(running);
    this->power->setText(running ? Text("room.powerOnState") : Text("room.powerOn"));
    this->state->setText(running ? Text("room.stateRunning") : sleeping ? Text("room.stateSleeping") : Text("room.stateOff"));
}

void RoomHeader::setAgentState(const QString& memberId, const QString& status, const QString& model, bool boosting)
{
    if (AgentChip* c = this->chips.value(memberId))
        c->setState(status, model, boosting);
}

void RoomHeader::setInspectorVisible(bool visible)
{
    const QSignalBlocker block(this->inspector);
    this->inspector->setChecked(visible);
}

void RoomHeader::beginRename()
{
    this->titleEdit->setText(this->title->text());
    this->titleStack->setCurrentWidget(this->titleEdit);
    this->titleEdit->setFocus();
    this->titleEdit->selectAll();
}

void RoomHeader::commitRename()
{
    if (this->titleStack->currentWidget() != this->titleEdit)
        return;
    this->titleStack->setCurrentWidget(this->title);
    const QString t = this->titleEdit->text().trimmed();
    if (!t.isEmpty() && t != this->title->text())
        emit renameRequested(t);
}
