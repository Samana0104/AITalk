#pragma once

#include <QMap>
#include <QString>
#include <QStringList>

namespace AITalk
{

// 멤버 CLI(claude, codex, agy) 실행 파일을 찾는다.
// 설정의 직접 지정 경로 > PATH > 각 CLI 의 보통 설치 위치 순서.
// Windows 의 npm 설치본은 .cmd 껍데기라 바로 실행할 수 없어서 패키지 안의 진짜 exe 를 찾는다.
namespace CliLocator {
QStringList keys();
QString resolve(const QString& key, const QMap<QString, QString>& overrides = {});
QMap<QString, QString> resolveAll(const QMap<QString, QString>& overrides = {});
// 설치 안내 (공식 설치 명령)
QString installHint(const QString& key);
}

} // namespace AITalk
