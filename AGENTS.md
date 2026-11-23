# AITalk 작업 규칙

## 프로젝트
- C++20 / Qt 6.8+ Quick(QML)·Widgets·Svg / CMake 3.21+ 데스크톱 앱이다. 화면은 `qml/`(`panels/`·`chat/`·`components/`), QML 연결 계층은 `src/ui/`다.
- 구조와 실행 방법은 `README.md`, 빌드 정의는 `CMakeLists.txt`를 확인한다. 필요한 파일만 읽는다.
- 사용자와의 설명 및 주석은 한국어를 기본으로 하고, 코드 식별자는 영어로 작성한다.

## 코딩 스타일
| 대상 | 규칙 | 예시 |
| --- | --- | --- |
| 클래스·구조체·열거형·타입 별칭 | PascalCase | `ChatSession`, `MemberStatus` |
| 함수·메서드·직접 정의한 signal/slot | PascalCase | `SendMessage()`, `LoadConfig()`, `MessageReceived()` |
| 지역 변수·매개변수·멤버 변수 | camelCase | `messageCount`, `sessionId`, `sendButton` |
| 상수·constexpr 변수 | camelCase | `maxRetryCount`, `defaultTimeoutMs` |
| enum class 내부 열거형 값 | UPPER_SNAKE_CASE | `MemberStatus::THINKING`, `MemberStatus::WAITING_FOR_REPLY` |
| 매크로 | UPPER_SNAKE_CASE | `AITALK_VERSION` |
| 클래스 파일 | 클래스명과 일치하는 `.h` / `.cpp` | `ChatSession.h`, `ChatSession.cpp` |

- 클래스·구조체·함수·열거형 타입 이름의 대문자 시작은 PascalCase를 뜻한다. 열거형 값과 매크로는 전체 대문자로 작성한다. 변수에는 `m_`, `s_`, 헝가리안 접두사를 붙이지 않는다.
- 열거형은 일반 `enum` 대신 `enum class`를 사용한다. 타입 이름은 구조체와 동일하게 PascalCase, 내부 값 이름은 UPPER_SNAKE_CASE로 작성한다.
- bool 이름은 `isReady`, `hasMessages`, `canSend`처럼 의미가 드러나게 짓는다. 불필요한 축약어는 피한다.
- Qt/표준 라이브러리 API, `main`, `eventFilter`, `paintEvent` 같은 재정의 함수는 요구된 이름을 유지하고 `override`를 명시한다. 람다를 담는 변수는 camelCase다.
- 기존 함수가 camelCase여도 새 함수에는 위 규칙을 적용한다. 관련 없는 코드를 일괄 개명하지 않는다. 개명하는 경우 선언·정의·호출부·Qt 연결·관련 문서까지 함께 수정한다.
- 들여쓰기는 공백 4칸. 중괄호는 Allman 스타일로 여는 중괄호와 닫는 중괄호를 각각 독립된 줄에 배치한다. 함수·클래스·구조체·enum class·네임스페이스·람다·제어문에 적용한다. `else`, `else if`, `catch`도 닫는 중괄호 다음 줄에 작성한다.
- `if`, `for`, `while` 등 제어문은 본문이 한 줄이어도 중괄호를 생략하지 않는다. 제어문 키워드와 여는 소괄호 사이에는 공백을 넣지 않는다: `if(isReady)`.
- 새로 작성하거나 수정하는 블록에는 위 중괄호 규칙을 적용한다. 관련 없는 기존 코드를 일괄 재포맷하지 않는다. include 구성은 수정하는 파일을 따르고, 헤더는 `#pragma once`를 사용한다.
- 클래스 선언은 `Q_OBJECT`·`Q_PROPERTY` 다음에 `public` 함수 → `public slots` → `signals` → `protected` → `private` 함수·slot → `private` 멤버 변수 순서로 쓴다. 함수 구역과 멤버 변수 구역은 같은 접근 수준이어도 접근 지정자를 따로 적어 나눈다. 새로 작성하거나 수정하는 클래스에 적용한다.
- 구조체·클래스의 멤버 변수는 같은 자료형끼리 연속해서 선언한다. 필드 순서를 바꾸면 `{ ... }` 순서 초기화 사용처도 함께 고친다.
- 읽기 전용 멤버 함수는 `const`, 복사할 필요 없는 큰 입력은 `const T&`, 널 포인터는 `nullptr`를 사용한다. 소유권은 Qt 부모 객체나 RAII로 명확히 한다.

