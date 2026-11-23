#include "manager/DataManager.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QLocale>

#include "util/CsvReader.h"

bool DataManager::LoadLocales(const QString& directory, QString* error)
{
    const QDir localeDirectory(directory);
    const QStringList files = localeDirectory.entryList({ QStringLiteral("*.csv") }, QDir::Files, QDir::Name);
    auto fail = [error](const QString& message)
    {
        if(error)
        {
            *error = message;
        }
        return false;
    };
    if(files.isEmpty())
    {
        return fail(QStringLiteral("No locale CSV files in %1").arg(directory));
    }

    QStringList languages;
    QHash<QString, QHash<QString, QString>> translations;
    for(const QString& file : files)
    {
        const QString path = localeDirectory.filePath(file);
        const QString code = QFileInfo(file).completeBaseName().toLower().replace(QLatin1Char('_'), QLatin1Char('-'));
        QList<QStringList> rows;
        QString readError;
        if(!CsvReader::readFile(path, &rows, &readError))
        {
            return fail(QStringLiteral("%1: %2").arg(path, readError));
        }
        if(rows.isEmpty() || rows.takeFirst() != QStringList { QStringLiteral("key"), QStringLiteral("value") })
        {
            return fail(QStringLiteral("%1: expected key,value header").arg(path));
        }
        if(languages.contains(code))
        {
            return fail(QStringLiteral("%1: duplicate language %2").arg(path, code));
        }
        QSet<QString> keys;
        for(const QStringList& row : std::as_const(rows))
        {
            const QString key = row.value(0).trimmed();
            if(row.size() != 2 || key.isEmpty() || keys.contains(key))
            {
                return fail(QStringLiteral("%1: invalid or duplicate key %2").arg(path, key));
            }
            keys.insert(key);
            if(!row.at(1).isEmpty()) // 빈 값은 번역 안 됨 → 대체 언어로
            {
                translations[key].insert(code, CsvReader::unescape(row.at(1)));
            }
        }
        if(translations.value(QStringLiteral("lang.name")).value(code).isEmpty())
        {
            return fail(QStringLiteral("%1: missing lang.name").arg(path));
        }
        languages.append(code);
    }
    {
        QWriteLocker lock(&this->lock);
        this->availableLanguages = languages;
        this->strings = translations;
    }
    if(error)
    {
        error->clear();
    }
    return true;
}

QString DataManager::ResolveLanguage(const QString& code) const
{
    QReadLocker lock(&this->lock);
    return ResolveLanguageLocked(code);
}

QString DataManager::ResolveLanguageLocked(const QString& code) const
{
    QString resolved = code.trimmed().toLower().replace(QLatin1Char('_'), QLatin1Char('-'));
    if(resolved.isEmpty() || resolved == QLatin1String("auto"))
    {
        resolved = QLocale::system().name().toLower().replace(QLatin1Char('_'), QLatin1Char('-'));
    }
    if(this->availableLanguages.contains(resolved))
    {
        return resolved;
    }
    // 기존 ko/en 설정과 en-GB 같은 다른 지역도 같은 언어의 번역을 사용한다.
    const QString prefix = resolved.section(QLatin1Char('-'), 0, 0) + QLatin1Char('-');
    for(const QString& available : this->availableLanguages)
    {
        if(available.startsWith(prefix))
        {
            return available;
        }
    }
    return this->availableLanguages.contains(QStringLiteral("en-us")) ? QStringLiteral("en-us") : this->fallback;
}

QStringList DataManager::languages() const
{
    QReadLocker lock(&this->lock);
    return this->availableLanguages;
}

QString DataManager::languageName(const QString& code) const
{
    QReadLocker lock(&this->lock);
    return this->strings.value(QStringLiteral("lang.name")).value(code, code);
}

QString DataManager::language() const
{
    QReadLocker lock(&this->lock);
    return this->currentLanguage;
}

void DataManager::setLanguage(const QString& code)
{
    QString resolved;
    {
        QWriteLocker lock(&this->lock);
        resolved = ResolveLanguageLocked(code);
        if(resolved == this->currentLanguage)
        {
            return;
        }
        this->currentLanguage = resolved;
    }
    emit languageChanged(resolved);
}

QString DataManager::fallbackLanguage() const
{
    QReadLocker lock(&this->lock);
    return this->fallback;
}

void DataManager::setFallbackLanguage(const QString& code)
{
    QWriteLocker lock(&this->lock);
    this->fallback = ResolveLanguageLocked(code);
}

QString DataManager::text(const QString& key) const
{
    {
        QReadLocker lock(&this->lock);
        const auto it = this->strings.constFind(key);
        if (it != this->strings.cend()) {
            const auto value = it->constFind(this->currentLanguage);
            if (value != it->cend())
                return value.value();
            const auto fb = it->constFind(this->fallback);
            if (fb != it->cend())
                return fb.value();
        }
    }
    QMutexLocker lock(&this->missingMutex);
    if (!this->missing.contains(key)) {
        this->missing.insert(key);
        qWarning().noquote() << "[DataManager] missing text key:" << key;
    }
    return key;
}

bool DataManager::contains(const QString& key) const
{
    QReadLocker lock(&this->lock);
    return this->strings.contains(key);
}
