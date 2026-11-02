#pragma once

#include <QMap>
#include <QWidget>

class AgentChip;
class ChipButton;
class ElidedLabel;
class IconButton;
class QComboBox;
class QLabel;
class QLineEdit;
class QStackedWidget;

// 채팅 영역 위쪽 바: 방 제목(더블클릭으로 이름 변경) + 방 켜기 + 대화 속도 + 진심모드 +
// 내보내기 + 참가자 칩 + 오른쪽 패널 토글.
class RoomHeader : public QWidget {
    Q_OBJECT
public:
    explicit RoomHeader(QWidget* parent = nullptr);

    void setRoom(const QString& title, const QString& speed, const QString& boostMode);
    void setRunning(bool running, bool sleeping);
    void setAgentState(const QString& memberId, const QString& status, const QString& model, bool boosting);
    void setInspectorVisible(bool visible);
    AgentChip* chip(const QString& memberId) const { return this->chips.value(memberId); }

signals:
    void renameRequested(const QString& title);
    void powerToggled(bool on);
    void speedChanged(const QString& speed);
    void boostModeChanged(const QString& mode);
    void exportRequested();
    void inspectorToggled(bool visible);
    void agentClicked(const QString& memberId);

private:
    void beginRename();
    void commitRename();

    QStackedWidget* titleStack;
    ElidedLabel* title;
    QLineEdit* titleEdit;
    QLabel* state;
    ChipButton* power;
    QComboBox* speed;
    QComboBox* boost;
    IconButton* inspector;
    QMap<QString, AgentChip*> chips;
    bool updating = false;
};
