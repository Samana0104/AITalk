#pragma once

#include <QMap>
#include <QMetaObject>
#include <QPointer>
#include <QWidget>

class Application;
class ChatSession;
class MemberCard;
class QComboBox;
class QPlainTextEdit;
class QPushButton;

// 오른쪽 패널 (ai-chatroom 의 오른쪽 패널에서 가져옴):
//  - 멤버 탭: 멤버별 상태·모델·호출 수·오류
//  - 메모 탭: 멤버가 스스로 적은 개인 메모 (방장이 직접 고칠 수도 있다)
class InspectorPanel : public QWidget {
    Q_OBJECT
public:
    explicit InspectorPanel(Application* app, QWidget* parent = nullptr);

private:
    void bindSession(ChatSession* session);
    void refreshMember(const QString& memberId);
    void loadNote();
    void saveNote();

    Application* m_app;
    QPointer<ChatSession> m_session;
    QMetaObject::Connection m_noteConn;
    QMap<QString, MemberCard*> m_cards;
    QComboBox* m_noteMember;
    QPlainTextEdit* m_note;
    QPushButton* m_saveNote;
    bool m_noteDirty = false;
};
