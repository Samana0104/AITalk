#include "app/AppPaths.h"

#include <QCoreApplication>
#include <QDir>

namespace AITalk
{

namespace {
QString ensure(const QString& path)
{
    QDir().mkpath(path);
    return path;
}

// 설정 파일은 실행 파일 옆 config/ 에 둔다 (빌드할 때 프로젝트의 config/ 가 복사된다).
QString ConfigFilePath(const QString& name)
{
    return QDir(ensure(QCoreApplication::applicationDirPath() + QStringLiteral("/config"))).filePath(name);
}
} // namespace

namespace AppPaths
{

QString dataRoot()
{
    // 포터블 방식: 채팅 기록도 실행 파일 옆에 두어 폴더만 지우면 앱이 깨끗이 사라지게 한다.
    static const QString root = ensure(QCoreApplication::applicationDirPath() + QStringLiteral("/data"));
    return root;
}

QString configFile() { return ConfigFilePath(QStringLiteral("config.json")); }
QString uiSettingsFile() { return ConfigFilePath(QStringLiteral("ui.ini")); }
QString sessionsDir() { return ensure(QDir(dataRoot()).filePath(QStringLiteral("sessions"))); }
QString trashDir() { return ensure(QDir(dataRoot()).filePath(QStringLiteral("trash"))); }
QString workDir(const QString &memberId)
{
    return ensure(QDir(dataRoot()).filePath(QStringLiteral("cwd/") + memberId));
}
QString tempDir(const QString &name) { return ensure(QDir(dataRoot()).filePath(QStringLiteral("tmp/") + name)); }

} // namespace AppPaths

} // namespace AITalk
