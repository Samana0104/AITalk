#pragma once

#include <QString>

namespace AITalk
{

// 설정은 실행 파일 옆 config/ 에 둔다 (빌드할 때 프로젝트의 config/ 가 복사된다).
//   <exe>/config/config.json      설정
//   <exe>/config/ui.ini           창 크기·분할 위치
//
// 채팅 기록 등 앱 데이터는 실행 파일 옆 data/ 에 둔다 (포터블: 폴더만 지우면 앱이 사라진다).
//
//   <root>/sessions/<id>/       세션(대화방)마다 session.json, messages.jsonl, notes/, logs/
//   <root>/trash/                 삭제한 세션
//   <root>/cwd/<member>/          CLI 를 실행하는 빈 작업 폴더
//   <root>/tmp/<name>/            CLI 전용 임시 폴더
namespace AppPaths
{
QString dataRoot();
QString configFile();
QString uiSettingsFile();
QString sessionsDir();
QString trashDir();
QString workDir(const QString &memberId);
QString tempDir(const QString &name);
} // namespace AppPaths

} // namespace AITalk
