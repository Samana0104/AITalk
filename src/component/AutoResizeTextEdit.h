#pragma once

#include <QPlainTextEdit>

// 내용에 맞춰 1줄 ~ maxLines 줄까지 늘어나는 입력창.
// Enter = submitRequested, Shift+Enter = 줄바꿈.
class AutoResizeTextEdit : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit AutoResizeTextEdit(QWidget* parent = nullptr);

    void setMaxLines(int lines);

signals:
    void submitRequested();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void updateHeight();

    int maxLines = 6;
};
