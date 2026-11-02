#pragma once

#include <QFrame>

class QLabel;

// 입력창 위 "↪ 답장할 메시지" 미리보기와 해제(×) 버튼.
class ReplyPreview : public QFrame {
    Q_OBJECT
public:
    explicit ReplyPreview(QWidget* parent = nullptr);

    void setTarget(const QString& author, const QString& excerpt);

signals:
    void cleared();

private:
    QLabel* author;
    QLabel* excerpt;
};
