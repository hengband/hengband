# hengband-mcp

LLM のエージェントが変愚蛮怒を遊べるようにする [MCP](https://modelcontextprotocol.io/) サーバです。
ゲームの[制御サーバ](../bot/README.md)（`--control-port`）を通してキー入力を送り、画面とゲームの状態を
LLM が読みやすい形にまとめて返します。

```text
MCP クライアント（Claude Code など）
   │ MCP（stdio）
   ▼
hengband-mcp（Python）
   │ 制御サーバのプロトコル（JSON、TCP 127.0.0.1）
   ▼
変愚蛮怒（--headless --control-port=<空いているポート> で起動する）
```

## 必要なもの

- Python 3.10 以上と [uv](https://docs.astral.sh/uv/)
- ビルドした変愚蛮怒。既定では、このリポジトリのトップの `Hengband.exe`（Windows）か `src/hengband`（Unix 系）を使います。
  制御サーバの `input` op を使うため、それに対応した版が要ります。
- Windows では `lib/xtra` サブモジュール（ゲームの起動に必要）

## セットアップ

```bash
cd tools/hengband-mcp
uv sync
```

## MCP クライアントへの登録

Claude Code の例です（パスはリポジトリの場所に合わせてください。Windows でも `/` で区切れます）。

```bash
claude mcp add hengband -- uv run --directory /path/to/hengband/tools/hengband-mcp hengband-mcp
```

設定ファイル（Claude Code の `.mcp.json`、Claude Desktop の `claude_desktop_config.json` など）に書く場合:

```json
{
  "mcpServers": {
    "hengband": {
      "command": "uv",
      "args": ["run", "--directory", "/path/to/hengband/tools/hengband-mcp", "hengband-mcp"]
    }
  }
}
```

### オプション

`hengband-mcp` の後に付けます。

| オプション | 内容 |
| --- | --- |
| `--game <path>` | ゲームの実行ファイル。省略すると環境変数 `HENGBAND_GAME`、それも無ければこのリポジトリでビルドしたもの |
| `--instances-dir <dir>` | インスタンスを置くディレクトリ。既定は Windows が `%LOCALAPPDATA%\hengband-mcp\instances`、Unix 系が `$XDG_STATE_HOME/hengband-mcp/instances`（`XDG_STATE_HOME` が無ければ `~/.local/state/hengband-mcp/instances`） |
| `--instance <name>` | 使うインスタンスの名前（既定: `default`）。セーブファイルや設定はインスタンスごとに分かれる |
| `--log-level <level>` | 標準エラー出力に出すログの詳しさ（`DEBUG`、`INFO`（既定）、`WARNING`、`ERROR`） |

## ツール

| ツール | 内容 |
| --- | --- |
| `start_game` | ゲームを起動して、最初の観測を返す。`savefile` を省くと新しいキャラクターを作る（キャラクター作成の画面から始まる）。`display="window"` にするといつものウィンドウを表示して観戦できる（Windows のみ。既定はヘッドレス）。`seed` で乱数の種を固定し、`allow_debug` でデバッグコマンドを許可する |
| `send_keys` | キーを送り、結果と新しい観測を返す |
| `observe` | キーを送らずに観測を返す |
| `get_screen` | 画面全体をテキストで返す |
| `stop_game` | セーブして終了する（`save=false` ならセーブせずに終了する） |
| `list_saves` | インスタンスのセーブファイルを新しい順に返す |

ツールの説明と MCP の instructions（観測の読み方、キーの書き方、主なコマンド、注意点）は LLM が読むもので、
英語で書いています（`src/hengband_mcp/server.py`）。ゲームのメッセージやモンスターの名前は、日本語版なら日本語のまま返します。

### 観測

ツールの結果は 2 つのブロックから成ります。1 つ目は JSON の要約、2 つ目は画面のテキストです。

```json
{"outcome":"disturbed","consumed":".6","remaining":"R<ENTER>","input":"command","messages":[],
 "player":{"level":1,"hp":[43,43],"mp":[0,0],"exp":1,"exp_to_advance":10,"gold":100,"ac":14,"speed":0,
  "food":"normal","light":1,"place":"イークの洞穴","depth":1,"pos":{"x":52,"y":20},"clock":"day 1 06:08"},
 "monsters":[{"name":"ジャッカル","dir":"E(6)","dist":4,"dx":4,"dy":0,"health":"unhurt"}],"view":"map"}
```

| キー | 内容 |
| --- | --- |
| `outcome` | `send_keys` の結果。`done`（すべて送った）、`disturbed`（途中で中断された。[キーの送り方](#キーの送り方)）、`timeout`（ゲームがまだ処理している）、`game_over`（キャラクターが死んだ） |
| `consumed` / `remaining` | 送ったキーと、送らなかったキー |
| `more_dismissed` | 自動で閉じた `-more-` の数 |
| `input` | ゲームが待っている入力（制御サーバの [`input`](../bot/README.md#入力待ちの種類-input) の `kind`） |
| `prompt` | コマンド以外の入力を待っているときの、画面の 1 行目（問いかけ） |
| `messages` | 前回の呼び出しより後のメッセージ（古い順で最大 50 件。超えた分の数は `messages_omitted`）。`start_game` では直前の 10 件 |
| `player` | レベル、HP と MP（`[現在値, 最大値]`）、経験値、所持金、AC、速度、満腹度、光源の半径、場所と階（`depth`。0 は地上）、位置、状態異常（`status`）、ステータスバーの効果（`effects`）、続けている行動（`action`）、時刻 |
| `monsters` | 見えているモンスター（近い順に最大 15 体）。`dir` はそのモンスターに向かう方向とその数字キー、`dist` は距離、`dx` と `dy` は相対位置（東と南が正） |
| `detected_monsters` | 感知しているが見えていないモンスター（最大 10 体） |
| `inventory` / `equipment` | 引数で `inventory=true` を指定したときの持ち物と装備 |
| `view` | 2 つ目のブロックの内容。`map`（マップの部分だけ）か `screen`（画面全体） |

2 つ目のブロックは引数の `view` で選べます。既定の `auto` は、コマンドの入力待ちならマップ、
それ以外（メニュー、プロンプト、キャラクター作成など）なら画面全体です。`none` にすると省きます。

### キーの書き方

- 普通の文字はそのまま送ります。日本語版では日本語（名前や銘）もそのまま書けます（ゲームの文字コードに変換して送る）。
- 特殊なキーは名前で書きます: `<ESC>`、`<ENTER>`、`<TAB>`、`<BS>`、`<SPACE>`、`<C-x>`（Ctrl+x）。
- 名前に一致しない `<` は、そのまま `<`（上り階段）として送ります。名前に見えてしまう場合は `<LT>`、`<GT>` と書きます。
  `<ENTER` のように `>` を閉じ忘れたものや知らない名前はエラーにします（残りを 1 文字ずつコマンドとして送ってしまわないため）。
- 1 回に送れるのは 256 キーまでです。
- `<C-x>`（セーブして終了）は送らずにエラーにします。終了には `stop_game` を使います。
  `<C-a>`（デバッグコマンド）と `<C-w>`（ウィザードモード）は、`allow_debug=true` で起動したときだけ送れます。

### キーの送り方

- キーは 1 つずつ送り、ゲームがそのキーを処理して次の入力待ちになってから、次のキーを送ります。
- `-more-` は自動で閉じます（ESC を送るので、そのターンの後の `-more-` も出なくなります）。メッセージは `messages` で読めます。
  死んだときの `-more-` は閉じません。
- `stop_on_disturb`（既定 true）: コマンドの入力待ちに戻ったとき、ゲームがプレイヤーの行動を中断させていた（`disturb()`）なら、
  残りのキーがあってもそれを送らずに `outcome: "disturbed"` を返します。モンスターが現れた、ダメージを受けた、壁にぶつかったときのほか、
  **走る・休むが終わったときもゲームは中断として扱う**ため、`.6.4` のように走るコマンドを続けると 1 つ目の後で止まります。
- 1 回の `send_keys` は 60 秒で打ち切り、`outcome: "timeout"` を返します。ゲームはそのまま動いているので、`observe` で確かめます。
  制御サーバはゲームがキー入力を待っている間しか応答しないため、時間のかかる処理（条件の厳しいオートローラーなど）の途中でも
  `timeout` になります。

## インスタンス

ゲームは、普段遊ぶ環境から切り離した「インスタンス」のディレクトリで動かします。セーブファイル、スコア、設定はインスタンスごとに分かれ、
手元で遊んでいる変愚蛮怒のデータには触れません。

- Windows 版は実行ファイルの隣の `lib` に書き込むため、実行ファイルと `lib` の読み込み専用の部分（`edit`、`file`、`help`、`pref`、`xtra`）を
  インスタンスに複製して起動します。起動のたびに、変わったファイルだけを複製し直します。
- Unix 系は `-d<what>=<path>` で書き込み先をインスタンスに向け、`lib` はリポジトリのものを読みます。
- ゲームの標準出力と標準エラー出力は、インスタンスの `game.log` に書きます。
- 起動のたびにインスタンスの `lib/user/user.prf` を書き直し、次のオプションを設定します（セーブファイルを読んだ後に読まれるため、
  セーブファイルに記録された設定より優先されます）。
  - `send_score` を無効にする（スコアを送信しない）
  - `view_unsafe_grids` を無効にする（罠を感知していない場所を `x` で埋めず、マップの未知の場所を空白のままにする）
  - `rogue_like_commands` を無効にする（instructions はオリジナルのキー配置で説明しているため）
  - `allow_debug_opts` を `start_game` の `allow_debug` に合わせる

## 制限と注意

- 1 つのサーバで同時に動かせるゲームは 1 つです。Windows 版は多重起動できないため、PC 全体でも 1 つです。
  手元で変愚蛮怒を遊んでいる間は `start_game` が失敗します。
- MCP クライアントを終える前に `stop_game` を呼んでください。サーバは終了するときに動いているゲームをセーブして終了させようとしますが、
  クライアントは数秒でサーバを強制終了することがあり（MCP の Python SDK の stdio クライアントは 2 秒）、その場合は最後にセーブしたところまで戻ります。
- Windows では、ヘッドレスのゲームはサーバと一緒に終わります（ジョブオブジェクト）。ウィンドウを表示したゲームは、
  サーバが異常終了すると残ることがあります。
- ウィンドウを表示した Windows 版は壁や鉱脈を塗りつぶした四角（0x7F）で描くため、観測では `#` に置き換えています。
- 死んだキャラクターのセーブファイルも `list_saves` に残りますが、続きは遊べません。

## テスト

```bash
cd tools/hengband-mcp
uv run pytest
```

テストは偽の制御サーバとプロセスを使い、ゲームは起動しません。実際のゲームでの確認には MCP クライアントから遊ぶか、
制御サーバのクライアント（[`tools/bot/hbctl.py`](../bot/README.md)）を使います。
制御サーバは同時に 1 つのクライアントしか受け付けません。MCP サーバはツールを呼び出している間だけ制御サーバに接続するため、
その間は `hbctl.py` が待たされます。
