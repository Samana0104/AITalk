#pragma once

#include <QWidget>

#include "core/ChatMessage.h"

class QFrame;
class QHBoxLayout;
class QLabel;

// 메시지 한 줄.
//  - AI: 왼쪽 프로필 + 이름·시간·모델 + 말풍선(마크다운) 또는 캐릭터 이모티콘 + 반응
//  - 방장: 오른쪽 정렬, 진한 말풍선
//  - 시스템: 가운데 작은 알림
// 말풍선에 마우스를 올리면 답장·복사 버튼, 우클릭하면 답장·복사·반응 메뉴.
class MessageBubble : public QWidget {
    Q_OBJECT
public:
    struct Context {
        QString userName;
        QString replyAuthor;   // 인용 대상 작성자 (없으면 빈 문자열)
        QString replyExcerpt;
    };

    MessageBubble(const ChatMessage& message, const Context& ctx, QWidget* parent = nullptr);

    qint64 messageId() const { return m_msg.id; }
    const ChatMessage& message() const { return m_msg; }

    void updateMessage(const ChatMessage& message);
    void setMaxBubbleWidth(int width);

    static const QStringList& quickReactions();

signals:
    void replyRequested(qint64 messageId);
    void reactionRequested(qint64 messageId, const QString& emoji);

protected:
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void buildSystem();
    void buildChat();
    QFrame* buildTextBubble(bool mine);
    QFrame* buildEmoticon();
    void rebuildReactions();
    void showMenu(const QPoint& globalPos);
    void copyText();

    ChatMessage m_msg;
    Context m_ctx;
    QFrame* m_bubble = nullptr;
    QFrame* m_replyFrame = nullptr;
    QLabel* m_text = nullptr;
    QWidget* m_actions = nullptr;
    QWidget* m_reactionRow = nullptr;
    QHBoxLayout* m_reactionLayout = nullptr;
    int m_idealTextWidth = -1; // 줄바꿈 없이 쓸 때 글 폭 (한 번만 잰다)
};
