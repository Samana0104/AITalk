#include "agent/AgentHub.h"

#include "agent/AgyBackend.h"
#include "agent/ClaudeBackend.h"
#include "agent/CliLocator.h"
#include "agent/CodexBackend.h"
#include "app/AppPaths.h"
#include "core/AppConfig.h"
#include "core/Member.h"

AgentHub::AgentHub(QObject* parent)
    : QObject(parent)
{
    m_backends.emplace(QStringLiteral("claude"), std::make_unique<ClaudeBackend>());
    m_backends.emplace(QStringLiteral("gpt"), std::make_unique<CodexBackend>());
    m_backends.emplace(QStringLiteral("gemini"), std::make_unique<AgyBackend>());
}

AgentHub::~AgentHub() = default;

void AgentHub::setConfig(const AppConfig* config)
{
    m_cfg = config;
    if (auto* agy = dynamic_cast<AgyBackend*>(backend(QStringLiteral("gemini"))))
        agy->setRotate(m_cfg->agents.value(QStringLiteral("gemini")).rotate);
    refreshBins();
}

void AgentHub::refreshBins()
{
    m_bins = CliLocator::resolveAll(m_cfg ? m_cfg->bins : QMap<QString, QString>());
    emit binsChanged();
}

AgentBackend* AgentHub::backend(const QString& memberId) const
{
    const auto it = m_backends.find(memberId);
    return it == m_backends.end() ? nullptr : it->second.get();
}

QString AgentHub::binOf(const QString& memberId) const
{
    const MemberInfo* m = Members::find(memberId);
    return m ? m_bins.value(m->cli) : QString();
}

bool AgentHub::isAvailable(const QString& memberId) const
{
    return backend(memberId) && !binOf(memberId).isEmpty();
}

int AgentHub::maxPromptChars(const QString& memberId) const
{
    const AgentBackend* b = backend(memberId);
    return b ? b->maxPromptChars() : 120000;
}

AgentCall* AgentHub::chat(const QString& memberId, const QString& brief, const QString& turn, bool boost, QObject* parent)
{
    AgentBackend* b = backend(memberId);
    if (!b || !m_cfg)
        return nullptr;
    const AgentConfig a = m_cfg->agents.value(memberId);

    AgentRequest req;
    req.memberId = memberId;
    req.brief = brief;
    req.turn = turn;
    req.boost = boost && a.hasBoost();
    req.model = a.effectiveModel(req.boost);
    req.effort = a.effectiveEffort(req.boost);
    req.timeoutMs = (req.boost ? m_cfg->boostTimeoutSec : m_cfg->turnTimeoutSec) * 1000;
    req.workDir = AppPaths::workDir(memberId);
    return b->start(binOf(memberId), req, parent);
}
