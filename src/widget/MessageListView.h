#pragma once

#include <QHash>
#include <QPointer>
#include <QScrollArea>

#include "core/ChatMessage.h"

class ChatSession;
class MessageBubble;
class QPushButton;
class QVBoxLayout;

// 대화 목록. 최근 메시지부터 pageSize 개씩 보여 주고, 위의 버튼으로 이전 대화를 더 불러온다
// (읽던 위치 유지). 맨 아래를 보고 있을 때만 새 메시지에 맞춰 따라 내려간다.
class MessageListView : public QScrollArea {
    Q_OBJECT
public:
    static constexpr int pageSize = 50;

    explicit MessageListView(QWidget* parent = nullptr);

    void setSession(ChatSession* session, const QString& userName);
    void setUserName(const QString& userName);
    void appendMessage(const ChatMessage& message);
    void updateMessage(const ChatMessage& message);
    void scrollToBottom();

signals:
    void replyRequested(qint64 messageId);
    void reactionRequested(qint64 messageId, const QString& emoji);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    MessageBubble* makeBubble(const ChatMessage& message);
    void clear();
    void loadOlder();
    void updateLoadMore();
    int bubbleWidth() const;

    QPointer<ChatSession> session;
    QString userName;
    QWidget* canvas;
    QVBoxLayout* layout;
    QPushButton* loadMore;
    QHash<qint64, MessageBubble*> rows;
    int firstIndex = 0;      // 화면에 있는 가장 오래된 메시지의 인덱스
    bool stickToBottom = true;       // 맨 아래를 따라갈지
    int anchorFromBottom = -1; // 이전 대화를 불러온 뒤 지켜야 할 위치
};
