#pragma once

#include <QLabel>
#include <QTimer>

// 창 아래쪽 가운데에 잠깐 떴다 사라지는 알림.
class Toast : public QLabel {
    Q_OBJECT
public:
    explicit Toast(QWidget* host);

    void showMessage(const QString& text, int ms = 2600);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void reposition();

    QTimer m_timer;
};
