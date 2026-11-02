#pragma once

#include <QObject>

class QWidget;
class QWindow;

// 프레임 없는 창의 가장자리 크기 조절.
// 위젯이 아니라 QWindow 에 이벤트 필터를 걸어야 자식 위젯 위에서도 마우스 이동을 다 받는다.
// 가장자리에서 누르면 OS 의 startSystemResize 로 넘긴다 (Windows/macOS/Linux 공통).
class FramelessHelper : public QObject {
    Q_OBJECT
public:
    explicit FramelessHelper(QWidget* window, int border = 6);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    Qt::Edges edgesAt(const QPoint& pos) const;
    void setEdgeCursor(Qt::Edges edges);

    QWidget* widget;
    QWindow* window = nullptr;
    int border;
    bool overriding = false;
    Qt::Edges current;
};
