#pragma once

#include <QDialog>

class AgentHub;
class QLabel;
class QPlainTextEdit;
class QTableWidget;

// 환경 진단 (CodePet 의 doctor): 멤버별 CLI 경로와 `--version` 결과, 없는 CLI 설치 명령.
class DoctorDialog : public QDialog {
    Q_OBJECT
public:
    explicit DoctorDialog(AgentHub* hub, QWidget* parent = nullptr);

private:
    void runChecks();
    void finishOne();

    AgentHub* m_hub;
    QTableWidget* m_table;
    QLabel* m_summary;
    QPlainTextEdit* m_install;
    int m_pending = 0;
    int m_found = 0;
};
