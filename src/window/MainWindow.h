#pragma once

#include <QWidget>

class Application;
class ChatPanel;
class ChatSession;
class InspectorPanel;
class QSplitter;
class SessionPanel;
class TitleBar;
class Toast;

// 프레임 없는 메인 창.
//  ┌──────────────────── TitleBar ────────────────────┐
//  │ SessionPanel │       ChatPanel        │ Inspector │
//  └──────────────────────────────────────────────────┘
class MainWindow : public QWidget {
    Q_OBJECT
public:
    explicit MainWindow(Application* app, QWidget* parent = nullptr);

    void showToast(const QString& text);
    void openSettings(const QString& agentId = {});
    void openDoctor();
    void exportSession(ChatSession* session);

protected:
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;

private:
    void restoreUiState();
    void saveUiState() const;
    void setInspectorVisible(bool visible);

    Application* m_app;
    TitleBar* m_titleBar;
    QSplitter* m_splitter;
    SessionPanel* m_sessionPanel;
    ChatPanel* m_chatPanel;
    InspectorPanel* m_inspector;
    Toast* m_toast;
    bool m_trayHintShown = false;
};
