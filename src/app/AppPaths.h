#pragma once

#include <QString>

// 앱 데이터 위치. 기본은 %APPDATA%/AITalk (macOS/Linux 는 각 OS 의 AppData 위치),
// 환경 변수 AITALK_HOME 으로 바꿀 수 있다.
//
//   <root>/config/config.json     설정
//   <root>/config/ui.ini          창 크기·분할 위치
//   <root>/sessions/<id>/         세션(대화방)마다 session.json, messages.jsonl, notes/, logs/
//   <root>/trash/                 삭제한 세션
//   <root>/cwd/<member>/          CLI 를 실행하는 빈 작업 폴더
//   <root>/tmp/<name>/            CLI 전용 임시 폴더
namespace AppPaths {
QString dataRoot();
QString configFile();
QString uiSettingsFile();
QString sessionsDir();
QString trashDir();
QString workDir(const QString& memberId);
QString tempDir(const QString& name);
}
