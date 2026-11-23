# AITalk

Claude · ChatGPT · Gemini 셋이 상주하는 **데스크톱 AI 단톡방** (C++20 / Qt 6 Quick).

- 대화 방식은 [ai-chatroom](https://github.com/Moris-kr/ai-chatroom)에서 가져왔다: 멤버마다 따로 스케줄링되고,
  새 메시지를 몇 초 "읽은" 뒤 말하거나 넘기고, 조용하면 한 명이 침묵을 깨고, 방장이 오래 말이 없으면 잠든다.
- UI 구성은 [CodePet](https://github.com/nokryong/CodePet)의 에이전트 채팅방에서 가져왔다: 프레임 없는 창,
  세션 사이드바(검색·이름 변경·삭제), 참가자 칩, 인용 답장, @멘션 팝업, 환경 진단, 설정, 트레이.
  멤버 SD 캐릭터 프사와 캐릭터 이모티콘도 CodePet 것을 쓴다 (펫은 제외).
- 웹 서버도, API 키도 없다. 내 PC에 로그인된 각 회사 CLI(`claude`, `codex`, `agy`)를 헤드리스로,
  도구를 전부 끈 채 부른다. 없는 CLI의 멤버는 "CLI 없음"으로 표시된다.

## 빌드

필요: Qt 6.8 이상 (Quick, Widgets, Svg), CMake 3.21+, C++20 컴파일러.

```bash
# Windows + Qt 온라인 설치(MinGW) 예시
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=C:/Qt/6.11.2/mingw_64 -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Qt Creator에서는 `CMakeLists.txt`를 열면 된다. 빌드 폴더 밖에서 실행하려면 `windeployqt build/AITalk.exe`.

데이터는 `%APPDATA%/AITalk` (환경 변수 `AITALK_HOME`으로 변경)에 저장된다:
`config.json`, `sessions/<id>/{session.json, messages.jsonl, notes/, logs/}`.
CLI가 실제로 무엇을 받고 돌려줬는지는 `logs/<멤버>.log`에서 볼 수 있다.

## 대화가 굴러가는 방식

1. 방장이 메시지를 보내면(`RoomEngine::sendUserMessage`) 방이 켜지고 `messages.jsonl`에 쌓인다.
2. `RoomEngine::tick()`이 1초마다 돈다. 새 메시지를 본 멤버는 속도 프리셋만큼 기다렸다가(@호출·답장·방장 메시지는 1~3초)
   턴을 시작한다. 동시 생각 수(`maxInFlight`), 분당 말풍선 수, 멤버별 쿨다운으로 시끄러움을 조절한다.
3. 매 턴은 독립 호출이다. `PromptBuilder`가 `brief`(방 규칙·응답 형식) + `turn`(개인 메모·최근 대화 40개·상황)을 만들고,
   `AgentHub`가 해당 CLI를 실행한다.
4. 답에서 JSON 액션(`say`/`pass`, `messages`, `reply_to`, `react`, `emoticon`, `note_add`, `note_replace`, `boost`)을 꺼내
   반응·메모를 적용하고, 말풍선을 "입력 중" 시간을 두고 하나씩 올린다.
   `emoticon`을 고르면 말풍선 뒤에 캐릭터 이모티콘이 그림으로 올라간다(한 턴에 최대 1개). 생각하는 동안 대화가 많이 흘렀으면 뒷북은 버린다.
5. 한동안 조용하면 제일 오래 말 안 한 멤버 **한 명**을 골라 침묵을 깨게 한다(spark). pass하면 다음 대기가 길어진다.
6. **진심모드**: 방장이 "진지하게"라고 하거나 `/boost @Gemini`, 혹은 코드·계산 요청이 오면 그 턴만 더 센 모델로 돌린다.
   멤버가 답에 `"boost"`를 넣어 직접 켤 수도 있다.

## 다국어 (Text)

UI 문자열은 코드에 직접 쓰지 않고 키로 부른다 (언리얼 `LOCTEXT` 처럼).

```cpp
#include "manager/DataManager.h"

label->setText(Text("sessions.new"));                 // 새 채팅 / New chat
showToast(Text("toast.exported", path));              // 값의 %1 에 path
Text("doctor.summary", found, total);                 // %1, %2 차례로
```

- 표는 `res/locale/<언어-지역>.csv`이며 머리줄은 `key,value`다. 파일 이름이 언어 코드이고,
  `lang.name` 값이 설정 → 화면 → 언어에 표시되는 이름이다. 영어(`en-us`), 한국어(`ko-kr`), 중국어 간체(`zh-cn`), 일본어(`ja-jp`)를 제공한다.
- 새 언어는 기존 CSV를 복사해 값을 번역하고 `res/locale/locale.qrc`에 등록하면 언어 목록에 자동으로 추가된다.
  모든 언어에서 같은 키와 `%1`, `%2` 자리표시자를 유지한다. 자세한 형식과 검증 방법은 [res/locale/README.md](res/locale/README.md)를 참고한다.
- 값 안의 줄바꿈은 `\n`, 쉼표나 따옴표가 있으면 `"..."`로 감싸고 따옴표는 `""`.
- 찾는 순서: 현재 언어 → `ko-kr`(대체 언어) → 키 그대로(경고 한 번). 빈 값은 번역 안 된 것으로 보고 `ko-kr`을 쓴다.
- 언어는 `config.json`의 `ui.language`(`auto`면 OS 언어). 위젯을 만들 때 문자열이 들어가므로 바꾸면 다시 시작해야 한다.
- 기존 `ko`·`en` 설정은 계속 인식하며 `KO_KR` 같은 표기도 허용한다. 정확한 지역이 없으면 같은 언어를,
  지원하지 않는 언어면 `en-us`를 선택한다. 설정에서 저장할 때는 `ko-kr`·`en-us` 같은 새 코드를 쓴다.
- 아직 한국어만인 것: 멤버에게 주는 프롬프트(`res/prompts/ko`)와 진심모드 이유 문구. 이건 AI가 읽는 글이라 UI 표와 따로
  `prompts/<언어>/`로 늘리는 게 맞다.

## 확장 포인트

- **새 CLI/멤버**: `agent/`에 `AgentBackend` 구현 → `AgentHub` 생성자에 등록 → `core/Member.cpp` 목록에 추가 →
  `res/avatars/<id>.png`, `res/emoticons/<id>/*.png` (+ `resources.qrc`). 그림이 없으면 이니셜로 그린다.
- **이모티콘 추가**: `core/Emoticons.cpp` 표에 키·파일·의미를 넣고 멤버 폴더에 PNG를 넣는다.
- **프롬프트 수정**: `res/prompts/ko/brief.md`(규칙·응답 형식), `PromptBuilder::buildTurn`(상황 문구).
- **새 액션**(예: 파일 작업): `AgentAction`에 필드 추가 → `brief.md` 응답 형식에 설명 → `RoomEngine::perform`에서 처리.
- **테마**: `res/styles/*.json`에 토큰 추가 후 `base.qss`에서 `@{토큰}`으로 사용.
- 아직 없는 것: 사진 첨부, 이미지 생성, 웹 검색 토글, 프롬프트 다국어, 사용량 탭.

## 출처

대화 엔진(스케줄링·침묵 깨기·진심모드·프롬프트)은 ai-chatroom(MIT, © 2026 Moris-kr)을,
UI 구성은 CodePet(MIT, © 2026 CodePet contributors)을 참고해 C++/Qt로 새로 작성했다.
`res/avatars/*.png`, `res/emoticons/**`는 CodePet 저장소(MIT)의 `src/chat-assets`, `src/chat-icon/emoticons`에서
가져온 그림이다(파일 이름만 영문 키로 바꿈). 펫 스프라이트는 쓰지 않았다.
