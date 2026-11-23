#include "agent/CliLocator.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace AITalk
{

namespace {

#ifdef Q_OS_WIN
constexpr bool isWindows = true;
#else
constexpr bool isWindows = false;
#endif

QString exe(const QString& name) { return isWindows ? name + QStringLiteral(".exe") : name; }

bool isFile(const QString& path)
{
    if (path.isEmpty())
        return false;
    const QFileInfo fi(path);
    return fi.exists() && fi.isFile();
}

QString firstExisting(const QStringList& candidates)
{
    for (const QString& c : candidates)
        if (isFile(c))
            return QDir::cleanPath(c);
    return {};
}

QString onPath(const QString& name) { return QStandardPaths::findExecutable(name); }

// <root>/<버전 폴더>/<rest>: 가장 최근에 바뀐 것
QString newestUnder(const QString& root, const QString& rest)
{
    const QDir d(root);
    if (root.isEmpty() || !d.exists())
        return {};
    QString best;
    QDateTime bestTime;
    for (const QString& sub : d.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        const QFileInfo fi(d.filePath(sub + QLatin1Char('/') + rest));
        if (fi.isFile() && (best.isEmpty() || fi.lastModified() > bestTime)) {
            best = fi.filePath();
            bestTime = fi.lastModified();
        }
    }
    return best;
}

QString home() { return QDir::homePath(); }
QString appData() { return qEnvironmentVariable("APPDATA", home() + QStringLiteral("/AppData/Roaming")); }
QString localAppData() { return qEnvironmentVariable("LOCALAPPDATA", home() + QStringLiteral("/AppData/Local")); }
QString npmRoot() { return isWindows ? appData() + QStringLiteral("/npm/node_modules") : QString(); }

}

namespace CliLocator {

QStringList keys()
{
    return { QStringLiteral("claude"), QStringLiteral("codex"), QStringLiteral("agy") };
}

QString resolve(const QString& key, const QMap<QString, QString>& overrides)
{
    const QString manual = overrides.value(key).trimmed();
    if (!manual.isEmpty())
        return isFile(manual) ? manual : QString();

    if (key == QLatin1String("claude")) {
        return firstExisting({
            onPath(exe(QStringLiteral("claude"))),
            home() + QStringLiteral("/.local/bin/") + exe(QStringLiteral("claude")),
            isWindows ? npmRoot() + QStringLiteral("/@anthropic-ai/claude-code/bin/claude.exe") : QString(),
        });
    }
    if (key == QLatin1String("codex")) {
        return firstExisting({
            onPath(exe(QStringLiteral("codex"))),
            isWindows ? localAppData() + QStringLiteral("/Programs/OpenAI/Codex/bin/codex.exe")
                 : home() + QStringLiteral("/.local/bin/codex"),
            isWindows ? newestUnder(localAppData() + QStringLiteral("/OpenAI/Codex/bin"), QStringLiteral("codex.exe")) : QString(),
            isWindows ? newestUnder(npmRoot() + QStringLiteral("/@openai/codex/vendor"), QStringLiteral("codex/codex.exe")) : QString(),
        });
    }
    if (key == QLatin1String("agy")) {
        return firstExisting({
            onPath(exe(QStringLiteral("agy"))),
            isWindows ? localAppData() + QStringLiteral("/agy/bin/agy.exe") : home() + QStringLiteral("/.local/bin/agy"),
        });
    }
    return {};
}

QMap<QString, QString> resolveAll(const QMap<QString, QString>& overrides)
{
    QMap<QString, QString> out;
    for (const QString& k : keys())
        out.insert(k, resolve(k, overrides));
    return out;
}

QString installHint(const QString& key)
{
    if (isWindows) {
        if (key == QLatin1String("claude")) return QStringLiteral("irm https://claude.ai/install.ps1 | iex");
        if (key == QLatin1String("codex")) return QStringLiteral("irm https://chatgpt.com/codex/install.ps1 | iex");
        if (key == QLatin1String("agy")) return QStringLiteral("irm https://antigravity.google/cli/install.ps1 | iex");
    } else {
        if (key == QLatin1String("claude")) return QStringLiteral("curl -fsSL https://claude.ai/install.sh | bash");
        if (key == QLatin1String("codex")) return QStringLiteral("curl -fsSL https://chatgpt.com/codex/install.sh | sh");
        if (key == QLatin1String("agy")) return QStringLiteral("curl -fsSL https://antigravity.google/cli/install.sh | bash");
    }
    return {};
}

}

} // namespace AITalk
