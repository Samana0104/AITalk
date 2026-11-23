#include <QApplication>
#include <QIcon>

#include "app/Application.h"

int main(int argc, char* argv[])
{
    QApplication qapp(argc, argv);
    QApplication::setApplicationName(QStringLiteral("AITalk"));
    QApplication::setApplicationDisplayName(QStringLiteral("AITalk"));
    QApplication::setApplicationVersion(QStringLiteral(AITALK_VERSION));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/app.svg")));
    // 창을 닫아도 트레이에 남을 수 있으니 종료는 Application 이 직접 정한다.
    QApplication::setQuitOnLastWindowClosed(false);

    AITalk::Application app;
    if(!app.initialize())
    {
        return 1;
    }
    return qapp.exec();
}
