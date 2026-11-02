#pragma once

#include <QDialog>
#include <QMap>

#include "core/AppConfig.h"

class AgentHub;
class QCheckBox;
class QComboBox;
class QGroupBox;
class QLineEdit;
class QScrollArea;
class QSpinBox;
class QTabWidget;

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

    AppConfig m_config;
    const AgentHub* m_hub;
    QTabWidget* m_tabs;

    QLineEdit* m_userName;
    QLineEdit* m_roomName;
    QComboBox* m_speed;
    QSpinBox* m_autoSleep;
    QSpinBox* m_maxInFlight;
    QSpinBox* m_history;
    QSpinBox* m_turnTimeout;
    QComboBox* m_boostMode;
    QCheckBox* m_spark;

    QComboBox* m_language;
    QComboBox* m_theme;
    QSpinBox* m_fontSize;
    QCheckBox* m_closeToTray;

    struct AgentFields {
        QGroupBox* box = nullptr;
        QLineEdit* model = nullptr;
        QComboBox* effort = nullptr;
        QLineEdit* boostModel = nullptr;
        QComboBox* boostEffort = nullptr;
        QLineEdit* bin = nullptr;
        QString cliKey;
    };
    QMap<QString, AgentFields> m_agents;
    QScrollArea* m_agentScroll = nullptr;
};
