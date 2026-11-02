#pragma once

#include <QList>
#include <QMetaObject>
#include <QPointer>
#include <QWidget>

class Application;
class ChatSession;
class ComposerWidget;
class MessageListView;
class RoomHeader;
class TypingIndicator;

// 가운데 채팅 영역: 방 헤더 + 대화 목록 + 입력 중 표시 + 입력창.
// 현재 세션과 RoomEngine 을 화면에 묶는다.
class ChatPanel : public QWidget {
    Q_OBJECT
public:
    explicit ChatPanel(Application* app, QWidget* parent = nullptr);

    void setInspectorVisible(bool visible);

signals:
    void inspectorToggled(bool visible);
    void agentSettingsRequested(const QString& memberId);
    void exportRequested(ChatSession* session);
    void toast(const QString& text);

private:
    void bindSession(ChatSession* session);
    void refreshRoom();
    void refreshMember(const QString& memberId);
    void refreshTyping();
    void showAgentMenu(const QString& memberId);
    void startReply(qint64 messageId);

    Application* m_app;
    QPointer<ChatSession> m_session;
    QList<QMetaObject::Connection> m_sessionConns;

    RoomHeader* m_header;
    MessageListView* m_messages;
    TypingIndicator* m_typing;
    ComposerWidget* m_composer;
};
