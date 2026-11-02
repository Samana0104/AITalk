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
    , m_config(config)
    , m_hub(hub)
{
    setWindowTitle(Text("settings.title"));
    resize(620, 600);

    auto* lay = new QVBoxLayout(this);
    m_tabs = new QTabWidget(this);
    m_tabs->addTab(buildGeneralTab(), Text("settings.tabGeneral"));
    m_tabs->addTab(buildAgentsTab(), Text("settings.tabAgents"));
    m_tabs->addTab(buildAppearanceTab(), Text("settings.tabAppearance"));
    lay->addWidget(m_tabs, 1);

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

    m_userName = new QLineEdit(m_config.userName, page);
    m_roomName = new QLineEdit(m_config.defaultRoomName, page);
    m_speed = dataCombo({ { Text("speed.slow"), "slow" }, { Text("speed.normal"), "normal" }, { Text("speed.fast"), "fast" } }, m_config.speed, page);
    m_autoSleep = spin(0, 600, m_config.autoSleepMinutes, Text("unit.minutes"), page);
    m_autoSleep->setSpecialValueText(Text("settings.neverSleep"));
    m_maxInFlight = spin(1, 4, m_config.maxInFlight, Text("unit.members"), page);
    m_history = spin(8, 200, m_config.historyForPrompt, Text("unit.messages"), page);
    m_turnTimeout = spin(30, 900, m_config.turnTimeoutSec, Text("unit.seconds"), page);
    m_boostMode = dataCombo({ { Text("boost.auto"), "auto" }, { Text("boost.manual"), "manual" }, { Text("boost.off"), "off" } }, m_config.boostMode, page);
    m_spark = new QCheckBox(Text("settings.sparkCheck"), page);
    m_spark->setChecked(m_config.sparkEnabled);

    form->addRow(Text("settings.userName"), m_userName);
    form->addRow(QString(), hint(Text("settings.userNameHint"), page));
    form->addRow(Text("settings.roomName"), m_roomName);
    form->addRow(Text("settings.roomSpeed"), m_speed);
    form->addRow(Text("settings.roomBoost"), m_boostMode);
    form->addRow(Text("settings.autoSleep"), m_autoSleep);
    form->addRow(QString(), hint(Text("settings.autoSleepHint"), page));
    form->addRow(Text("settings.maxInFlight"), m_maxInFlight);
    form->addRow(Text("settings.history"), m_history);
    form->addRow(Text("settings.turnTimeout"), m_turnTimeout);
    form->addRow(Text("settings.spark"), m_spark);
    return page;
}

QWidget* SettingsDialog::buildAgentsTab()
{
    m_agentScroll = new QScrollArea(this);
    m_agentScroll->setWidgetResizable(true);
    m_agentScroll->setFrameShape(QFrame::NoFrame);
    auto* page = new QWidget(m_agentScroll);
    auto* lay = new QVBoxLayout(page);
    lay->setContentsMargins(16, 8, 16, 16);
    lay->addWidget(hint(Text("settings.agentsHint"), page));

    for (const MemberInfo& m : Members::all()) {
        const AgentConfig a = m_config.agents.value(m.id);
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
        f.bin = new QLineEdit(m_config.bins.value(m.cli), f.box);
        const QString detected = m_hub->binOf(m.id);
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
        m_agents.insert(m.id, f);
    }
    lay->addStretch(1);
    m_agentScroll->setWidget(page);
    return m_agentScroll;
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
    m_language = dataCombo(langs, m_config.language, page);
    form->addRow(Text("settings.language"), m_language);
    form->addRow(QString(), hint(Text("settings.languageHint"), page));

    m_theme = dataCombo({ { Text("theme.light"), "light" }, { Text("theme.dark"), "dark" } }, m_config.theme, page);
    m_fontSize = spin(8, 18, m_config.fontSize, QStringLiteral(" pt"), page);
    m_closeToTray = new QCheckBox(Text("settings.closeToTray"), page);
    m_closeToTray->setChecked(m_config.closeToTray);
    form->addRow(Text("settings.theme"), m_theme);
    form->addRow(Text("settings.fontSize"), m_fontSize);
    form->addRow(Text("settings.closeBehavior"), m_closeToTray);
    return page;
}

void SettingsDialog::focusAgent(const QString& memberId)
{
    m_tabs->setCurrentIndex(1);
    const AgentFields f = m_agents.value(memberId);
    if (f.box) {
        m_agentScroll->ensureWidgetVisible(f.box);
        f.model->setFocus();
    }
}

AppConfig SettingsDialog::editedConfig() const
{
    AppConfig c = m_config;
    c.userName = m_userName->text().trimmed().isEmpty() ? Text("config.defaultUserName") : m_userName->text().trimmed();
    c.defaultRoomName = m_roomName->text().trimmed().isEmpty() ? Text("config.defaultRoomName") : m_roomName->text().trimmed();
    c.speed = m_speed->currentData().toString();
    c.autoSleepMinutes = m_autoSleep->value();
    c.maxInFlight = m_maxInFlight->value();
    c.historyForPrompt = m_history->value();
    c.turnTimeoutSec = m_turnTimeout->value();
    c.boostMode = m_boostMode->currentData().toString();
    c.sparkEnabled = m_spark->isChecked();
    c.language = m_language->currentData().toString();
    c.theme = m_theme->currentData().toString();
    c.fontSize = m_fontSize->value();
    c.closeToTray = m_closeToTray->isChecked();

    for (auto it = m_agents.cbegin(); it != m_agents.cend(); ++it) {
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
