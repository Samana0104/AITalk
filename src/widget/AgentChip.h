#pragma once

#include <QPushButton>

class AvatarWidget;
class QLabel;
class StatusDot;

// 방 헤더의 참가자 칩: 작은 프로필 + 이름 + 상태 점. 누르면 ChatPanel 이 메뉴를 띄운다.
class AgentChip : public QPushButton {
    Q_OBJECT
public:
    AgentChip(const QString& memberId, QWidget* parent = nullptr);

    QString memberId() const { return this->id; }
    void setState(const QString& status, const QString& modelLabel, bool boosting);

    // 글자 없는 QPushButton 이라 크기는 안쪽 레이아웃 기준으로
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

private:
    QString id;
    AvatarWidget* avatar;
    QLabel* name;
    StatusDot* dot;
};
