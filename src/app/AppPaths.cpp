#include "app/AppPaths.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

namespace {
QString ensure(const QString& path)
{
    QDir().mkpath(path);
    return path;
}

// 설정 파일은 실행 파일 옆 config/ 에 둔다 (빌드할 때 프로젝트의 config/ 가 복사된다).
// 예전 버전은 데이터 폴더(<root>/config/ 또는 <root>/)에 두었으므로, 남아 있으면 한 번 가져온다.
// 가져온 원본은 지우지 않고 ".migrated" 를 붙여 백업으로 남긴다. 가져오지 못하면 원본을 그대로 쓴다.
QString ConfigFilePath(const QString &name)
{
    const QString newPath =
        QDir(ensure(QCoreApplication::applicationDirPath() + QStringLiteral("/config"))).filePath(name);
    const QDir root(AppPaths::dataRoot());
    for (const QString &oldPath : {root.filePath(QStringLiteral("config/") + name), root.filePath(name)})
    {
        if (!QFile::exists(oldPath))
        {
            continue;
        }
        QFile::remove(newPath); // 빌드가 복사해 둔 초기 설정보다 기존 사용자 설정이 우선이다.
        if (!QFile::copy(oldPath, newPath))
        {
            qWarning() << "failed to import" << oldPath << "to" << newPath;
            return oldPath;
        }
        if (!QFile::rename(oldPath, oldPath + QStringLiteral(".migrated")))
        {
            qWarning() << "failed to mark" << oldPath << "as migrated";
        }
        break;
    }
    return newPath;
}
} // namespace

namespace AppPaths
{

QString dataRoot()
{
    static const QString root = []
    {
        const QString env = qEnvironmentVariable("AITALK_HOME");
        if (!env.isEmpty())
            return ensure(QDir::cleanPath(env));
        return ensure(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    }();
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
