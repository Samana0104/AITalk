#include "dialog/SettingsDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

#include "agent/AgentHub.h"
#include "core/Member.h"
#include "util/DataManager.h"

namespace {

QComboBox* effortCombo(const QString& current, QWidget* parent)
{
    auto* c = new QComboBox(parent);
    c->setEditable(true);
    c->addItems({ QString(), QStringLiteral("low"), QStringLiteral("medium"), QStringLiteral("high"), QStringLiteral("xhigh"), QStringLiteral("max") });
    c->setCurrentText(current);
    c->setToolTip(Text("settings.effortHint"));
    return c;
}

QSpinBox* spin(int min, int max, int value, const QString& suffix, QWidget* parent)
{
    auto* s = new QSpinBox(parent);
    s->setRange(min, max);
    s->setValue(value);
    s->setSuffix(suffix);
    return s;
}

QComboBox* dataCombo(const QList<QPair<QString, QString>>& items, const QString& current, QWidget* parent)
{
    auto* c = new QComboBox(parent);
    for (const auto& [label, value] : items)
        c->addItem(label, value);
    c->setCurrentIndex(qMax(0, c->findData(current)));
    return c;
}

QLabel* hint(const QString& text, QWidget* parent)
{
    auto* l = new QLabel(text, parent);
    l->setObjectName(QStringLiteral("dialogHint"));
    l->setWordWrap(true);
    return l;
}

}

SettingsDialog::SettingsDialog(const AppConfig& config, const AgentHub* hub, QWidget* parent)
    : QDialog(parent)
    , config(config)
    , hub(hub)
{
    setWindowTitle(Text("settings.title"));
    resize(620, 600);

    auto* lay = new QVBoxLayout(this);
    this->tabs = new QTabWidget(this);
    this->tabs->addTab(buildGeneralTab(), Text("settings.tabGeneral"));
    this->tabs->addTab(buildAgentsTab(), Text("settings.tabAgents"));
    this->tabs->addTab(buildAppearanceTab(), Text("settings.tabAppearance"));
    lay->addWidget(this->tabs, 1);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Save)->setText(Text("common.save"));
    buttons->button(QDialogButtonBox::Save)->setProperty("variant", "primary");
    buttons->button(QDialogButtonBox::Cancel)->setText(Text("common.cancel"));
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    lay->addWidget(buttons);
}

QWidget* SettingsDialog::buildGeneralTab()
{
    auto* page = new QWidget(this);
    auto* form = new QFormLayout(page);
    form->setContentsMargins(16, 16, 16, 16);
    form->setSpacing(10);

    this->userName = new QLineEdit(this->config.userName, page);
    this->roomName = new QLineEdit(this->config.defaultRoomName, page);
    this->speed = dataCombo({ { Text("speed.slow"), "slow" }, { Text("speed.normal"), "normal" }, { Text("speed.fast"), "fast" } }, this->config.speed, page);
    this->autoSleep = spin(0, 600, this->config.autoSleepMinutes, Text("unit.minutes"), page);
    this->autoSleep->setSpecialValueText(Text("settings.neverSleep"));
    this->maxInFlight = spin(1, 4, this->config.maxInFlight, Text("unit.members"), page);
    this->history = spin(8, 200, this->config.historyForPrompt, Text("unit.messages"), page);
    this->turnTimeout = spin(30, 900, this->config.turnTimeoutSec, Text("unit.seconds"), page);
    this->boostMode = dataCombo({ { Text("boost.auto"), "auto" }, { Text("boost.manual"), "manual" }, { Text("boost.off"), "off" } }, this->config.boostMode, page);
    this->spark = new QCheckBox(Text("settings.sparkCheck"), page);
    this->spark->setChecked(this->config.sparkEnabled);

    form->addRow(Text("settings.userName"), this->userName);
    form->addRow(QString(), hint(Text("settings.userNameHint"), page));
    form->addRow(Text("settings.roomName"), this->roomName);
    form->addRow(Text("settings.roomSpeed"), this->speed);
    form->addRow(Text("settings.roomBoost"), this->boostMode);
    form->addRow(Text("settings.autoSleep"), this->autoSleep);
    form->addRow(QString(), hint(Text("settings.autoSleepHint"), page));
    form->addRow(Text("settings.maxInFlight"), this->maxInFlight);
    form->addRow(Text("settings.history"), this->history);
    form->addRow(Text("settings.turnTimeout"), this->turnTimeout);
    form->addRow(Text("settings.spark"), this->spark);
    return page;
}

