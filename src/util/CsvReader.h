#pragma once

#include <QList>
#include <QString>
#include <QStringList>

namespace AITalk
{

// RFC 4180 식 CSV 읽기.
//  - 쉼표로 구분, "..." 안에서는 쉼표·줄바꿈을 그대로 쓸 수 있고 "" 는 따옴표 하나
//  - UTF-8 (BOM 있어도 됨), 줄 끝은 \n 또는 \r\n
//  - 빈 줄과 # 으로 시작하는 줄은 주석으로 건너뛴다
namespace CsvReader {

QList<QStringList> parse(const QString& text);
bool readFile(const QString& path, QList<QStringList>* rows, QString* error = nullptr);

// 값 안의 \n \t \\ 표기를 실제 문자로 (한 줄 CSV 로 여러 줄 문자열을 쓰기 위해)
QString unescape(const QString& value);

}

} // namespace AITalk
