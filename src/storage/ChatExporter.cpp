#include "storage/ChatExporter.h"

#include <QDateTime>
#include <QRegularExpression>
#include <QSaveFile>

#include "core/Member.h"
#include "storage/ChatSession.h"
#include "manager/DataManager.h"

namespace AITalk
{

namespace ChatExporter {

QString suggestedFileName(const ChatSession& session)
{
    static const QRegularExpression bad(QStringLiteral(R"([\\/:*?"<>|]+)"));
    QString name = session.title();
    name.replace(bad, QStringLiteral("_"));
    return QStringLiteral("%1-%2.md").arg(name, QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmm")));
}

bool exportMarkdown(const ChatSession& session, const QString& userName, const QString& filePath, QString* error)
{
    QString out;
    out += QStringLiteral("# %1\n\n").arg(session.title());
    out += Text("export.exportedAt", QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm"))) + QLatin1Char('\n');
    out += Text("export.messageCount", session.messages().size()) + QStringLiteral("\n\n---\n\n");

    for (const ChatMessage& m : session.messages()) {
        const QString when = QDateTime::fromMSecsSinceEpoch(m.ts).toString(QStringLiteral("yyyy-MM-dd HH:mm"));
        if (m.isSystem()) {
            out += QStringLiteral("_%1 · %2_\n\n").arg(m.text, when);
            continue;
        }
        out += QStringLiteral("**%1** · %2").arg(Members::displayName(m.from, userName), when);
        if (!m.model.isEmpty())
            out += QStringLiteral(" · `%1`").arg(m.model);
        out += QStringLiteral("\n\n");
        if (const ChatMessage* r = session.byId(m.replyTo))
            out += QStringLiteral("> ↪ %1: %2\n\n").arg(Members::displayName(r->from, userName), r->displayText().left(80).simplified());
        out += m.displayText() + QStringLiteral("\n\n");
        if (!m.reactions.isEmpty()) {
            QStringList parts;
            for (auto it = m.reactions.cbegin(); it != m.reactions.cend(); ++it)
                parts << QStringLiteral("%1 %2").arg(it.key()).arg(it.value().size());
            out += Text("export.reactions", parts.join(QStringLiteral(" · "))) + QStringLiteral("\n\n");
        }
    }

    QSaveFile f(filePath);
    if (!f.open(QIODevice::WriteOnly)) {
        if (error)
            *error = f.errorString();
        return false;
    }
    f.write(out.toUtf8());
    if (!f.commit()) {
        if (error)
            *error = f.errorString();
        return false;
    }
    return true;
}

}

} // namespace AITalk
