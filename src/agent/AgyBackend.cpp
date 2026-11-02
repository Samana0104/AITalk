#include "agent/AgyBackend.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "app/AppPaths.h"

AgentCall* AgyBackend::start(const QString& bin, const AgentRequest& req, QObject* parent)
{
    // 진심모드 턴은 다른 모델이라 대화를 따로 쓴다.
    const bool reuse = !req.boost;
    if (reuse && !m_conversationId.isEmpty() && m_turns >= m_rotate) {
        m_conversationId.clear();
        m_turns = 0;
    }

    const int timeoutSec = req.timeoutMs / 1000;
    QStringList args {
        QStringLiteral("--model"), req.model,
        QStringLiteral("--output-format"), QStringLiteral("json"),
        QStringLiteral("--print-timeout"), QStringLiteral("%1s").arg(timeoutSec),
    };
    if (reuse && !m_conversationId.isEmpty())
        args << QStringLiteral("--conversation") << m_conversationId;
    args << QStringLiteral("-p") << req.brief + QStringLiteral("\n\n=====\n\n") + req.turn;

    // agy 는 임시 폴더를 읽을 수 있으므로 방 전용 빈 폴더로 돌려 둔다.
    const QString tmp = AppPaths::tempDir(QStringLiteral("agy"));
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("TEMP"), tmp);
    env.insert(QStringLiteral("TMP"), tmp);
    env.insert(QStringLiteral("TMPDIR"), tmp);

    auto* call = new ProcessAgentCall([this, reuse](const CliResult& r) {
        QString text;
        CliResult res = r;
        const QJsonDocument doc = QJsonDocument::fromJson(r.out);
        if (doc.isObject()) {
            const QJsonObject j = doc.object();
            text = j.value(QLatin1String("response")).toString();
            const QString status = j.value(QLatin1String("status")).toString();
            // 도구를 쓰려다 막히고 아무 말도 안 했으면 실패가 아니라 pass 로 본다.
            if (text.trimmed().isEmpty() && status == QLatin1String("SUCCESS")
                && !j.value(QLatin1String("denied_actions")).toArray().isEmpty())
                text = QStringLiteral("{\"action\":\"pass\"}");
            const QString cid = j.value(QLatin1String("conversation_id")).toString();
            if (reuse && !cid.isEmpty()) {
                if (cid != m_conversationId) {
                    m_conversationId = cid;
                    m_turns = 0;
                }
                ++m_turns;
            }
            if (!status.isEmpty() && status != QLatin1String("SUCCESS"))
                res.err += "\nstatus=" + status.toUtf8();
        } else {
            text = QString::fromUtf8(r.out);
            if (reuse) {
                m_conversationId.clear();
                m_turns = 0;
            }
        }
        return AgentReply::fromProcess(res, text);
    }, parent);
    call->run(bin, args, QByteArray(), req.workDir, req.timeoutMs + 10000, env);
    return call;
}
