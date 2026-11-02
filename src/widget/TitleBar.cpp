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
    , window(window)
{
    setObjectName(QStringLiteral("titleBar"));
    setAttribute(Qt::WA_StyledBackground);
    setFixedHeight(38);

    auto* lay = new QHBoxLayout(this);
    lay->setContentsMargins(12, 0, 0, 0);
    lay->setSpacing(8);

    auto* logo = new QLabel(this);
    logo->setPixmap(QIcon(QStringLiteral(":/icons/app.svg")).pixmap(16, 16));
    this->title = new QLabel(QStringLiteral("AITalk"), this);
    this->title->setObjectName(QStringLiteral("titleBarTitle"));
    lay->addWidget(logo);
    lay->addWidget(this->title);
    lay->addStretch(1);

    auto* settings = makeButton(QStringLiteral("settings"), Text("title.settings"), QStringLiteral("titleBarButton"), this);
    settings->setIconSize(QSize(15, 15));
    auto* min = makeButton(QStringLiteral("minimize"), Text("title.minimize"), QStringLiteral("titleBarButton"), this);
    this->maximizeButton = makeButton(QStringLiteral("maximize"), Text("title.maximize"), QStringLiteral("titleBarButton"), this);
    auto* close = makeButton(QStringLiteral("close"), Text("title.close"), QStringLiteral("titleBarClose"), this);
    lay->addWidget(settings);
    lay->addWidget(min);
    lay->addWidget(this->maximizeButton);
    lay->addWidget(close);

    connect(settings, &QToolButton::clicked, this, &TitleBar::settingsClicked);
    connect(min, &QToolButton::clicked, this->window, &QWidget::showMinimized);
    connect(this->maximizeButton, &QToolButton::clicked, this, &TitleBar::toggleMaximize);
    connect(close, &QToolButton::clicked, this->window, &QWidget::close);

    this->window->installEventFilter(this);
}

void TitleBar::setTitle(const QString& title)
{
    this->title->setText(title);
}

void TitleBar::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && this->window->windowHandle()) {
        this->window->windowHandle()->startSystemMove();
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
    if (watched == this->window && event->type() == QEvent::WindowStateChange)
        updateMaximizeIcon();
    return QWidget::eventFilter(watched, event);
}

void TitleBar::toggleMaximize()
{
    this->window->isMaximized() ? this->window->showNormal() : this->window->showMaximized();
}

void TitleBar::updateMaximizeIcon()
{
    const bool max = this->window->isMaximized();
    this->maximizeButton->setIconName(max ? QStringLiteral("restore") : QStringLiteral("maximize"));
    this->maximizeButton->setToolTip(max ? Text("title.restore") : Text("title.maximize"));
}
