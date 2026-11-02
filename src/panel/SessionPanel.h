#pragma once

#include <QTimer>
#include <QWidget>

class ChatSession;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class SessionManager;

// 왼쪽 세션 사이드바 (CodePet 의 세션 목록):
// 새 채팅 / 메시지 검색 / 세션 목록(우클릭: 이름 바꾸기·내보내기·삭제) / 환경 진단·CLI 다시 탐지.
class SessionPanel : public QWidget {
    Q_OBJECT
public:
    explicit SessionPanel(SessionManager* sessions, QWidget* parent = nullptr);

signals:
    void doctorRequested();
    void refreshCliRequested();
    void exportRequested(ChatSession* session);

private:
    void scheduleRebuild();
    void rebuild();
    void rebuildSessions();
    void rebuildSearch(const QString& query);
    void onItemClicked(QListWidgetItem* item);
    void showContextMenu(const QPoint& pos);
    void renameSession(ChatSession* session);
    void deleteSession(ChatSession* session);

    SessionManager* sessions;
    QLineEdit* search;
    QListWidget* list;
    QTimer rebuildTimer;
};
