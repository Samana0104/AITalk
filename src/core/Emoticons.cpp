#include "core/Emoticons.h"

#include <QFile>

namespace Emoticons {

const QList<EmoticonInfo>& all()
{
    static const QList<EmoticonInfo> list = {
        { "검토완료", "reviewed", "뭔가를 다 따져 보고 확인을 끝냈을 때" },
        { "그건안돼", "not-allowed", "분명히 거절하거나 반대할 때" },
        { "그정돈아님", "skeptical", "상대의 과장된 평가를 차분히 낮춰 말할 때" },
        { "좋은데", "happy", "제안이나 결과에 명확히 동의하고 만족할 때" },
        { "내가볼게", "inspecting", "직접 확인하거나 맡겠다고 나설 때" },
        { "파일줘봐", "file-request", "판단하려면 더 자세한 걸 보여 달라고 할 때" },
        { "하트", "heart", "고마움·애정·강한 호의를 따뜻하게 표현할 때" },
        { "잠깐만", "thinking", "생각할 시간이 잠깐 필요할 때" },
        { "이건웃기네", "lol", "상황이 명백히 재미있거나 어이없어 웃길 때" },
        { "오류났다", "blocked", "뭔가 틀어졌거나 막혔을 때" },
        { "수정완료", "fixed", "고치거나 바로잡는 걸 끝냈을 때" },
        { "몰루", "uncertain", "정보가 부족해 모르겠거나 판단을 미룰 때" },
        { "우헤헤", "uhehe", "작은 성공이나 장난스러운 만족감에 은근히 득의양양할 때" },
        { "주인님", "greeting", "방장을 친근하게 부르며 반가움을 표현할 때" },
        { "멍청", "insult", "자기 실수나 착각을 가볍게 자책할 때만. 남을 모욕하는 데는 쓰지 마" },
    };
    return list;
}

const EmoticonInfo* find(const QString& key)
{
    const QString k = key.trimmed();
    for (const EmoticonInfo& e : all())
        if (e.key == k)
            return &e;
    return nullptr;
}

QString resourcePath(const QString& memberId, const QString& key)
{
    const EmoticonInfo* e = find(key);
    if (!e)
        return {};
    const QString path = QStringLiteral(":/emoticons/%1/%2.png").arg(memberId, e->file);
    return QFile::exists(path) ? path : QString();
}

bool isAvailable(const QString& memberId, const QString& key)
{
    return !resourcePath(memberId, key).isEmpty();
}

}
