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
// 문자열 표: locale/<언어-지역>.csv (key,value). 파일 이름이 언어 코드다.
// 폴더의 모든 CSV를 검증한 뒤 한 번에 교체한다.
//
// 표는 읽기/쓰기 잠금으로 보호하므로 다른 스레드에서 불러와도 된다.
class DataManager : public QObject, public Singleton<DataManager>
{
    Q_OBJECT
    SINGLETON(DataManager)

public:
    bool LoadLocales(const QString& directory, QString* error = nullptr);
    QString ResolveLanguage(const QString& code) const;

    QStringList languages() const;              // 파일 이름 순서의 언어 코드
    QString languageName(const QString& code) const; // "lang.name" 키 값 (한국어, English …)
    QString language() const;
    // "auto"(또는 빈 값)면 OS 언어. 미지원 언어면 en-us, 없으면 대체 언어.
    void setLanguage(const QString& code);
    QString fallbackLanguage() const;
    void setFallbackLanguage(const QString& code);

    // 현재 언어 → 대체 언어 → 키 그대로 (빠진 키는 한 번만 경고)
    QString text(const QString& key) const;
    bool contains(const QString& key) const;

signals:
    void languageChanged(const QString& code);

private:
    QString ResolveLanguageLocked(const QString& code) const;

private:
    mutable QReadWriteLock lock;
    QStringList availableLanguages;
    QHash<QString, QHash<QString, QString>> strings; // key -> (언어 -> 문자열)
    QString currentLanguage = QStringLiteral("ko-kr");
    QString fallback = QStringLiteral("ko-kr");

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
