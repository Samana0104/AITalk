#include "util/CsvReader.h"

#include <QFile>

namespace AITalk
{

namespace CsvReader {

QList<QStringList> parse(const QString& text)
{
    QList<QStringList> rows;
    QStringList row;
    QString field;
    bool inQuotes = false;
    bool fieldWasQuoted = false;

    auto endField = [&] {
        row << field;
        field.clear();
        fieldWasQuoted = false;
    };
    auto endRow = [&] {
        endField();
        const bool blank = row.size() == 1 && row.first().trimmed().isEmpty();
        const bool comment = !row.isEmpty() && row.first().startsWith(QLatin1Char('#'));
        if (!blank && !comment)
            rows << row;
        row.clear();
    };

    const qsizetype n = text.size();
    for (qsizetype i = 0; i < n; ++i) {
        const QChar c = text.at(i);
        if (inQuotes) {
            if (c == QLatin1Char('"')) {
                if (i + 1 < n && text.at(i + 1) == QLatin1Char('"')) {
                    field += QLatin1Char('"');
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                field += c;
            }
            continue;
        }
        if (c == QLatin1Char('"') && field.isEmpty() && !fieldWasQuoted) {
            inQuotes = true;
            fieldWasQuoted = true;
        } else if (c == QLatin1Char(',')) {
            endField();
        } else if (c == QLatin1Char('\n')) {
            endRow();
        } else if (c == QLatin1Char('\r')) {
            // \r\n 의 \r 은 무시
        } else if (c == QChar(0xFEFF) && i == 0) {
            // BOM
        } else {
            field += c;
        }
    }
    if (!field.isEmpty() || !row.isEmpty())
        endRow();
    return rows;
}

bool readFile(const QString& path, QList<QStringList>* rows, QString* error)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (error)
            *error = f.errorString();
        return false;
    }
    *rows = parse(QString::fromUtf8(f.readAll()));
    return true;
}

QString unescape(const QString& value)
{
    if (!value.contains(QLatin1Char('\\')))
        return value;
    QString out;
    out.reserve(value.size());
    for (qsizetype i = 0; i < value.size(); ++i) {
        const QChar c = value.at(i);
        if (c != QLatin1Char('\\') || i + 1 >= value.size()) {
            out += c;
            continue;
        }
        const QChar next = value.at(++i);
        if (next == QLatin1Char('n'))
            out += QLatin1Char('\n');
        else if (next == QLatin1Char('t'))
            out += QLatin1Char('\t');
        else if (next == QLatin1Char('\\'))
            out += QLatin1Char('\\');
        else {
            out += c; // 모르는 표기는 그대로
            out += next;
        }
    }
    return out;
}

}

} // namespace AITalk
