#pragma once

#include <QMap>
#include <QObject>
#include <map>
#include <memory>

class AgentBackend;
class AgentCall;
struct AppConfig;

// 멤버 id(claude, gpt, gemini) -> CLI 백엔드 연결, CLI 경로 탐지, 호출 시작.
class AgentHub : public QObject {
    Q_OBJECT
public:
    explicit AgentHub(QObject* parent = nullptr);
    ~AgentHub() override;

    void setConfig(const AppConfig* config); // CLI 경로를 다시 찾는다
    void refreshBins();

    QString binOf(const QString& memberId) const;
    bool isAvailable(const QString& memberId) const;
    int maxPromptChars(const QString& memberId) const;

    // 한 턴 호출. 반환된 호출 객체의 finished 를 받으면 된다.
    AgentCall* chat(const QString& memberId, const QString& brief, const QString& turn, bool boost, QObject* parent);

signals:
    void binsChanged();

private:
    AgentBackend* backend(const QString& memberId) const;

    const AppConfig* m_cfg = nullptr;
    std::map<QString, std::unique_ptr<AgentBackend>> m_backends;
    QMap<QString, QString> m_bins; // CLI 키 -> 경로 (없으면 빈 문자열)
};