```cpp
enum class MemberStatus
{
    IDLE = 0,
    THINKING = 1,
    WAITING_FOR_REPLY = 2
};

void SendIfReady(bool isReady)
{
    if(isReady)
    {
        SendMessage();
    }
}
```

## 최소 구현 원칙 — Ponytail 기반
공식 원칙: https://github.com/DietrichGebert/ponytail/blob/main/AGENTS.md
이 문서는 프로젝트용 요약 규칙이며 플러그인 설치나 훅·명령어 활성화를 의미하지 않는다.

- 요구사항을 완전히 만족하는 가장 작은 변경을 선택한다. 요청하지 않은 기능, 미래용 옵션, 래퍼, 추상화는 추가하지 않는다.
- 기존 프로젝트 구현 → 표준 라이브러리·Qt 기능 → 이미 설치된 의존성 → 필요한 최소 직접 구현 순서로 검토한다. 몇 줄을 줄이려고 의존성을 추가하지 않는다.
- 줄 수보다 가독성과 정확성을 우선한다. 난해한 한 줄 표현이나 의미 없는 축약을 피한다.
- 변경 전에 호출부와 영향을 받는 데이터·설정·테스트를 검색한다. 공통 원인을 한 곳에서 고치고 필요한 연결부까지 완료한다.
- 입력 검증, 오류 처리, 데이터 보호, 접근성, 사용자가 요청한 동작은 간결화를 이유로 제거하지 않는다.
- 주석은 코드로 드러나지 않는 이유를 짧게 설명한다. 알려진 임시 제약은 `shortcut: <제약>, <개선이 필요한 조건>`으로 기록한다.
- 검색은 `rg`로 범위를 좁히고, 이미 확인한 내용을 불필요하게 다시 읽지 않는다. 문서·로그·답변은 필요한 정보 위주로 유지한다.

## 프로젝트별 구현 규칙
- UI 문자열은 `Text("key")`와 `res/locale/<언어-지역>.csv`(`key,value`)로 관리한다. UI 코드에 표시 문구를 직접 넣지 않는다. 언어 파일 추가 시 `res/locale/locale.qrc`에도 등록한다.
- 테마는 `res/styles/`, 에이전트 프롬프트는 `res/prompts/ko/`의 기존 구성을 활용한다.
- 리소스 파일을 추가하면 `res/resources.qrc` 등록도 확인한다.
- CLI 실행은 기존 `agent` 계층을 재사용한다. UI 스레드를 동기 대기나 긴 작업으로 막지 않는다.
- 세션·설정 저장 형식을 바꿀 때는 기존 데이터 호환성을 확인한다. 사용자 데이터와 로그를 임의로 삭제하지 않는다.

## 검증 및 보고
- 코드 변경 후 환경에 맞게 CMake를 구성하고 `cmake --build build`로 검증한다. Qt 경로와 생성기는 기존 환경을 확인해 사용한다.
- 새 비단순 로직이나 버그 수정은 관련 동작을 확인할 작은 테스트 또는 재현 절차를 마련한다. 문서만 바꾼 경우 빌드는 생략해도 된다.
- 사용자 변경을 보존하고, 완료 전에 diff를 확인해 작업 범위 밖 변경이 없는지 살핀다.
- 결과는 변경 내용과 검증 결과를 간결하게 보고한다. 확인하지 못한 동작과 실제 남은 제약이 있으면 명시한다.
