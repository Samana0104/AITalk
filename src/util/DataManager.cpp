#include "util/DataManager.h"

#include <QDebug>
#include <QLocale>

#include "util/CsvReader.h"

DataManager& DataManager::instance()
{
    static DataManager inst; // C++11 부터 초기화는 스레드 안전
    return inst;
}

bool DataManager::loadStringTable(const QString& path, QString* error)
{
    QList<QStringList> rows;
    if (!CsvReader::readFile(path, &rows, error))
        return false;
    if (rows.isEmpty() || rows.first().size() < 2) {
        if (error)
            *error = QStringLiteral("no header row (key,<lang>,...)");
        return false;
    }

    const QStringList header = rows.takeFirst();
    QStringList langs;
    for (qsizetype i = 1; i < header.size(); ++i)
        langs << header.at(i).trimmed();

    QWriteLocker lock(&m_lock);
    for (const QString& l : langs)
        if (!m_languages.contains(l))
            m_languages << l;
    for (const QStringList& row : std::as_const(rows)) {
        const QString key = row.value(0).trimmed();
        if (key.isEmpty())
            continue;
        QHash<QString, QString>& entry = m_strings[key];
        for (qsizetype i = 0; i < langs.size(); ++i) {
            const QString value = row.value(i + 1);
            if (!value.isEmpty()) // 빈 칸은 번역 안 됨 → 대체 언어로
                entry.insert(langs.at(i), CsvReader::unescape(value));
        }
    }
    return true;
}

QStringList DataManager::languages() const
{
    QReadLocker lock(&m_lock);
    return m_languages;
}

QString DataManager::languageName(const QString& code) const
{
    QReadLocker lock(&m_lock);
    return m_strings.value(QStringLiteral("lang.name")).value(code, code);
}

QString DataManager::language() const
{
    QReadLocker lock(&m_lock);
    return m_language;
}

void DataManager::setLanguage(const QString& code)
{
    QString resolved = code.trimmed();
    {
        QWriteLocker lock(&m_lock);
        if (resolved.isEmpty() || resolved == QLatin1String("auto"))
            resolved = QLocale::system().name().section(QLatin1Char('_'), 0, 0); // ko_KR -> ko
        if (!m_languages.contains(resolved))
            resolved = m_languages.contains(QStringLiteral("en")) ? QStringLiteral("en") : m_fallback;
        if (resolved == m_language)
            return;
        m_language = resolved;
    }
    emit languageChanged(resolved);
}

QString DataManager::fallbackLanguage() const
{
    QReadLocker lock(&m_lock);
    return m_fallback;
}

void DataManager::setFallbackLanguage(const QString& code)
{
    QWriteLocker lock(&m_lock);
    m_fallback = code;
}

QString DataManager::text(const QString& key) const
{
    {
        QReadLocker lock(&m_lock);
        const auto it = m_strings.constFind(key);
        if (it != m_strings.cend()) {
            const auto value = it->constFind(m_language);
            if (value != it->cend())
                return value.value();
            const auto fb = it->constFind(m_fallback);
            if (fb != it->cend())
                return fb.value();
        }
    }
    QMutexLocker lock(&m_missingMutex);
    if (!m_missing.contains(key)) {
        m_missing.insert(key);
        qWarning().noquote() << "[DataManager] missing text key:" << key;
    }
    return key;
}

bool DataManager::contains(const QString& key) const
{
    QReadLocker lock(&m_lock);
    return m_strings.contains(key);
}
