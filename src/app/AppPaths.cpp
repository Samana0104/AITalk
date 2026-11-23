#include "app/AppPaths.h"

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

// 예전 버전은 설정 파일을 루트에 두었다. 루트에만 있으면 config/ 로 옮기고,
// 옮기지 못하면 기존 파일을 그대로 써서 설정을 잃지 않게 한다.
QString ConfigFilePath(const QString& name)
{
    const QDir root(AppPaths::dataRoot());
    const QString newPath = QDir(ensure(root.filePath(QStringLiteral("config")))).filePath(name);
    const QString oldPath = root.filePath(name);
    if(QFile::exists(oldPath) && !QFile::exists(newPath) && !QFile::rename(oldPath, newPath))
    {
        qWarning() << "failed to move" << oldPath << "to" << newPath;
        return oldPath;
    }
    return newPath;
}
}

namespace AppPaths {

QString dataRoot()
{
    static const QString root = [] {
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
QString workDir(const QString& memberId) { return ensure(QDir(dataRoot()).filePath(QStringLiteral("cwd/") + memberId)); }
QString tempDir(const QString& name) { return ensure(QDir(dataRoot()).filePath(QStringLiteral("tmp/") + name)); }

}
