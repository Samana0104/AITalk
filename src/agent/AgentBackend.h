#pragma once

#include <QObject>
#include <QString>
#include <functional>

#include "agent/CliProcess.h"

namespace AITalk
{

// 한 턴 호출 입력. model/effort 는 AgentHub 가 진심모드 여부까지 반영해서 채운다.
struct AgentRequest {
    QString memberId;
    QString brief;      // 시스템 프롬프트 (방 규칙, 응답 형식)
    QString turn;       // 이번 턴 내용 (메모, 대화 기록, 상황)
    QString model;
    QString effort;
    QString workDir;    // CLI 를 실행할 빈 폴더
    bool boost = false;
    int timeoutMs = 150000;
};

struct AgentReply {
    bool ok = false;
    QString text;       // 모델이 돌려준 원문 (JSON 액션이 들어 있어야 함)
    QString detail;     // 실패 이유 (exit code, stderr 끝부분)
    qint64 ms = 0;

    static AgentReply fromProcess(const CliResult& r, const QString& text);
};

// 진행 중인 호출 하나. finished 는 한 번만 나오고, 그 뒤 스스로 deleteLater 된다.
// abort() 하면 finished 없이 끝난다.
class AgentCall : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    virtual void abort() = 0;

signals:
    void finished(const AgentReply& reply);
};

// CLI 프로세스 하나로 끝나는 호출. 결과 해석은 백엔드가 넘긴 parser 가 한다.
class ProcessAgentCall : public AgentCall {
    Q_OBJECT
public:
    using Parser = std::function<AgentReply(const CliResult&)>;

    ProcessAgentCall(Parser parser, QObject* parent = nullptr);
    void run(const QString& program, const QStringList& args, const QByteArray& input,
             const QString& workDir, int timeoutMs,
             const QProcessEnvironment& env = QProcessEnvironment::systemEnvironment());
    void abort() override;

private:
    Parser parser;
    CliProcess* process;
    bool aborted = false;
};

// CLI 별 어댑터. 새 CLI 를 붙이려면 이걸 구현해서 AgentHub 에 등록하면 된다.
class AgentBackend {
public:
    virtual ~AgentBackend() = default;
    virtual QString cliKey() const = 0;
    // 프롬프트 최대 길이 (agy 는 명령줄 인자로 넘기므로 짧다)
    virtual int maxPromptChars() const { return 120000; }
    virtual AgentCall* start(const QString& bin, const AgentRequest& req, QObject* parent) = 0;
};

// 백엔드 공용 도우미
namespace BackendUtil {
bool writeText(const QString& path, const QString& text);
QString uniqueName(const QString& prefix, const QString& suffix);
}

} // namespace AITalk
