#pragma once

#include <QList>
#include <QString>

namespace AITalk
{

// 멤버별 SD 캐릭터 이모티콘 (CodePet 에서 가져옴).
// 그림은 res/emoticons/<멤버 id>/<file>.png 이고, AI 는 한국어 키("우헤헤")로 고른다.
struct EmoticonInfo {
    QString key;      // AI 가 쓰는 이름
    QString file;     // 리소스 파일 이름 (확장자 제외)
    QString meaning;  // 언제 쓰는지 (프롬프트에 그대로 들어간다)
};

namespace Emoticons {
const QList<EmoticonInfo>& all();
const EmoticonInfo* find(const QString& key);
// 이 멤버가 가진 이모티콘인가
bool isAvailable(const QString& memberId, const QString& key);
// ":/emoticons/<멤버>/<file>.png" (없으면 빈 문자열)
QString resourcePath(const QString& memberId, const QString& key);
}

} // namespace AITalk
