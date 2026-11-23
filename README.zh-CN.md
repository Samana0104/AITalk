# AITalk

[한국어](README.md) | [English](README.en.md) | [日本語](README.ja.md) | **简体中文**

Claude、ChatGPT、Gemini 三位常驻的**桌面 AI 群聊**（C++20 / Qt 6 Quick）。

- 对话方式借鉴自 [ai-chatroom](https://github.com/Moris-kr/ai-chatroom)：每位成员独立调度，看到新消息后先“阅读”几秒，
  再发言或跳过；群里安静时会有一位成员打破沉默；群主长时间不说话，聊天室就会休眠。
- 界面布局借鉴自 [CodePet](https://github.com/nokryong/CodePet) 的智能体聊天室：无边框窗口、会话侧边栏（搜索、重命名、删除）、
  参与者标签、引用回复、@提及弹窗、环境诊断、设置和托盘。成员的 SD 角色头像和角色表情也来自 CodePet（不含宠物）。
- 没有 Web 服务器，也不需要 API 密钥。程序以无界面方式、关闭所有工具来调用你电脑上已登录的各家 CLI（`claude`、`codex`、`agy`）。
  缺少 CLI 的成员会显示为“未找到 CLI”。

## 构建

要求：Qt 6.8 及以上（Quick、Widgets、Svg）、CMake 3.21+、支持 C++20 的编译器。

```bash
# 示例：Windows + Qt 在线安装（MinGW）
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=C:/Qt/6.11.2/mingw_64 -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

在 Qt Creator 中直接打开 `CMakeLists.txt` 即可。

发布版（`dist/`）只用 Release 构建生成。从 Debug 构建目录安装时会报错并停止。

```bash
cmake -S . -B build-release -G Ninja -DCMAKE_PREFIX_PATH=C:/Qt/6.11.2/mingw_64 -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
cmake --install build-release
```

运行所需的文件（exe、DLL、插件、QML 模块、config）会被收集到 `dist/`。在其他电脑上使用时，直接复制这个文件夹即可。
正在运行的 `dist/AITalk.exe` 无法被覆盖，安装前请彻底退出程序（包括托盘）。

程序采用便携方式：设置和聊天记录都保存在可执行文件旁边，删除文件夹即可彻底移除程序。
请不要放在没有写入权限的位置（如 `C:\Program Files`）。

- `config/{config.json, ui.ini}`：设置。构建和安装时会复制项目中的 `config/`，已存在的文件不会被覆盖。
  要修改默认设置，请编辑项目中的 `config/config.json`。
- `data/sessions/<id>/{session.json, messages.jsonl, notes/, logs/}`：聊天记录。`data/trash/` 存放已删除的会话。

每个 CLI 实际收到和返回的内容可以在 `logs/<成员>.log` 中查看。

## 对话如何进行

1. 群主发送消息（`RoomEngine::sendUserMessage`）后，聊天室开启，消息追加到 `messages.jsonl`。
2. `RoomEngine::tick()` 每秒运行一次。看到新消息的成员按速度预设等待一段时间（@提及、回复和群主消息为 1~3 秒）后开始自己的回合。
   通过同时思考的成员数（`maxInFlight`）、每分钟气泡数和每位成员的冷却时间来控制热闹程度。
3. 每个回合都是独立调用。`PromptBuilder` 生成 `brief`（聊天室规则和回复格式）和 `turn`（个人笔记、最近 40 条消息和当前情况），
   再由 `AgentHub` 运行对应的 CLI。
4. 从回复中取出 JSON 动作（`say`/`pass`、`messages`、`reply_to`、`react`、`emoticon`、`note_add`、`note_replace`、`boost`），
   应用表情回应和笔记，并以“正在输入”的间隔逐条发出气泡。
   如果选择了 `emoticon`，会在气泡之后以图片形式发出角色表情（每回合最多 1 个）。如果思考期间对话已经推进很多，过时的回复会被丢弃。
5. 安静一段时间后，会挑选沉默最久的**一位**成员来打破沉默（spark）。如果该成员选择 pass，下一次等待会变长。
6. **认真模式（boost）**：当群主说“认真点”、发送 `/boost @Gemini`，或提出代码、计算类请求时，只有这一回合会使用更强的模型。
   成员也可以在回复中加入 `"boost"` 自行开启。

## 多语言（Text）

界面字符串不直接写在代码里，而是通过键来获取（类似 Unreal 的 `LOCTEXT`）。

```cpp
#include "manager/DataManager.h"

label->setText(Text("sessions.new"));                 // “새 채팅” / “New chat”
showToast(Text("toast.exported", path));              // path 填入值中的 %1
Text("doctor.summary", found, total);                 // 依次填入 %1、%2
```

- 字符串表是 `res/locale/<语言-地区>.csv`，表头为 `key,value`。文件名就是语言代码，`lang.name` 的值是
  设置 → 外观 → 语言中显示的名称。目前提供英语（`en-us`）、韩语（`ko-kr`）、简体中文（`zh-cn`）和日语（`ja-jp`）。
- 添加新语言时，复制现有 CSV 并翻译其中的值，再注册到 `res/locale/locale.qrc`，它就会自动出现在语言列表中。
  所有语言都要保持相同的键以及 `%1`、`%2` 占位符。
- 值中的换行写作 `\n`；包含逗号或引号的值用 `"..."` 包起来，引号写作 `""`。
- 查找顺序：当前语言 → `ko-kr`（备用语言）→ 键本身（只警告一次）。空值视为未翻译，改用 `ko-kr`。
- 语言由 `config.json` 中的 `ui.language` 决定（`auto` 表示使用系统语言）。字符串在构建界面时填入，因此更改语言后需要重启。
- 旧的 `ko`、`en` 设置仍然可以识别，也接受 `KO_KR` 这样的写法。如果没有完全匹配的地区，会选择同一语言；
  如果语言不受支持，则选择 `en-us`。在设置中保存时使用 `ko-kr`、`en-us` 这样的新代码。
- 目前仍只有韩语的部分：发给成员的提示词（`res/prompts/ko`）和认真模式的原因文字。这些是给 AI 读的内容，
  应当与界面字符串表分开，扩展为 `prompts/<语言>/`。

## 扩展点

- **新的 CLI/成员**：在 `agent/` 中实现 `AgentBackend` → 在 `AgentHub` 构造函数中注册 → 添加到 `core/Member.cpp` 的列表 →
  `res/avatars/<id>.png`、`res/emoticons/<id>/*.png`（+ `resources.qrc`）。没有图片时会绘制首字母。
- **添加表情**：在 `core/Emoticons.cpp` 的表中加入键、文件和含义，并把 PNG 放进成员的文件夹。
- **修改提示词**：`res/prompts/ko/brief.md`（规则和回复格式）、`PromptBuilder::buildTurn`（情况描述）。
- **新的动作**（例如文件操作）：在 `AgentAction` 中添加字段 → 在 `brief.md` 的回复格式中说明 → 在 `RoomEngine::perform` 中处理。
- **主题**：在 `res/styles/*.json` 中添加令牌，然后在 `base.qss` 中以 `@{令牌}` 使用。
- 尚未支持：图片附件、图像生成、网页搜索开关、提示词多语言、用量标签页。

## 来源

对话引擎（调度、打破沉默、认真模式、提示词）参考了 ai-chatroom（MIT, © 2026 Moris-kr），
界面布局参考了 CodePet（MIT, © 2026 CodePet contributors），并用 C++/Qt 重新编写。
`res/avatars/*.png` 和 `res/emoticons/**` 是取自 CodePet 仓库（MIT）中 `src/chat-assets`、`src/chat-icon/emoticons` 的图片
（仅将文件名改为英文键）。没有使用宠物精灵图。
