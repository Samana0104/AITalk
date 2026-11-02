#include "core/AppConfig.h"

#include <QFile>
#include <QJsonDocument>
#include <QSaveFile>

QString AgentConfig::effectiveModel(bool boost) const
{
    return boost && !boostModel.isEmpty() ? boostModel : model;
}

QString AgentConfig::effectiveEffort(bool boost) const
{
    return boost && !boostEffort.isEmpty() ? boostEffort : effort;
}

QString AgentConfig::label(bool boost) const
{
    const QString m = effectiveModel(boost);
    const QString e = effectiveEffort(boost);
    if (m.isEmpty())
        return {};
    return e.isEmpty() ? m : QStringLiteral("%1 (effort %2)").arg(m, e);
}

AppConfig AppConfig::defaults()
{
    AppConfig c;
    // 모델 이름은 각자 계정·CLI 에서 되는 이름으로 바꿔 쓰면 된다 (설정 → 에이전트).
    c.agents.insert(QStringLiteral("claude"), { QStringLiteral("sonnet"), {}, QStringLiteral("opus"), {}, 4 });
    c.agents.insert(QStringLiteral("gpt"), { QStringLiteral("gpt-6-sol"), QStringLiteral("low"), QStringLiteral("gpt-6-astra"), QStringLiteral("medium"), 4 });
    c.agents.insert(QStringLiteral("gemini"), { QStringLiteral("gemini-3.8-flash-medium"), {}, QStringLiteral("gemini-3.8-flash-high"), {}, 4 });
    for (const char* key : { "claude", "codex", "agy" })
        c.bins.insert(QString::fromLatin1(key), QString());
    return c;
}

AppConfig AppConfig::load(const QString& file)
{
    QFile f(file);
    if (!f.open(QIODevice::ReadOnly))
        return defaults();
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    return doc.isObject() ? fromJson(doc.object()) : defaults();
}

bool AppConfig::save(const QString& file) const
{
    QSaveFile f(file);
    if (!f.open(QIODevice::WriteOnly))
        return false;
    f.write(QJsonDocument(toJson()).toJson(QJsonDocument::Indented));
    return f.commit();
}

QJsonObject AppConfig::toJson() const
{
    QJsonObject agentsObj;
    for (auto it = agents.cbegin(); it != agents.cend(); ++it) {
        const AgentConfig& a = it.value();
        agentsObj.insert(it.key(), QJsonObject {
            { "model", a.model },
            { "effort", a.effort },
            { "boost", QJsonObject { { "model", a.boostModel }, { "effort", a.boostEffort } } },
            { "rotate", a.rotate },
        });
    }
    QJsonObject binsObj;
    for (auto it = bins.cbegin(); it != bins.cend(); ++it)
        binsObj.insert(it.key(), it.value());

    return QJsonObject {
        { "userName", userName },
        { "defaultRoomName", defaultRoomName },
        { "speed", speed },
        { "autoSleepMinutes", autoSleepMinutes },
        { "maxInFlight", maxInFlight },
        { "historyForPrompt", historyForPrompt },
        { "turnTimeoutSec", turnTimeoutSec },
        { "boost", QJsonObject {
              { "mode", boostMode },
              { "timeoutSec", boostTimeoutSec },
              { "selfCooldownSec", boostSelfCooldownSec },
              { "aiRequestCooldownSec", boostAiRequestCooldownSec } } },
        { "spark", QJsonObject {
              { "enabled", sparkEnabled },
              { "backoff", sparkBackoff },
              { "maxWaitSec", sparkMaxWaitSec } } },
        { "bins", binsObj },
        { "agents", agentsObj },
        { "ui", QJsonObject {
              { "language", language },
              { "theme", theme },
              { "fontSize", fontSize },
              { "closeToTray", closeToTray } } },
    };
}

AppConfig AppConfig::fromJson(const QJsonObject& o)
{
    AppConfig c = defaults();
    auto str = [](const QJsonObject& obj, const char* key, QString& out) {
        if (obj.value(QLatin1String(key)).isString())
            out = obj.value(QLatin1String(key)).toString();
    };
    auto num = [](const QJsonObject& obj, const char* key, int& out) {
        if (obj.value(QLatin1String(key)).isDouble())
            out = obj.value(QLatin1String(key)).toInt();
    };

    str(o, "userName", c.userName);
    str(o, "defaultRoomName", c.defaultRoomName);
    str(o, "speed", c.speed);
    num(o, "autoSleepMinutes", c.autoSleepMinutes);
    num(o, "maxInFlight", c.maxInFlight);
    num(o, "historyForPrompt", c.historyForPrompt);
    num(o, "turnTimeoutSec", c.turnTimeoutSec);

    const QJsonObject boost = o.value(QLatin1String("boost")).toObject();
    str(boost, "mode", c.boostMode);
    num(boost, "timeoutSec", c.boostTimeoutSec);
    num(boost, "selfCooldownSec", c.boostSelfCooldownSec);
    num(boost, "aiRequestCooldownSec", c.boostAiRequestCooldownSec);

    const QJsonObject spark = o.value(QLatin1String("spark")).toObject();
    if (spark.contains(QLatin1String("enabled")))
        c.sparkEnabled = spark.value(QLatin1String("enabled")).toBool(true);
    if (spark.value(QLatin1String("backoff")).isDouble())
        c.sparkBackoff = spark.value(QLatin1String("backoff")).toDouble();
    num(spark, "maxWaitSec", c.sparkMaxWaitSec);

    // 예전 config.json 에 남아 있을 수 있는 없는 멤버·CLI(예: grok)는 무시한다.
    const QJsonObject bins = o.value(QLatin1String("bins")).toObject();
    for (auto it = bins.begin(); it != bins.end(); ++it)
        if (c.bins.contains(it.key()))
            c.bins.insert(it.key(), it.value().toString());

    const QJsonObject agents = o.value(QLatin1String("agents")).toObject();
    for (auto it = agents.begin(); it != agents.end(); ++it) {
        if (!c.agents.contains(it.key()))
            continue;
        const QJsonObject a = it.value().toObject();
        AgentConfig ac = c.agents.value(it.key());
        str(a, "model", ac.model);
        str(a, "effort", ac.effort);
        num(a, "rotate", ac.rotate);
        if (a.value(QLatin1String("boost")).isNull()) {
            ac.boostModel.clear();
            ac.boostEffort.clear();
        } else {
            const QJsonObject b = a.value(QLatin1String("boost")).toObject();
            str(b, "model", ac.boostModel);
            str(b, "effort", ac.boostEffort);
        }
        c.agents.insert(it.key(), ac);
    }

    const QJsonObject ui = o.value(QLatin1String("ui")).toObject();
    str(ui, "language", c.language);
    str(ui, "theme", c.theme);
    num(ui, "fontSize", c.fontSize);
    if (ui.contains(QLatin1String("closeToTray")))
        c.closeToTray = ui.value(QLatin1String("closeToTray")).toBool();
    return c;
}
