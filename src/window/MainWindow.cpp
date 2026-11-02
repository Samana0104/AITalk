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
    , m_app(app)
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

    m_titleBar = new TitleBar(this);
    root->addWidget(m_titleBar);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setHandleWidth(1);
    m_splitter->setChildrenCollapsible(false);
    m_sessionPanel = new SessionPanel(m_app->sessions(), m_splitter);
    m_chatPanel = new ChatPanel(m_app, m_splitter);
    m_inspector = new InspectorPanel(m_app, m_splitter);
    m_splitter->addWidget(m_sessionPanel);
    m_splitter->addWidget(m_chatPanel);
    m_splitter->addWidget(m_inspector);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setStretchFactor(2, 0);
    m_splitter->setSizes({ 260, 700, 280 });
    root->addWidget(m_splitter, 1);

    m_toast = new Toast(this);
    new FramelessHelper(this);

    connect(m_titleBar, &TitleBar::settingsClicked, this, [this] { openSettings(); });
    connect(m_sessionPanel, &SessionPanel::doctorRequested, this, &MainWindow::openDoctor);
    connect(m_sessionPanel, &SessionPanel::refreshCliRequested, this, [this] {
        m_app->hub()->refreshBins();
        showToast(Text("toast.cliRescanned"));
    });
    connect(m_sessionPanel, &SessionPanel::exportRequested, this, &MainWindow::exportSession);
    connect(m_chatPanel, &ChatPanel::exportRequested, this, &MainWindow::exportSession);
    connect(m_chatPanel, &ChatPanel::inspectorToggled, this, &MainWindow::setInspectorVisible);
    connect(m_chatPanel, &ChatPanel::agentSettingsRequested, this, &MainWindow::openSettings);
    connect(m_chatPanel, &ChatPanel::toast, this, &MainWindow::showToast);

    restoreUiState();
}

void MainWindow::showToast(const QString& text)
{
    m_toast->showMessage(text);
}

void MainWindow::openSettings(const QString& agentId)
{
    SettingsDialog dlg(m_app->config(), m_app->hub(), this);
    if (!agentId.isEmpty())
        dlg.focusAgent(agentId);
    if (dlg.exec() == QDialog::Accepted) {
        const bool languageChanged = dlg.editedConfig().language != m_app->config().language;
        m_app->updateConfig(dlg.editedConfig());
        // 문자열은 위젯을 만들 때 박히므로 언어는 다시 시작해야 바뀐다.
        showToast(languageChanged ? Text("toast.restartForLanguage") : Text("toast.settingsSaved"));
    }
}

void MainWindow::openDoctor()
{
    DoctorDialog dlg(m_app->hub(), this);
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
    if (ChatExporter::exportMarkdown(*session, m_app->config().userName, file, &err))
        showToast(Text("toast.exported").arg(QDir::toNativeSeparators(file)));
    else
        showToast(Text("toast.exportFailed").arg(err));
}

void MainWindow::setInspectorVisible(bool visible)
{
    m_inspector->setVisible(visible);
    m_chatPanel->setInspectorVisible(visible);
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
    QSystemTrayIcon* tray = m_app->tray();
    if (m_app->config().closeToTray && tray && tray->isVisible()) {
        hide();
        if (!m_trayHintShown) {
            tray->showMessage(QStringLiteral("AITalk"), Text("tray.stillRunning"));
            m_trayHintShown = true;
        }
        event->ignore();
        return;
    }
    event->accept();
    m_app->quit();
}

void MainWindow::restoreUiState()
{
    QSettings s(AppPaths::uiSettingsFile(), QSettings::IniFormat);
    if (const QByteArray g = s.value(QStringLiteral("window/geometry")).toByteArray(); !g.isEmpty())
        restoreGeometry(g);
    if (const QByteArray sp = s.value(QStringLiteral("window/splitter")).toByteArray(); !sp.isEmpty())
        m_splitter->restoreState(sp);
    setInspectorVisible(s.value(QStringLiteral("window/inspector"), true).toBool());
}

void MainWindow::saveUiState() const
{
    QSettings s(AppPaths::uiSettingsFile(), QSettings::IniFormat);
    s.setValue(QStringLiteral("window/geometry"), saveGeometry());
    s.setValue(QStringLiteral("window/splitter"), m_splitter->saveState());
    s.setValue(QStringLiteral("window/inspector"), m_inspector->isVisible());
}
