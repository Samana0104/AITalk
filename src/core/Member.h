#pragma once

#include <QColor>
#include <QList>
#include <QString>
#include <QStringList>

namespace AITalk
{

// 방 멤버(AI)의 고정 정보. 성격이나 말투는 일부러 넣지 않는다:
// 대화하면서 생기도록 둔다 (engine/PromptBuilder 참고).
struct MemberInfo {
    QString id;           // 고정 키: claude, gpt, gemini (res/avatars, res/emoticons 폴더 이름과 같다)
    QString name;         // 화면 이름
    QString maker;
    QString cli;          // agent/CliLocator 키: claude, codex, agy
    QColor color;         // 이름 글자색
    QColor avatarBg;      // SD 프사 뒤 파스텔 배경
    QStringList aliases;  // 이 멤버를 부르는 표기(소문자)
    bool finalConsonant;  // 한국어로 읽었을 때 받침이 있는지 (조사 이/가 고르기)
};

namespace Members {

inline const QString User = QStringLiteral("user");
inline const QString System = QStringLiteral("system");

const QList<MemberInfo>& all();
QStringList ids();
const MemberInfo* find(const QString& id);
bool isAgent(const QString& id);

QString displayName(const QString& id, const QString& userName);
QColor colorOf(const QString& id);

// @이름 또는 그냥 이름·별명이 들어 있으면 true
bool mentions(const QString& text, const QString& id);
// @이름 으로 콕 집어 부른 경우만 true
bool atMentions(const QString& text, const QString& id);

// withJosa("user", "민수", "이/가") -> "민수가"
QString withJosa(const QString& id, const QString& userName, const QString& pair);

}

} // namespace AITalk
