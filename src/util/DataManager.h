#pragma once

#include <QHash>
#include <QMutex>
#include <QObject>
#include <QReadWriteLock>
#include <QSet>
#include <QStringList>

#include "util/Singleton.h"

// 앱 전역 데이터 창구 (싱글턴). 지금은 다국어 문자열 표를 맡는다.
//
// 문자열 표: res/i18n/strings.csv
//   key,ko,en          ← 머리줄: 첫 칸은 키, 나머지는 언어 코드 (열을 추가하면 언어가 늘어난다)
//   session.new,새 채팅,New chat
// 여러 표를 불러오면 합쳐지고, 같은 키는 나중 것이 이긴다.
//
// 표는 읽기/쓰기 잠금으로 보호하므로 다른 스레드에서 불러와도 된다.
class DataManager : public QObject, public Singleton<DataManager> {
    Q_OBJECT
    SINGLETON(DataManager)

public:
    bool loadStringTable(const QString& path, QString* error = nullptr);

    QStringList languages() const;              // 표에 있는 언어 코드 (머리줄 순서)
    QString languageName(const QString& code) const; // "lang.name" 키 값 (한국어, English …)
    QString language() const;
    // "auto"(또는 빈 값)면 OS 언어. 표에 없는 언어면 en, 그것도 없으면 대체 언어.
    void setLanguage(const QString& code);
    QString fallbackLanguage() const;
    void setFallbackLanguage(const QString& code);

    // 현재 언어 → 대체 언어 → 키 그대로 (빠진 키는 한 번만 경고)
    QString text(const QString& key) const;
    bool contains(const QString& key) const;

signals:
    void languageChanged(const QString& code);

private:
    mutable QReadWriteLock lock;
    QStringList availableLanguages;
    QHash<QString, QHash<QString, QString>> strings; // key -> (언어 -> 문자열)
    QString currentLanguage = QStringLiteral("ko");
    QString fallback = QStringLiteral("ko");

    mutable QMutex missingMutex;
    mutable QSet<QString> missing;
};

// 언리얼 LOCTEXT 처럼 키로 현재 언어의 문자열을 가져온다.
//   Text("session.new")                 -> "새 채팅" / "New chat"
//   Text("toast.exported", path)        -> 값의 %1, %2 … 에 인자를 차례로 넣는다
inline QString Text(const QString& key)
{
    return DataManager::instance().text(key);
}

inline QString Text(const char* key)
{
    return DataManager::instance().text(QString::fromLatin1(key));
}

template <typename... Args>
QString Text(const char* key, const Args&... args)
{
    QString s = Text(key);
    ((s = s.arg(args)), ...);
    return s;
}
