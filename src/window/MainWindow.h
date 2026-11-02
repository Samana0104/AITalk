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

    Application* app;
    TitleBar* titleBar;
    QSplitter* splitter;
    SessionPanel* sessionPanel;
    ChatPanel* chatPanel;
    InspectorPanel* inspector;
    Toast* toast;
    bool trayHintShown = false;
};
