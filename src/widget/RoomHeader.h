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
    AgentChip* chip(const QString& memberId) const { return m_chips.value(memberId); }

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

    QStackedWidget* m_titleStack;
    ElidedLabel* m_title;
    QLineEdit* m_titleEdit;
    QLabel* m_state;
    ChipButton* m_power;
    QComboBox* m_speed;
    QComboBox* m_boost;
    IconButton* m_inspector;
    QMap<QString, AgentChip*> m_chips;
    bool m_updating = false;
};
