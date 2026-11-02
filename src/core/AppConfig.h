#pragma once

#include <QJsonObject>
#include <QMap>
#include <QString>

// 멤버 한 명의 모델 설정. boost* 는 진심모드 턴에서 덮어쓸 값(비우면 평소 값 유지).
struct AgentConfig {
    QString model;
    QString effort;
    QString boostModel;
    QString boostEffort;
    int rotate = 4; // agy: 대화(conversation) 하나를 몇 턴 재사용할지

    bool hasBoost() const { return !boostModel.isEmpty() || !boostEffort.isEmpty(); }
    QString effectiveModel(bool boost) const;
    QString effectiveEffort(bool boost) const;
    // "sonnet" / "gpt-6-sol (effort low)" 처럼 멤버들과 화면에 보여 줄 이름
    QString label(bool boost) const;
};

// config.json 전체. 처음 실행하면 기본값으로 파일을 만든다.
struct AppConfig {
    QString userName = QStringLiteral("방장");
    QString defaultRoomName = QStringLiteral("AI 단톡방");
    QString speed = QStringLiteral("normal"); // 새 세션의 기본 대화 속도: slow | normal | fast
    int autoSleepMinutes = 30;                // 방장이 이 시간 말이 없으면 방이 잠든다 (0: 안 잠듦)
    int maxInFlight = 3;                      // 동시에 생각할 수 있는 멤버 수
    int historyForPrompt = 40;                // 프롬프트에 넣을 최근 메시지 수
    int turnTimeoutSec = 150;

    QString boostMode = QStringLiteral("auto"); // 새 세션의 진심모드: auto | manual | off
    int boostTimeoutSec = 360;
    int boostSelfCooldownSec = 180;
    int boostAiRequestCooldownSec = 180;

    bool sparkEnabled = true; // 침묵 깨기
    double sparkBackoff = 1.6;
    int sparkMaxWaitSec = 1800;

    QMap<QString, QString> bins;           // CLI 경로 직접 지정: claude, codex, agy
    QMap<QString, AgentConfig> agents;     // 멤버 id -> 모델 설정

    QString language = QStringLiteral("auto"); // auto | res/i18n/strings.csv 의 언어 코드 (ko, en …)
    QString theme = QStringLiteral("light"); // light | dark
    int fontSize = 10;                       // pt
    bool closeToTray = false;

    static AppConfig defaults();
    static AppConfig load(const QString& file);
    bool save(const QString& file) const;

    QJsonObject toJson() const;
    static AppConfig fromJson(const QJsonObject& o);
};
