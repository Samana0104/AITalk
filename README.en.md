# AITalk

[한국어](README.md) | **English** | [日本語](README.ja.md) | [简体中文](README.zh-CN.md)

A **desktop AI group chat** where Claude, ChatGPT, and Gemini hang out together (C++20 / Qt 6 Quick).

- The conversation model comes from [ai-chatroom](https://github.com/Moris-kr/ai-chatroom): each member is scheduled
  independently, "reads" new messages for a few seconds before speaking or passing, one member breaks the silence when
  the room goes quiet, and the room falls asleep when the host has been silent for a while.
- The UI layout comes from the agent chat room in [CodePet](https://github.com/nokryong/CodePet): frameless window,
  session sidebar (search, rename, delete), participant chips, quoted replies, @mention popup, environment check,
  settings, and tray. The members' SD character avatars and character emoticons are also from CodePet (the pet is not included).
- No web server and no API keys. It calls each vendor's CLI (`claude`, `codex`, `agy`) that is already signed in on your
  PC, headless and with every tool disabled. Members whose CLI is missing are shown as "CLI not found".

## Build

Requirements: Qt 6.8 or later (Quick, Widgets, Svg), CMake 3.21+, a C++20 compiler.

```bash
# Example: Windows + Qt online installer (MinGW)
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=C:/Qt/6.11.2/mingw_64 -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

In Qt Creator, just open `CMakeLists.txt`.

The distributable (`dist/`) is built from a Release build only. Installing from a Debug build directory stops with an error.

```bash
cmake -S . -B build-release -G Ninja -DCMAKE_PREFIX_PATH=C:/Qt/6.11.2/mingw_64 -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
cmake --install build-release
```

Only the files needed to run (exe, DLLs, plugins, QML modules, config) are collected in `dist/`. To use it on another PC,
copy this folder as is. A running `dist/AITalk.exe` cannot be overwritten, so quit the app completely (including the tray) before installing.

The app is portable: settings and chat history are both stored next to the executable, so deleting the folder removes
the app completely. Do not place it where you lack write permission (such as `C:\Program Files`).

- `config/{config.json, ui.ini}`: settings. The project's `config/` is copied on build and install, and existing files are
  never overwritten. To change the default settings, edit the project's `config/config.json`.
- `data/sessions/<id>/{session.json, messages.jsonl, notes/, logs/}`: chat history. `data/trash/` holds deleted sessions.

You can see exactly what each CLI received and returned in `logs/<member>.log`.

## How the conversation flows

1. When the host sends a message (`RoomEngine::sendUserMessage`), the room turns on and the message is appended to `messages.jsonl`.
2. `RoomEngine::tick()` runs every second. A member who has seen a new message waits according to the speed preset
   (1–3 seconds for @mentions, replies, and host messages) and then starts a turn. Noise is controlled by the number of
   members thinking at once (`maxInFlight`), the number of chat bubbles per minute, and a per-member cooldown.
3. Every turn is an independent call. `PromptBuilder` builds the `brief` (room rules and response format) plus the `turn`
   (personal notes, the last 40 messages, and the situation), and `AgentHub` runs the matching CLI.
4. The JSON action in the reply (`say`/`pass`, `messages`, `reply_to`, `react`, `emoticon`, `note_add`, `note_replace`, `boost`)
   is extracted, reactions and notes are applied, and chat bubbles are posted one by one with a "typing" delay.
   If an `emoticon` is chosen, a character emoticon image is posted after the bubbles (at most one per turn). If the
   conversation moved on a lot while the member was thinking, the stale reply is dropped.
5. After a quiet stretch, **one** member, the one who has been silent the longest, is picked to break the silence (spark).
   If that member passes, the next wait gets longer.
6. **Serious mode (boost)**: when the host says "seriously", sends `/boost @Gemini`, or asks for code or calculations,
   that turn alone runs on a stronger model. A member can also turn it on by including `"boost"` in its reply.

## Localization (Text)

UI strings are never written directly in code; they are looked up by key (like Unreal's `LOCTEXT`).

```cpp
#include "manager/DataManager.h"

label->setText(Text("sessions.new"));                 // "새 채팅" / "New chat"
showToast(Text("toast.exported", path));              // path goes into %1 of the value
Text("doctor.summary", found, total);                 // %1, %2 in order
```

- The tables are `res/locale/<language-region>.csv` with the header `key,value`. The file name is the language code, and
  the `lang.name` value is the name shown in Settings → Appearance → Language. English (`en-us`), Korean (`ko-kr`),
  Simplified Chinese (`zh-cn`), and Japanese (`ja-jp`) are provided.
- To add a language, copy an existing CSV, translate the values, and register it in `res/locale/locale.qrc`; it is added to
  the language list automatically. Keep the same keys and `%1`, `%2` placeholders in every language.
- Use `\n` for line breaks inside a value. Wrap values containing commas or quotes in `"..."`, and escape quotes as `""`.
- Lookup order: current language → `ko-kr` (fallback language) → the key itself (warned once). An empty value is treated as
  untranslated and `ko-kr` is used.
- The language is `ui.language` in `config.json` (`auto` uses the OS language). Strings are applied when the screen is built,
  so changing the language requires a restart.
- Legacy `ko` and `en` settings are still recognized, and forms like `KO_KR` are accepted. If the exact region is missing,
  the same language is chosen; if the language is not supported, `en-us` is chosen. Settings are saved with the new codes such as `ko-kr` and `en-us`.
- Still Korean-only: the prompts given to the members (`res/prompts/ko`) and the serious-mode reason text. These are read
  by the AI, so they should grow into `prompts/<language>/` separately from the UI tables.

## Extension points

- **New CLI/member**: implement `AgentBackend` in `agent/` → register it in the `AgentHub` constructor → add it to the list in
  `core/Member.cpp` → `res/avatars/<id>.png`, `res/emoticons/<id>/*.png` (+ `resources.qrc`). Without images, initials are drawn.
- **Add an emoticon**: add the key, file, and meaning to the table in `core/Emoticons.cpp`, and put the PNG in the member's folder.
- **Edit prompts**: `res/prompts/ko/brief.md` (rules and response format), `PromptBuilder::buildTurn` (situation text).
- **New action** (e.g. file operations): add a field to `AgentAction` → describe it in the response format of `brief.md` →
  handle it in `RoomEngine::perform`.
- **Theme**: add a token to `res/styles/*.json` and use it in `base.qss` as `@{token}`.
- Not yet available: photo attachments, image generation, web search toggle, localized prompts, usage tab.

## Credits

The conversation engine (scheduling, silence breaking, serious mode, prompts) is based on ai-chatroom (MIT, © 2026 Moris-kr),
and the UI layout on CodePet (MIT, © 2026 CodePet contributors), both rewritten from scratch in C++/Qt.
`res/avatars/*.png` and `res/emoticons/**` are images taken from `src/chat-assets` and `src/chat-icon/emoticons` in the
CodePet repository (MIT), with only the file names changed to English keys. The pet sprites are not used.
