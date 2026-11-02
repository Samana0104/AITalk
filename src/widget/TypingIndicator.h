#pragma once

#include <QList>
#include <QPair>
#include <QTimer>
#include <QWidget>

class QHBoxLayout;
class QLabel;

// 입력창 위 "Claude 입력 중…", "Gemini 읽는 중…" 표시. 아무도 없으면 숨는다.
class TypingIndicator : public QWidget {
    Q_OBJECT
public:
    explicit TypingIndicator(QWidget* parent = nullptr);

    // (이름, 상태 문구) 목록
    void setEntries(const QList<QPair<QString, QString>>& entries);

private:
    void tickDots();

    QHBoxLayout* m_layout;
    QList<QLabel*> m_pills;
    QList<QPair<QString, QString>> m_entries;
    QTimer m_timer;
    int m_dots = 0;
};
