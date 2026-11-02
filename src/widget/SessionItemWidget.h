#pragma once

#include <QWidget>

class ElidedLabel;
class QLabel;

// 세션 목록 한 칸: 제목 / 마지막 메시지 미리보기 / 상대 시간.
class SessionItemWidget : public QWidget {
    Q_OBJECT
public:
    explicit SessionItemWidget(QWidget* parent = nullptr);

    void setContent(const QString& title, const QString& preview, qint64 updatedAt);
    static QString relativeTime(qint64 ts);

private:
    ElidedLabel* title;
    ElidedLabel* preview;
    QLabel* time;
};
