#pragma once

#include <QFrame>

class AvatarWidget;
class QLabel;
class StatusDot;

// 오른쪽 패널의 멤버 상태 카드: 프로필·이름·상태·모델·호출 수·마지막 오류.
class MemberCard : public QFrame {
    Q_OBJECT
public:
    explicit MemberCard(const QString& memberId, QWidget* parent = nullptr);

    QString memberId() const { return this->id; }

    struct State {
        QString status;
        QString model;
        QString cliPath;
        int calls = 0;
        qint64 lastMs = 0;
        QString lastError;
    };
    void setState(const State& s);

private:
    QString id;
    StatusDot* dot;
    QLabel* status;
    QLabel* model;
    QLabel* stats;
    QLabel* error;
};
