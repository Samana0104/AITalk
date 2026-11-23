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

private:
    AgentHub* hub;
    QTableWidget* table;
    QLabel* summary;
    QPlainTextEdit* install;
    int pending = 0;
    int found = 0;
};
