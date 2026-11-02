#pragma once

#include <QWidget>

class AutoResizeTextEdit;
class MentionPopup;
class QFrame;
class QPushButton;
class ReplyPreview;

// 입력창 영역: 인용 답장 미리보기 + 자동 높이 입력칸 + 보내기 버튼 + @멘션 팝업.
class ComposerWidget : public QWidget {
    Q_OBJECT
public:
    explicit ComposerWidget(QWidget* parent = nullptr);

    void setReplyTarget(qint64 messageId, const QString& author, const QString& excerpt);
    void clearReply();
    void focusInput();
    void setHint(const QString& placeholder);

signals:
    void sendRequested(const QString& text, qint64 replyTo);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void send();
    void updateMention();
    void acceptMention(const QString& name);
    void closeMention();
    void positionMention();

    ReplyPreview* reply;
    QFrame* box;
    AutoResizeTextEdit* input;
    QPushButton* sendButton;
    MentionPopup* mention = nullptr;
    qint64 replyTo = 0;
};
