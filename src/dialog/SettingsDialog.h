#pragma once

#include <QDialog>
#include <QMap>

#include "core/AppConfig.h"

class QCheckBox;
class QComboBox;
class QGroupBox;
class QLineEdit;
class QScrollArea;
class QSpinBox;
class QTabWidget;

namespace AITalk
{

class AgentHub;

// 설정 창: 일반(이름·방·속도·잠들기·진심모드) / 에이전트(모델·effort·CLI 경로) / 화면(테마·글자 크기·트레이).
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    SettingsDialog(const AppConfig& config, const AgentHub* hub, QWidget* parent = nullptr);

    AppConfig editedConfig() const;
    void focusAgent(const QString& memberId);

private:
    QWidget* buildGeneralTab();
    QWidget* buildAgentsTab();
    QWidget* buildAppearanceTab();

private:
    AppConfig config;
    const AgentHub* hub;
    QTabWidget* tabs;

    QLineEdit* userName;
    QLineEdit* roomName;
    QComboBox* speed;
    QSpinBox* autoSleep;
    QSpinBox* maxInFlight;
    QSpinBox* history;
    QSpinBox* turnTimeout;
    QComboBox* boostMode;
    QCheckBox* spark;

    QComboBox* language;
    QComboBox* theme;
    QSpinBox* fontSize;
    QCheckBox* closeToTray;

    struct AgentFields {
        QGroupBox* box = nullptr;
        QLineEdit* model = nullptr;
        QLineEdit* boostModel = nullptr;
        QLineEdit* bin = nullptr;
        QComboBox* effort = nullptr;
        QComboBox* boostEffort = nullptr;
        QString cliKey;
    };
    QMap<QString, AgentFields> agents;
    QScrollArea* agentScroll = nullptr;
};

} // namespace AITalk