QWidget* SettingsDialog::buildAgentsTab()
{
    this->agentScroll = new QScrollArea(this);
    this->agentScroll->setWidgetResizable(true);
    this->agentScroll->setFrameShape(QFrame::NoFrame);
    auto* page = new QWidget(this->agentScroll);
    auto* lay = new QVBoxLayout(page);
    lay->setContentsMargins(16, 8, 16, 16);
    lay->addWidget(hint(Text("settings.agentsHint"), page));

    for (const MemberInfo& m : Members::all()) {
        const AgentConfig a = this->config.agents.value(m.id);
        AgentFields f;
        f.cliKey = m.cli;
        f.box = new QGroupBox(QStringLiteral("%1  ·  %2 CLI").arg(m.name, m.cli), page);
        auto* form = new QFormLayout(f.box);
        f.model = new QLineEdit(a.model, f.box);
        f.effort = effortCombo(a.effort, f.box);
        f.boostModel = new QLineEdit(a.boostModel, f.box);
        f.boostModel->setPlaceholderText(Text("settings.sameModel"));
        f.boostEffort = effortCombo(a.boostEffort, f.box);

        auto* binRow = new QHBoxLayout;
        f.bin = new QLineEdit(this->config.bins.value(m.cli), f.box);
        const QString detected = this->hub->binOf(m.id);
        f.bin->setPlaceholderText(detected.isEmpty() ? Text("settings.binNotFound") : Text("settings.binAuto").arg(detected));
        auto* browse = new QPushButton(Text("common.browse"), f.box);
        binRow->addWidget(f.bin, 1);
        binRow->addWidget(browse);
        QLineEdit* binEdit = f.bin;
        connect(browse, &QPushButton::clicked, this, [this, binEdit] {
            const QString file = QFileDialog::getOpenFileName(this, Text("settings.pickBin"));
            if (!file.isEmpty())
                binEdit->setText(file);
        });

        form->addRow(Text("settings.model"), f.model);
        form->addRow(QStringLiteral("effort"), f.effort);
        form->addRow(Text("settings.boostModel"), f.boostModel);
        form->addRow(Text("settings.boostEffort"), f.boostEffort);
        form->addRow(Text("settings.binPath"), binRow);
        lay->addWidget(f.box);
        this->agents.insert(m.id, f);
    }
    lay->addStretch(1);
    this->agentScroll->setWidget(page);
    return this->agentScroll;
}

QWidget* SettingsDialog::buildAppearanceTab()
{
    auto* page = new QWidget(this);
    auto* form = new QFormLayout(page);
    form->setContentsMargins(16, 16, 16, 16);
    form->setSpacing(10);

    // 언어 목록은 strings.csv 머리줄에서 온다 (열을 추가하면 여기도 늘어난다)
    QList<QPair<QString, QString>> langs { { Text("settings.languageAuto"), QStringLiteral("auto") } };
    const DataManager& data = DataManager::instance();
    for (const QString& code : data.languages())
        langs.append({ data.languageName(code), code });
    this->language = dataCombo(langs, this->config.language, page);
    form->addRow(Text("settings.language"), this->language);
    form->addRow(QString(), hint(Text("settings.languageHint"), page));

    this->theme = dataCombo({ { Text("theme.light"), "light" }, { Text("theme.dark"), "dark" } }, this->config.theme, page);
    this->fontSize = spin(8, 18, this->config.fontSize, QStringLiteral(" pt"), page);
    this->closeToTray = new QCheckBox(Text("settings.closeToTray"), page);
    this->closeToTray->setChecked(this->config.closeToTray);
    form->addRow(Text("settings.theme"), this->theme);
    form->addRow(Text("settings.fontSize"), this->fontSize);
    form->addRow(Text("settings.closeBehavior"), this->closeToTray);
    return page;
}

void SettingsDialog::focusAgent(const QString& memberId)
{
    this->tabs->setCurrentIndex(1);
    const AgentFields f = this->agents.value(memberId);
    if (f.box) {
        this->agentScroll->ensureWidgetVisible(f.box);
        f.model->setFocus();
    }
}

AppConfig SettingsDialog::editedConfig() const
{
    AppConfig c = this->config;
    c.userName = this->userName->text().trimmed().isEmpty() ? Text("config.defaultUserName") : this->userName->text().trimmed();
    c.defaultRoomName = this->roomName->text().trimmed().isEmpty() ? Text("config.defaultRoomName") : this->roomName->text().trimmed();
    c.speed = this->speed->currentData().toString();
    c.autoSleepMinutes = this->autoSleep->value();
    c.maxInFlight = this->maxInFlight->value();
    c.historyForPrompt = this->history->value();
    c.turnTimeoutSec = this->turnTimeout->value();
    c.boostMode = this->boostMode->currentData().toString();
    c.sparkEnabled = this->spark->isChecked();
    c.language = this->language->currentData().toString();
    c.theme = this->theme->currentData().toString();
    c.fontSize = this->fontSize->value();
    c.closeToTray = this->closeToTray->isChecked();

    for (auto it = this->agents.cbegin(); it != this->agents.cend(); ++it) {
        const AgentFields& f = it.value();
        AgentConfig a = c.agents.value(it.key());
        a.model = f.model->text().trimmed();
        a.effort = f.effort->currentText().trimmed();
        a.boostModel = f.boostModel->text().trimmed();
        a.boostEffort = f.boostEffort->currentText().trimmed();
        c.agents.insert(it.key(), a);
        c.bins.insert(f.cliKey, f.bin->text().trimmed());
    }
    return c;
}
