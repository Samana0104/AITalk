#pragma once

#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QStringList>

// 대화 기록 한 줄. messages.jsonl 에 한 줄씩 JSON 으로 쌓인다.
struct ChatMessage {
    qint64 id = 0;
    qint64 ts = 0;          // ms (epoch)
    QString from;           // 멤버 id, "user", "system"
    QString text;
    qint64 replyTo = 0;     // 인용 답장 대상 메시지 id
    QString kind;           // 시스템 알림 종류: info | error | boost | sleep
    QString model;          // AI 메시지를 쓴 모델 이름
    bool boosted = false;   // 진심모드 설정으로 쓴 메시지
    QString emoticon;       // 이모티콘 메시지면 키 (text 는 비어 있다)
    QMap<QString, QStringList> reactions; // 이모지 -> 반응한 사람들

    bool isUser() const;
    bool isSystem() const;
    bool isAgent() const;
    // 목록 미리보기·검색·내보내기에 쓰는 글 ("[이모티콘: 우헤헤]")
    QString displayText() const;

    QJsonObject toJson() const;
    static ChatMessage fromJson(const QJsonObject& o);
};
