#include "window/MainWindow.h"

#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include <QSettings>
#include <QSplitter>
#include <QStandardPaths>
#include <QSystemTrayIcon>
#include <QVBoxLayout>

#include "agent/AgentHub.h"
#include "app/AppPaths.h"
#include "app/Application.h"
#include "component/Toast.h"
#include "dialog/DoctorDialog.h"
#include "dialog/SettingsDialog.h"
#include "panel/ChatPanel.h"
#include "panel/InspectorPanel.h"
#include "panel/SessionPanel.h"
#include "storage/ChatExporter.h"
#include "storage/ChatSession.h"
#include "util/DataManager.h"
#include "widget/TitleBar.h"
#include "window/FramelessHelper.h"

MainWindow::MainWindow(Application* app, QWidget* parent)
    : QWidget(parent)
    , app(app)
{
    setObjectName(QStringLiteral("mainRoot"));
    setAttribute(Qt::WA_StyledBackground);
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setWindowTitle(QStringLiteral("AITalk"));
    setMinimumSize(820, 520);
    resize(1240, 800);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(1, 1, 1, 1); // mainRoot 테두리 1px
    root->setSpacing(0);

    this->titleBar = new TitleBar(this);
    root->addWidget(this->titleBar);

    this->splitter = new QSplitter(Qt::Horizontal, this);
    this->splitter->setHandleWidth(1);
    this->splitter->setChildrenCollapsible(false);
    this->sessionPanel = new SessionPanel(this->app->sessions(), this->splitter);
    this->chatPanel = new ChatPanel(this->app, this->splitter);
    this->inspector = new InspectorPanel(this->app, this->splitter);
    this->splitter->addWidget(this->sessionPanel);
    this->splitter->addWidget(this->chatPanel);
    this->splitter->addWidget(this->inspector);
    this->splitter->setStretchFactor(0, 0);
    this->splitter->setStretchFactor(1, 1);
    this->splitter->setStretchFactor(2, 0);
    this->splitter->setSizes({ 260, 700, 280 });
    root->addWidget(this->splitter, 1);

    this->toast = new Toast(this);
    new FramelessHelper(this);

    connect(this->titleBar, &TitleBar::settingsClicked, this, [this] { openSettings(); });
    connect(this->sessionPanel, &SessionPanel::doctorRequested, this, &MainWindow::openDoctor);
    connect(this->sessionPanel, &SessionPanel::refreshCliRequested, this, [this] {
        this->app->hub()->refreshBins();
        showToast(Text("toast.cliRescanned"));
    });
    connect(this->sessionPanel, &SessionPanel::exportRequested, this, &MainWindow::exportSession);
    connect(this->chatPanel, &ChatPanel::exportRequested, this, &MainWindow::exportSession);
    connect(this->chatPanel, &ChatPanel::inspectorToggled, this, &MainWindow::setInspectorVisible);
    connect(this->chatPanel, &ChatPanel::agentSettingsRequested, this, &MainWindow::openSettings);
    connect(this->chatPanel, &ChatPanel::toast, this, &MainWindow::showToast);

    restoreUiState();
}

void MainWindow::showToast(const QString& text)
{
    this->toast->showMessage(text);
}

void MainWindow::openSettings(const QString& agentId)
{
    SettingsDialog dlg(this->app->config(), this->app->hub(), this);
    if (!agentId.isEmpty())
        dlg.focusAgent(agentId);
    if (dlg.exec() == QDialog::Accepted) {
        const bool languageChanged = dlg.editedConfig().language != this->app->config().language;
        this->app->updateConfig(dlg.editedConfig());
        // 문자열은 위젯을 만들 때 박히므로 언어는 다시 시작해야 바뀐다.
        showToast(languageChanged ? Text("toast.restartForLanguage") : Text("toast.settingsSaved"));
    }
}

void MainWindow::openDoctor()
{
    DoctorDialog dlg(this->app->hub(), this);
    dlg.exec();
}

void MainWindow::exportSession(ChatSession* session)
{
    if (!session)
        return;
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString file = QFileDialog::getSaveFileName(this, Text("common.exportMarkdown"),
                                                      QDir(dir).filePath(ChatExporter::suggestedFileName(*session)),
                                                      QStringLiteral("Markdown (*.md)"));
    if (file.isEmpty())
        return;
    QString err;
    if (ChatExporter::exportMarkdown(*session, this->app->config().userName, file, &err))
        showToast(Text("toast.exported").arg(QDir::toNativeSeparators(file)));
    else
        showToast(Text("toast.exportFailed").arg(err));
}

void MainWindow::setInspectorVisible(bool visible)
{
    this->inspector->setVisible(visible);
    this->chatPanel->setInspectorVisible(visible);
}

void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::WindowStateChange) {
        // 최대화하면 바깥 테두리를 없앤다.
        const int m = isMaximized() || isFullScreen() ? 0 : 1;
        layout()->setContentsMargins(m, m, m, m);
    }
    QWidget::changeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    saveUiState();
    QSystemTrayIcon* tray = this->app->tray();
    if (this->app->config().closeToTray && tray && tray->isVisible()) {
        hide();
        if (!this->trayHintShown) {
            tray->showMessage(QStringLiteral("AITalk"), Text("tray.stillRunning"));
            this->trayHintShown = true;
        }
        event->ignore();
        return;
    }
    event->accept();
    this->app->quit();
}

void MainWindow::restoreUiState()
{
    QSettings s(AppPaths::uiSettingsFile(), QSettings::IniFormat);
    if (const QByteArray g = s.value(QStringLiteral("window/geometry")).toByteArray(); !g.isEmpty())
        restoreGeometry(g);
    if (const QByteArray sp = s.value(QStringLiteral("window/splitter")).toByteArray(); !sp.isEmpty())
        this->splitter->restoreState(sp);
    setInspectorVisible(s.value(QStringLiteral("window/inspector"), true).toBool());
}

void MainWindow::saveUiState() const
{
    QSettings s(AppPaths::uiSettingsFile(), QSettings::IniFormat);
    s.setValue(QStringLiteral("window/geometry"), saveGeometry());
    s.setValue(QStringLiteral("window/splitter"), this->splitter->saveState());
    s.setValue(QStringLiteral("window/inspector"), this->inspector->isVisible());
}
