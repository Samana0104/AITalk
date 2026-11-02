#pragma once

#include <QWidget>

class IconButton;
class QLabel;

// 프레임 없는 창의 타이틀바 (CodePet 스타일): 로고·제목 + 설정·최소화·최대화·닫기.
// 끌면 창 이동, 더블클릭하면 최대화/복원.
class TitleBar : public QWidget {
    Q_OBJECT
public:
    explicit TitleBar(QWidget* window);

    void setTitle(const QString& title);

signals:
    void settingsClicked();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void toggleMaximize();
    void updateMaximizeIcon();

    QWidget* m_window;
    QLabel* m_title;
    IconButton* m_max;
};
