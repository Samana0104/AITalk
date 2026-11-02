#pragma once

#include <QString>
#include <QStringList>
#include <optional>

// AI 가 한 턴에 돌려주는 JSON 액션 (res/prompts/ko/brief.md 의 "응답 형식").
struct AgentAction {
    QString action;          // say | pass
    QStringList messages;    // 말풍선 (최대 4개)
    qint64 replyTo = 0;
    qint64 reactId = 0;
    QString reactEmoji;
    QString noteAdd;
    QString noteReplace;
    QString emoticon;        // SD 캐릭터 이모티콘 키 (core/Emoticons)
    QString boost;          // 기본 모델로 불렸는데 진심모드를 직접 켜고 싶을 때 이유

    // 모델 답에서 첫 번째로 온전한 JSON 객체를 찾아 읽는다. 코드펜스나 앞뒤 잡담은 무시.
    static std::optional<AgentAction> parse(const QString& raw);
};
