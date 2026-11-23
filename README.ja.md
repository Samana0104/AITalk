# AITalk

[한국어](README.md) | [English](README.en.md) | **日本語** | [简体中文](README.zh-CN.md)

Claude・ChatGPT・Gemini の 3 人が常駐する**デスクトップ AI グループチャット**です（C++20 / Qt 6 Quick）。

- 会話の仕組みは [ai-chatroom](https://github.com/Moris-kr/ai-chatroom) から取り入れています。メンバーごとに個別に
  スケジューリングされ、新しいメッセージを数秒「読んで」から発言するかパスし、静かになると 1 人が沈黙を破り、
  ホストが長く発言しないとルームが眠ります。
- UI 構成は [CodePet](https://github.com/nokryong/CodePet) のエージェントチャットルームから取り入れています。
  フレームレスウィンドウ、セッションサイドバー（検索・名前変更・削除）、参加者チップ、引用返信、@メンションポップアップ、
  環境診断、設定、トレイ。メンバーの SD キャラクターアイコンとキャラクタースタンプも CodePet のものを使っています（ペットは除く）。
- Web サーバーも API キーも不要です。PC でログイン済みの各社 CLI（`claude`、`codex`、`agy`）を、ヘッドレスかつ
  ツールをすべて無効にした状態で呼び出します。CLI がないメンバーは「CLI が見つかりません」と表示されます。

## ビルド

必要なもの：Qt 6.8 以上（Quick、Widgets、Svg）、CMake 3.21 以上、C++20 コンパイラ。

```bash
# 例：Windows + Qt オンラインインストーラー（MinGW）
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=C:/Qt/6.11.2/mingw_64 -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Qt Creator では `CMakeLists.txt` を開くだけです。

配布版（`dist/`）は Release ビルドからのみ作成します。Debug ビルドのディレクトリからインストールするとエラーで停止します。

```bash
cmake -S . -B build-release -G Ninja -DCMAKE_PREFIX_PATH=C:/Qt/6.11.2/mingw_64 -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
cmake --install build-release
```

実行に必要なファイル（exe・DLL・プラグイン・QML モジュール・config）だけが `dist/` に集められます。他の PC では
このフォルダーをそのまま渡してください。実行中の `dist/AITalk.exe` は上書きできないため、インストール前にトレイも含めて
アプリを完全に終了してください。

ポータブル方式のため、設定と会話履歴はどちらも実行ファイルの隣に保存され、フォルダーを削除するだけでアプリが
きれいに消えます。書き込み権限のない場所（`C:\Program Files` など）には置かないでください。

- `config/{config.json, ui.ini}`：設定。ビルド・インストール時にプロジェクトの `config/` がコピーされ、既存のファイルは
  上書きされません。既定の設定値を変えるには、プロジェクトの `config/config.json` を編集します。
- `data/sessions/<id>/{session.json, messages.jsonl, notes/, logs/}`：会話履歴。`data/trash/` は削除したセッションです。

各 CLI が実際に何を受け取り何を返したかは `logs/<メンバー>.log` で確認できます。

## 会話の流れ

1. ホストがメッセージを送ると（`RoomEngine::sendUserMessage`）ルームがオンになり、`messages.jsonl` に追記されます。
2. `RoomEngine::tick()` が 1 秒ごとに動きます。新しいメッセージを見たメンバーは速度プリセットの分だけ待ってから
   （@メンション・返信・ホストのメッセージは 1〜3 秒）ターンを始めます。同時に考えるメンバー数（`maxInFlight`）、
   1 分あたりの吹き出し数、メンバーごとのクールダウンでにぎやかさを調整します。
3. 各ターンは独立した呼び出しです。`PromptBuilder` が `brief`（ルームのルール・応答形式）と `turn`（個人メモ・
   直近 40 件の会話・状況）を作り、`AgentHub` が該当する CLI を実行します。
4. 応答から JSON アクション（`say`/`pass`、`messages`、`reply_to`、`react`、`emoticon`、`note_add`、`note_replace`、`boost`）を
   取り出してリアクション・メモを反映し、吹き出しを「入力中」の時間をおいて 1 つずつ投稿します。
   `emoticon` を選ぶと、吹き出しの後にキャラクタースタンプが画像で投稿されます（1 ターンに最大 1 つ）。考えている間に
   会話が大きく進んでいたら、出遅れた応答は捨てます。
5. しばらく静かだと、いちばん長く発言していないメンバー **1 人** を選んで沈黙を破らせます（spark）。パスすると
   次の待ち時間が長くなります。
6. **本気モード（boost）**：ホストが「真剣に」と言ったり、`/boost @Gemini` を送ったり、コードや計算を頼んだりすると、
   そのターンだけより強力なモデルで実行します。メンバーが応答に `"boost"` を入れて自分でオンにすることもできます。

## 多言語対応（Text）

UI の文字列はコードに直接書かず、キーで呼び出します（Unreal の `LOCTEXT` のように）。

```cpp
#include "manager/DataManager.h"

label->setText(Text("sessions.new"));                 // 「새 채팅」/「New chat」
showToast(Text("toast.exported", path));              // 値の %1 に path
Text("doctor.summary", found, total);                 // %1、%2 の順に
```

- 表は `res/locale/<言語-地域>.csv` で、ヘッダー行は `key,value` です。ファイル名が言語コードで、`lang.name` の値が
  設定 → 表示 → 言語に表示される名前です。英語（`en-us`）、韓国語（`ko-kr`）、簡体字中国語（`zh-cn`）、
  日本語（`ja-jp`）を提供しています。
- 新しい言語は既存の CSV をコピーして値を翻訳し、`res/locale/locale.qrc` に登録すると言語一覧に自動で追加されます。
  すべての言語で同じキーと `%1`、`%2` のプレースホルダーを保ってください。
- 値の中の改行は `\n`。カンマや引用符を含む場合は `"..."` で囲み、引用符は `""` と書きます。
- 検索順：現在の言語 → `ko-kr`（代替言語）→ キーそのまま（警告は 1 回）。空の値は未翻訳とみなし `ko-kr` を使います。
- 言語は `config.json` の `ui.language`（`auto` なら OS の言語）です。画面を作るときに文字列が入るため、変更したら
  再起動が必要です。
- 以前の `ko`・`en` の設定も引き続き認識し、`KO_KR` のような表記も受け付けます。正確な地域がなければ同じ言語を、
  未対応の言語なら `en-us` を選びます。設定を保存するときは `ko-kr`・`en-us` のような新しいコードを使います。
- まだ韓国語のみのもの：メンバーに渡すプロンプト（`res/prompts/ko`）と本気モードの理由の文言。これは AI が読む文章なので、
  UI の表とは別に `prompts/<言語>/` として増やすのが適切です。

## 拡張ポイント

- **新しい CLI/メンバー**：`agent/` に `AgentBackend` を実装 → `AgentHub` のコンストラクタに登録 → `core/Member.cpp` の
  一覧に追加 → `res/avatars/<id>.png`、`res/emoticons/<id>/*.png`（+ `resources.qrc`）。画像がなければイニシャルで描きます。
- **スタンプの追加**：`core/Emoticons.cpp` の表にキー・ファイル・意味を入れ、メンバーのフォルダーに PNG を置きます。
- **プロンプトの修正**：`res/prompts/ko/brief.md`（ルール・応答形式）、`PromptBuilder::buildTurn`（状況の文言）。
- **新しいアクション**（例：ファイル操作）：`AgentAction` にフィールドを追加 → `brief.md` の応答形式に説明を追加 →
  `RoomEngine::perform` で処理。
- **テーマ**：`res/styles/*.json` にトークンを追加し、`base.qss` で `@{トークン}` として使います。
- まだないもの：写真の添付、画像生成、Web 検索の切り替え、プロンプトの多言語化、使用量タブ。

## 出典

会話エンジン（スケジューリング・沈黙破り・本気モード・プロンプト）は ai-chatroom（MIT, © 2026 Moris-kr）を、
UI 構成は CodePet（MIT, © 2026 CodePet contributors）を参考に、C++/Qt で新たに書き直しました。
`res/avatars/*.png`、`res/emoticons/**` は CodePet リポジトリ（MIT）の `src/chat-assets`、`src/chat-icon/emoticons` から
持ってきた画像です（ファイル名だけ英語のキーに変更）。ペットのスプライトは使っていません。
