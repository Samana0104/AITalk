#include "widget/TitleBar.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QWindow>

#include "component/IconButton.h"
#include "style/ThemeManager.h"
#include "util/DataManager.h"

namespace {
IconButton* makeButton(const QString& icon, const QString& tip, const QString& objectName, QWidget* parent)
{
    auto* b = new IconButton(icon, tip, parent);
    b->setObjectName(objectName);
    b->setFixedSize(46, 38);
    b->setIconSize(QSize(10, 10));
    b->setFocusPolicy(Qt::NoFocus);
    return b;
}
}

TitleBar::TitleBar(QWidget* window)
    : QWidget(window)
    , m_window(window)
{
    setObjectName(QStringLiteral("titleBar"));
    setAttribute(Qt::WA_StyledBackground);
    setFixedHeight(38);

    auto* lay = new QHBoxLayout(this);
    lay->setContentsMargins(12, 0, 0, 0);
    lay->setSpacing(8);

    auto* logo = new QLabel(this);
    logo->setPixmap(QIcon(QStringLiteral(":/icons/app.svg")).pixmap(16, 16));
    m_title = new QLabel(QStringLiteral("AITalk"), this);
    m_title->setObjectName(QStringLiteral("titleBarTitle"));
    lay->addWidget(logo);
    lay->addWidget(m_title);
    lay->addStretch(1);

    auto* settings = makeButton(QStringLiteral("settings"), Text("title.settings"), QStringLiteral("titleBarButton"), this);
    settings->setIconSize(QSize(15, 15));
    auto* min = makeButton(QStringLiteral("minimize"), Text("title.minimize"), QStringLiteral("titleBarButton"), this);
    m_max = makeButton(QStringLiteral("maximize"), Text("title.maximize"), QStringLiteral("titleBarButton"), this);
    auto* close = makeButton(QStringLiteral("close"), Text("title.close"), QStringLiteral("titleBarClose"), this);
    lay->addWidget(settings);
    lay->addWidget(min);
    lay->addWidget(m_max);
    lay->addWidget(close);

    connect(settings, &QToolButton::clicked, this, &TitleBar::settingsClicked);
    connect(min, &QToolButton::clicked, m_window, &QWidget::showMinimized);
    connect(m_max, &QToolButton::clicked, this, &TitleBar::toggleMaximize);
    connect(close, &QToolButton::clicked, m_window, &QWidget::close);

    m_window->installEventFilter(this);
}

void TitleBar::setTitle(const QString& title)
{
    m_title->setText(title);
}

void TitleBar::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_window->windowHandle()) {
        m_window->windowHandle()->startSystemMove();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
        toggleMaximize();
}

bool TitleBar::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_window && event->type() == QEvent::WindowStateChange)
        updateMaximizeIcon();
    return QWidget::eventFilter(watched, event);
}

void TitleBar::toggleMaximize()
{
    m_window->isMaximized() ? m_window->showNormal() : m_window->showMaximized();
}

void TitleBar::updateMaximizeIcon()
{
    const bool max = m_window->isMaximized();
    m_max->setIconName(max ? QStringLiteral("restore") : QStringLiteral("maximize"));
    m_max->setToolTip(max ? Text("title.restore") : Text("title.maximize"));
}
