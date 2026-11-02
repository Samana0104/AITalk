#pragma once

#include <QPushButton>

// 둥근 알약 모양 버튼 (CodePet 의 chip-button). checkable 로 켜면 켜진 상태가 진하게 보인다.
class ChipButton : public QPushButton {
    Q_OBJECT
public:
    explicit ChipButton(const QString& text, QWidget* parent = nullptr);
    ChipButton(const QString& iconName, const QString& text, QWidget* parent = nullptr);

private:
    void refreshIcon();

    QString m_iconName;
};
