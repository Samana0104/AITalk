#include "app/AppPaths.h"

#include <QDir>
#include <QStandardPaths>

namespace {
QString ensure(const QString& path)
{
    QDir().mkpath(path);
    return path;
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

QString configFile() { return QDir(dataRoot()).filePath(QStringLiteral("config.json")); }
QString uiSettingsFile() { return QDir(dataRoot()).filePath(QStringLiteral("ui.ini")); }
QString sessionsDir() { return ensure(QDir(dataRoot()).filePath(QStringLiteral("sessions"))); }
QString trashDir() { return ensure(QDir(dataRoot()).filePath(QStringLiteral("trash"))); }
QString workDir(const QString& memberId) { return ensure(QDir(dataRoot()).filePath(QStringLiteral("cwd/") + memberId)); }
QString tempDir(const QString& name) { return ensure(QDir(dataRoot()).filePath(QStringLiteral("tmp/") + name)); }

}
