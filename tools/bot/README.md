# ゲーム制御サーバ (bot-control)

ループバック TCP 経由で、画面内容の読み取りとキー入力の注入を公開する機能です。プログラム（AI や自動テスト）から
ゲームを操作・観測できます。

バグの再現手順をキー列として記録し、修正後に同じ手順を再生して画面を突き合わせる、といった使い方を想定しています。

**画面は普段通り表示されます。** 手元での操作もこれまで通り可能で、ソケットからの操作と併用できます。
自動操作の様子を画面で観測しながらデバッグできます。画面を出さずに動かしたい場合は
[ヘッドレス実行](#ヘッドレス実行---headless)を使います。

## 起動

```sh
mkdir -p /tmp/hbsave
src/hengband -mgcu --control-port=9000 --fixed-seed=12345 \
    -ds=/tmp/hbsave -uBotTest -n &
```

| オプション                    | 意味                                                                  |
| ----------------------------- | --------------------------------------------------------------------- |
| `--control-port=<port>`       | 127.0.0.1 の待ち受けポート。指定しない場合は起動しない                |
| `--fixed-seed=<seed>`         | 乱数の初期シードを固定する。新規キャラクター作成時のみ効く            |
| `--headless`                  | 画面を持たない端末を使う。`--control-port` が必須                     |
| `--headless-term-count=<num>` | ヘッドレス実行で生成する端末の数 (既定 1、最大 8)                     |

`--fixed-seed` は制御サーバ専用ではなく、どのフロントエンドでも有効です。

### JSONL出力の計測ログ

計測ログは既定では無効です。必要な場合だけ `--bot-json-timing` を追加します。

```sh
src/hengband --bot-json-output=bot-state.jsonl --bot-json-timing
```

JSONLパスに `.timing.log` を付けたファイル（例：`bot-state.jsonl.timing.log`）に、スナップショット種別・ターン・
生成／JSON変換／書込時間（マイクロ秒）・JSONバイト数を記録します。
オプションを付けなければ、既存の計測ログにも触れません。
`--bot-json-timing` 単独ではJSONL出力を有効にしません。
`--bot-json-output=-`（標準出力）や制御サーバの `state` 応答は計測ログの対象外です。
計測ログは10 MiBに達した次の書込時に切り替え、`.1`（最新）～`.3` の3世代を保持します。
古い `.3` は削除されます。行の途中では分割しないため、上限は最大1行分だけ超過します。
現在のログは起動時に空にし、バックアップは残します。ローテーション・書込に失敗した場合は
標準エラーへ通知して計測のみを停止し、JSONL出力とゲームは継続します。
JSONL本体の既存の容量制限・読込方式は変更しません。
異なるJSONLパスを使うプロセスでは計測ログも分離されます。同じJSONLパスの同時使用は避けてください。
計測ログとバックアップにはシンボリックリンク・複数のハードリンクを持つファイル・通常ファイル以外を使えません。
旧名 `emitter-timing-c2.log` は自動移行・削除しません。
JSONLのopenに2回連続で失敗すると出力を停止します。成功した時点で失敗回数をリセットします。

`-ds=<path>` でセーブディレクトリを、`-u<who>` でセーブファイル名を指定します。`-n` を付けると新規キャラクター作成から始まり、
付けなければ既存のセーブデータをロードします。**これらは Unix 版のみです**（[Windows でのセーブファイル指定](#windows-でのセーブファイル指定)を参照）。

待ち受けの開始に失敗した場合（ポートが使用中など）は、理由を標準エラー出力へ出して終了します。
接続を待たずにゲームは始まるため、起動直後の「[ 何かキーを押して下さい ]」から操作できます。

接続後、**30 秒間やり取りが無いクライアントは切断されます**。接続したまま何も送らないクライアントで
待ち受けが塞がり、`quit` すら届かなくなるのを防ぐためのものです。`hbctl.py` は 1 コマンドにつき 1 回接続して
すぐ切断するため、通常の運用で影響を受けることはありません。

### Windows での起動

`--control-port` と `--fixed-seed` は Windows 版でも使えます。診断メッセージ用にコンソールを確保します。

```bat
Hengband.exe --control-port=9000 --fixed-seed=12345
```

Windows 版は起動後、[ファイル] メニューで [新規] または [開く] を選ぶまでキー入力待ちに入りません。
**それまでソケットへの応答は返りません**。ゲームを開始してからクライアントを動かしてください。
（`--headless` を付けた場合はメニューを経由しないため、この制約はありません）

### Windows でのセーブファイル指定

**Windows 版は `-ds=<path>` `-u<who>` `-n` を実装していません。** コマンドライン解釈
（`src/main-win/commandline-win.cpp` の `CommandLine::handle()`）が解釈するのは `-o` `-r`
`--debug-console` `--output-spoilers` と `--` で始まる実行時オプションだけで、**それ以外の `-` で始まる引数は
黙って無視されます**（エラーにもなりません）。元々 [ファイル] メニューの [開く] でセーブファイルを選ぶ設計だったためです。

Windows 版で使えるのは次の 2 つだけです。

| やりたいこと | 方法 |
| --- | --- |
| 既存セーブのロード | `-` で始まらない最初の引数にセーブファイルの**フルパス**を渡す |
| 新規キャラクターの作成 | 位置引数を渡さない。保存先は**キャラクター作成時に入力した名前**で決まる |

```bat
rem 既存のセーブをロードする
Hengband.exe --headless --control-port=9000 lib\save\PLAYER

rem 新規キャラクターから始める (位置引数を渡さない)
Hengband.exe --headless --control-port=9000 --fixed-seed=12345
```

存在しないパスを位置引数に渡すと `validate_file()` が失敗して即座に終了します。接続していたクライアントからは
接続を強制的に切断されたように見えます（`WinError 10054`）。

**新規作成時の保存先は `lib/save/<キャラクター名>` に固定で、ディレクトリは変更できません。**
キャラクター名はキャラクター作成中に入力するため、クライアントから名前を送れば保存先のファイル名は選べますが、
**名前を既定のままにすると常に `lib/save/PLAYER` になります**。Windows で複数のヘッドレスインスタンスを
連続して走らせる場合、キャラクターごとに異なる名前を送らないと同じセーブファイルを共有・上書きします。

なお **Windows 版は多重起動チェック（ミューテックス）があるため、インスタンスを並行して走らせることはできません。**
2 つ目は理由を標準エラー出力へ出して非ゼロで終了します（ミューテックスはプロセスと共に消えるため、
連続して走らせる分には問題ありません）。Unix 版に同種のチェックはなく、ポートとセーブファイルさえ
分ければ並行実行できます。

## ヘッドレス実行 (--headless)

`--headless` を付けると、ウィンドウも端末も開かずに起動します。制御サーバ経由の操作だけでゲームが進むため、
CI や端末を持たない環境での自動テストに使えます。**`--control-port` が必須**で、
無い場合は理由を標準エラー出力へ出して終了します。

```sh
mkdir -p /tmp/hbsave
src/hengband --headless --control-port=9000 --fixed-seed=12345 \
    -ds=/tmp/hbsave -uHeadlessTest -n < /dev/null 2> headless.log &
```

Windows 版も同じオプションで動きます。[ファイル] メニューを経由せず直ちにゲームが始まるため、
位置引数を渡さなければ新規キャラクター作成、渡せばそのセーブファイルのロードになります。

```bat
Hengband.exe --headless --control-port=9000 --fixed-seed=12345
```

**上の例に `-ds=` `-u` `-n` が無いのは書き忘れではありません。** Windows 版はこれらを実装しておらず、
渡しても黙って無視されます。セーブファイルの扱いは
[Windows でのセーブファイル指定](#windows-でのセーブファイル指定)を参照してください。

### 通常の起動との違い

- **クライアントが接続するまで、最初のキー入力待ちで止まります。** ヘッドレス端末は取り出せる
  イベントを持たないため、接続待ちがそのまま入力待ちになります。接続は起動より後で構いません。
  裏返すと、**接続し忘れるとプロセスが残り続けます**。
- **端末の大きさは 80x24 固定です。** `--headless-term-count` で副端末を増やせます
  （`screen` の `term` で読めますが、ウィンドウが無いため通常は主端末だけで足ります）。
- **アニメーションの待ちがありません。** 演出を見せる相手が居ないため、
  ゲームが要求する遅延を無視します。キー列の再生が GUI より速く進みます。
- **フロントエンド固有の `pref-*.prf` を読みません。** `info` の `system` が `headless` になるため、
  `lib/pref/pref.prf` が `$SYS` で分岐して読む `pref-x11.prf` / `pref-gcu.prf` / `pref-win.prf` が
  どれも選ばれず、フロントエンド固有のマクロトリガが定義されません（`pref.prf` 自体と、そこから
  無条件に読まれる `pref-key.prf` `pref-opt.prf` `spell-xx.prf` は通常どおり読まれます）。
  この結果、同じキー列が Unix と Windows で同じ結果になります。
- **診断メッセージは標準エラー出力へ出ます。** ゲーム側は `headless-term:`、
  制御サーバは `bot-control:` の接頭辞が付きます。
  Windows 版はコンソールを確保してそこへ出します。

- **起動直後の「[ 何かキーを押して下さい ]」を表示しません。** キーを押す相手が居ないため、
  Unix・Windows とも待たずにゲームを始めます。GUI で組み立てたキー列を再生する場合は、
  先頭のこの 1 打を取り除いてください。
- **`-s<num>`（ハイスコア表示）と `-m<sys>`（フロントエンド指定）は併用できません。**
  ヘッドレス端末と両立しないため、理由を標準エラー出力へ出して非ゼロで終了します。

## クライアント (hbctl.py)

Python 3.9 以降の標準ライブラリのみで動作します。1 コマンドにつき 1 回接続します。

```sh
python3 tools/bot/hbctl.py info            # 端末情報とビルド情報
python3 tools/bot/hbctl.py screen          # 現在の画面を枠付きで表示
python3 tools/bot/hbctl.py screen --attrs --json  # 色属性込みの生JSON
python3 tools/bot/hbctl.py keys 'jjj'      # キーを送り、処理後の画面を表示
python3 tools/bot/hbctl.py keys '\e' --quiet      # 画面を表示しない
python3 tools/bot/hbctl.py state           # ゲームの内部状態 (JSON)
python3 tools/bot/hbctl.py messages 30     # 直近のメッセージ履歴
python3 tools/bot/hbctl.py replay keys.txt # キー列ファイルを一括投入
python3 tools/bot/hbctl.py raw '{"op":"info"}'
python3 tools/bot/hbctl.py quit            # ゲームを終了
```

`--host` `--port` `--timeout` `--term` は全サブコマンド共通のオプションのため、**サブコマンドより前**に
指定します（`hbctl.py screen --port 9001` のようにサブコマンドの後ろに置くとエラーになります）。

## プロトコル

1 行 1 JSON のリクエストに、1 行 1 JSON のレスポンスを返します。同時に接続できるクライアントは 1 つですが、
切断すれば次の接続を受け付けるため「1 コマンド 1 接続」で操作できます。

リクエストに `id` を入れると、そのままレスポンスに返されます。レスポンスは成功時 `"ok": true` と
`"result"`、失敗時 `"ok": false` と `"error"` を含みます。op ごとの中身は `result` の下に入ります。

```json
{"id": 1, "ok": true, "result": {"term": 0, "width": 80, "height": 24, "lines": ["..."]}}
{"id": 2, "ok": false, "error": "the term index is out of range"}
```

| op         | 主なパラメータ                        | 内容                                       |
| ---------- | ------------------------------------- | ------------------------------------------ |
| `info`     | —                                     | プロトコル版・ビルド情報・端末の一覧       |
| `screen`   | `term` (既定 0), `attrs` (既定 true)  | 画面内容                                   |
| `keys`     | `keys`                                | キー列を注入する。積めたキー数を返す       |
| `state`    | `map` (既定 true)                     | ゲームの内部状態のスナップショット         |
| `messages` | `count` (既定 20)                     | 直近のメッセージ履歴を古い順に返す         |
| `quit`     | —                                     | ゲームを終了する（**セーブしません**）     |

`keys` には `\e`（ESC）、`^X`（Ctrl+X）、`\xNN` といったマクロ表記を使えます。
マクロトリガ表記 `\[～]` は受け付けません。

プロトコル版は `3` です。版 `1` の `nearby_grids` 配列は、版 `2` で
`grid_map`（`palette` / `runs` / `cells` による圧縮地図）へ置き換わりました。
版 `3` の変更点は[版3での変更](#版3での変更)を参照してください。
`info` と各スナップショット（JSONLを含む）の `protocol_version` は共通です。
クライアントはこれを確認し、対応していない版は拒否してください。

`state` の `grid_map`（プレイヤーが記憶している地図）はフロア全域を走査します。
地図が要らない場合は `map` に `false` を指定してください（`hbctl.py state --no-map`）。

### 圧縮地図と情報の範囲（版2・版3）

地図は通常の地図描画で取得できる地形の観測です。トラベル処理が参照する内部地形や、過去の観測の自動補完ではありません。
描画とJSONは地形の表示判定を共有し、モンスターの暗闇・暗視・盲目・記憶表示を同じ条件で扱います。

- `w` / `h`: フロア幅・高さ。座標はゼロ起点の `y, x`。
- `palette`: `[terrain_id, flag_bits, terrain_bits, known]` の配列。
  `known` は記憶または知覚している地形を表す `1` で、現在見えているという意味ではありません。
  地形はMIMIC（見かけ）を使用し、未発見の秘密ドアなどの正体は公開しません。
  壁・扉・階段などの `REMEMBER` 地形は、画面表示と同じく `CAVE_MARK` と壁の表示条件で判定し、視界外でも保持します。
  `CAVE_KNOWN` のみでは地形を追加しません。このフラグは忘却や未観測の地形変更後にも残るため、現在の地形を知っている根拠には使いません。
  非 `REMEMBER` 地形は通常の表示判定に従い、盲目時や記憶・照明等の条件を満たさない場合は省略します。
- `runs`: `[y, x0, length, palette_index]` の配列。同じ行の連続セルのみをまとめます。
  今回取得できない地形は省略します。セル欠落は「今回の地形情報なし」であり、床の消滅・壁への変化・過去の観測の否定ではありません。
  クライアントは過去に受信した地形をフロアごとに記憶し、再観測で更新してください。フロアの切替・再生成をまたいで無条件に混ぜないでください。
  松明が離れた床の経路探索（#5516）は、この観測履歴で対応します。記憶は現在も通れる保証ではありません。
  `grid_map` 自体の欠落は地図未送信です。
- `cells`: `y, x` と追加情報を持つ疎な配列。キーは `m`=モンスターindex、
  `o`=発見済みオブジェクトのスタック数（金貨を含む）、`t`=そのtval配列、
  `s`=店番号、`e`=入口のダンジョンID、`q`=クエストID、`b`=建物種別、`p`=建物special。
  幻覚中の `t` は空配列です。
- `found_items`: `[y, x, stack_count, tval, ...]` の配列。
  地形が未知でも検知済みの品は含み、金貨・未発見品・個数0の品は除外します。
  `stack_count` は品の総個数ではなくスタック数、行長は `3 + stack_count` です。
  幻覚中は数と行長を維持し、tvalを `0`（種別不明）に置き換えます。
- `found_item_names`（版3）: `found_items` と同じ順・同じ品の `[y, x, name, ...]`。
  名前は注視コマンドが表示する名前（未鑑定品は見た目の名前）で、幻覚中は `null` です。
- `unsafe_rows`: 地下かつ罠未調査表示（`view_unsafe_grids`）が有効な場合に高さと同数の16進文字列、それ以外は `null`。
  各行は `ceil(w / 4)` 桁で、x座標は `row[x / 4]` の下位から `x % 4` ビットに対応します。
  端数の上位ビットは0。これは罠そのものではなく `CAVE_UNSAFE`（罠未調査）です。

`flag_bits` は版3から、地図がその地形をどの照明記号で描いているかを表します
（`0`=通常記号、`1`=照明記号、`2`=暗所記号）。同じ設定済みの色と文字になる区分は最小の番号に統合するため、
これは実際の光源や暗闇の状態ではありません。例えば3区分とも同じ見た目のドアは常に `0` です。判定は地図描画と同じ条件で行い、
オプション `view_special_lite` / `view_yellow_lite` / `view_bright_lite` / `view_granite_lite` に従います
（オプションが無効なら常に `0`）。版2までは予約値 `0` でした。
内部の `CAVE_MARK` / `CAVE_KNOWN` / `CAVE_ROOM` / 照明フラグ等そのものは出力しません。
`terrain_bits` は表示対象のMIMIC地形定義から得られる静的特性です。ビット番号は以下の順序で0から割り当てます。

```text
terrain_bits:
  building, can_dig, door, down_stairs, entrance, floor, has_gold, los, move,
  permanent, quest_enter, quest_exit, stairs, store, trap, tunnel, up_stairs, wall
```

`visible_monsters` は `ml && view`、`detected_monsters` は `ml && !view` の集合です。
両方とも自身の `y, x` を持つため、地図を省略した場合や足元の地形が未知でも位置を取得できます。
地図側の `m` はセルとの関連付け用です。幻覚中はモンスターの正体・体力・状態を伏せます。
既存クライアントが地図の `m` のみから位置を復元している場合は、各モンスターの `y, x` も利用してください。

店内スナップショットは地図を省略します。商品は店頭で公開される種類・特性を出力しますが、
自宅・博物館・所持品は通常の鑑定状態に従います。自分で付けた銘は未鑑定品でも出力します。
`player.can_see_own_grid` と `light_radius` は視認性と光源半径、`floor.feeling` は公開済みの階の雰囲気です。

`keys` に `term` はありません。キーを消費するのは常に現在の端末のため、副端末を指定する意味がないからです。

### 版3での変更

版3は「人間が画面で認識できる情報を過不足なく出す」ための改訂です。画面に出ていない値（逆方向の漏れ）は、
画面の表示と同じ形へ置き換えました。表示オプション `show_actual_value`（数値表示）が有効な場合だけ出る
数値は、その時点のオプションの値に従って出力します（無効なら出力しない、または `null`）。

**例外（ユーザー決定）**: `turn`（ゲームターン）は画面に出ませんが、ボットが判断の区切りとログの突き合わせだけに
使うため、フェアプレイ規則の例外として残します。人間が見られる日付・時刻は `clock` に出します。

#### 追加したキー

常時（`player_turn` ほか全スナップショット）:

| キー | 内容（出所となる画面） |
| --- | --- |
| `clock` | `{day, hour, minute}`。画面右下の日付・時刻。`day` は画面が `***` を出す1000日目以降 `null` |
| `health_bar` | 画面左の体力ゲージ（追跡中のモンスター）。追跡なしは `null`。見えない・幻覚中・死亡時は `{known: false}` だけ。見えていれば `{known: true, index, length(1-10), color, conditions[]}`。`conditions` は `invulnerable` / `fast` / `slow` / `afraid` / `confused` / `asleep` / `stunned` のうち表示中のもの |
| `riding_health_bar` | 騎乗中のモンスターの体力ゲージ。形は `health_bar` と同じで `conditions` なし |
| `floor.dungeon_name` | 画面右下の地名（町・荒野・ダンジョン・クエスト） |
| `player.max_exp` / `player.exp_drained` | 最大経験値と、経験値減少表示（`x経験`・黄色）の有無 |
| `player.max_level` / `player.level_drained` | 最高到達レベルと、レベル減少表示（`xレベル`・黄色）の有無 |
| `player.exp_to_advance` | 次のレベルに必要な経験値（キャラクター画面の値、最高レベルでは `null`） |
| `player.title` / `player.race_title` / `player.mimic_form` | 称号（ウィザード・勝利者を含む）、種族欄の名前（変身中は変身先）、変身の種類（0=なし） |
| `player.speed_display` | 速度欄 `{value, text, color, riding}`。色は一時的な加速（黄）・減速（紫）・騎乗中のモンスターの加減速等 |
| `player.action` | 行動状態欄 `{text, color, kind, repeat_count?, rest?}`。`kind` は `repeat` / `none` / `search` / `rest` / `learn` / `fish` / `monk_stance` / `samurai_stance` / `sing` / `hayagake` / `spell`。`rest` は残りターン数・`full_healing`・`until_done` |
| `player.study` | 「学習」「まね」表示。`null` または `{kind: "study"\|"imitation", new}` |
| `player.status_bar` | 画面下部の状態表示の全項目 `[{id, key, label}]`（表示順）。一時耐性・免疫・テレパシー・透明視・士気高揚・狂戦士化・祝福・対邪悪結界・無敵・幽体化・影分身・魔法の鎧・石肌・究極の耐性・魔法防御・壁抜け・反射・浮遊・急回復・赤外線視力・隠密・オーラ・魔法剣・つよし・変わり身・現実変容・呪術の効果等 |
| `player.status.cut_rank` / `cut_rank_name` | 負傷の段階（`graze`〜`mortal_wound`） |
| `player.status.stun_rank` / `stun_rank_name` | 朦朧の段階（`slight`〜`knocked_out`） |
| `player.stats.<stat>.top` | キャラクター画面の「合計」（減少していない場合の値） |
| `player.stats.<stat>.at_racial_max` | 能力値名の横の `!`（種族上限に到達） |
| `player.ability_sources.resist_time` / `resist_water` / `resist_curse` | 時間逆転・水・呪力の耐性の供給源 |
| `player.skill_ratings` | キャラクター画面の技能評価（下記） |
| `visible_monsters[]` / `detected_monsters[]` / `look.grids[].monster` の `level` | モンスターのレベル（その種族を倒したことがある場合だけ。未撃破・影は `null`） |
| 同上の `fast` / `slow` / `invulnerable` | 加速・減速・無敵（体力ゲージの状態欄と同じ判定） |
| `grid_map.found_item_names` | 前述 |
| 所持品・装備・店の品の `charging` / `charging_count` / `light_turns` | 下記「変更・削除」 |
| 所持品等の `weapon_proficiency_rank` | 武器の熟練度の段階（`~d` の一覧の表示） |

`player.skill_ratings` は `fighting` / `shooting` / `saving_throw` / `stealth` / `perception` / `searching` /
`disarming` / `magic_device` / `digging` の各キーに `{text, color, rating, legendary_level?, value?}` を持ちます。
`rating` は `very_bad` / `bad` / `poor` / `fair` / `good` / `very_good` / `excellent` / `superb` / `heroic` / `legendary`、
`value` は `show_actual_value` が有効な場合だけ画面に出る数値（画面の計算と同じく武器・弓の命中修正込み）です。

`look`（注視コマンド）: モンスターに `clone`（クローン表記）、`kills_to_level`（レベルアップまでに倒す数の表示。`**` / `??` / 3桁）、
`description`（`レベル N, 損傷具合, 態度, clone` の表示文字列）を追加しました。

`store`（店）: 店では `owner_name` / `owner_race` / `store_name` / `max_cost`（買取上限）、自宅・博物館では `capacity`（`アイテム数: n/容量` の容量）を追加しました。

`character`（`C` コマンド）: `skill_ratings`、`name`、`sex`、`race_title`、`class_title`、`personality_title`、`history`（生い立ち4行）、
`displayed_melee`（手ごとの `{hand, label, to_h, to_d}`。画面の表示値）、`displayed_shooting`（`{to_h, to_d}`）、`base_ac` / `ac_bonus`（`[基本AC, +修正]`）、
`speed`（`{base, temporary, lightspeed, riding}`。画面が「光速化 (+99)」だけを表示する間は `base:null`。乗馬時は `lightspeed:false`）、`exp`（`{current, max, to_advance}`。アンドロイドの `max` は `null`）、`day` / `hour` / `minute`、
`play_time`（実プレイ時間の表示文字列）、`stat_modifiers`（能力修正欄。能力値ごとに装備部位ごとの `{slot, symbol, color}` と本人の列 `player`）、
`curse_marks`（特性画面の呪い欄。部位ごとに `+` / `*` / `.`）、`alignment_label` / `alignment_value` を追加しました。

`knowledge`（`~` コマンド）:

- 新しい分類 `monsters`（メニューキー `6`）: 見たことのある種族の一覧 `monsters[]`（`{id, name, kills}`、ユニークは `{id, name, dead}`）。
- 新しい分類 `kill_count`（メニューキー `7`）: `total` と `kills[]`（ユニークは `{id, name, unique: true, defeat_level?, defeat_time?}`、それ以外は `{id, name, unique: false, kills}`）。
- `uniques_dead` の各行に `defeat_level` / `defeat_time`（撃破時のレベルと実時間）を追加しました。
- `weapon_exp` / `skill_exp` / `spell_exp` の各行に `at_max`（一覧の `!`）を追加しました。
- `virtues` に `alignment_label` / `alignment_value` と、各行の `text`（画面の文）を追加しました。

新しいスナップショットの種類（いずれも地図なし）:

| `type` | 出力する時点と内容 |
| --- | --- |
| `spell_list` | 操作メニューで魔法書の呪文一覧を表示したとき（閲覧・詠唱・学習）。サブウィンドウの再描画では出力しない。`spell_list: {realm_id, spells[]}`。各行は `{spell_id, status, name, level, mana, fail?, proficiency?, proficiency_mark?, info?}`。`status` は `available` / `untried` / `unknown` / `forgotten` / `illegible`。必殺剣は画面に熟練度・失敗率・効果を出さないので `fail` 以降が無い |
| `power_list` | 特殊能力の一覧を表示したとき。`power_list: {kind, page, browse_mode, powers[]}`。`kind` は `racial`（種族・職業・突然変異のパワー）または `mind`（超能力・練気術・狂戦士・鏡使い・忍術）。各行は `{letter, page, name, level, cost, fail, info}`。`cost` は画面の MP / HP 欄の値 |
| `lore` | モンスターの思い出を表示したとき（注視の `r`、`/`、`~6`、ギルド）。`lore: {race_id, name, text}`。`text` は画面に書かれる思い出の全文 |

#### 変更したキー

| キー | 変更内容 |
| --- | --- |
| `grid_map.palette[][1]`（`flag_bits`） | 予約値 `0` → 地形の照明記号番号（`0` 通常記号 / `1` 照明記号 / `2` 暗所記号）。同じ見た目の区分は統合し、実際の照明状態は表さない。単色表示では文字だけで比較 |
| `messages`（JSONL） | 新着が32件を超えても切り捨てない。新着の数え方を履歴の件数から追加行の累計に変更（履歴が上限に達しても取りこぼさない） |
| 所持品等の `weapon_proficiency` | `show_actual_value` が有効な場合だけ出力（常時の値は `weapon_proficiency_rank`） |
| `knowledge` の `weapon_exp[].exp` / `max` | `show_actual_value` が有効な場合だけ出力 |
| `knowledge` の `skill_exp[].exp` | `show_actual_value` が有効な場合だけ出力し、画面と同じく上限で切り詰める。`max` と `rank` を追加 |
| `knowledge` の `spell_exp[].exp` | `show_actual_value` が有効な場合だけ出力。`max`（達人の値）を追加。`masked` は画面と同じく第1領域の必殺剣だけ |
| `knowledge` の `weapon_exp` / `skill_exp` / `spell_exp` | `cheat_xtra` が有効な場合は、一覧末尾の生の経験値を別の `debug_exp` に出力。通常の `exp` の上限処理や必殺剣のマスクとは独立 |
| `player.melee.main_hand_to_h` / `sub_hand_to_h` / `main_hand_to_d` / `sub_hand_to_d` | 内部補正からキャラクター画面に出る最終補正へ変更。攻撃できない手は `null` |
| `character.ranged.to_h_b` | 内部の射撃補正から、`displayed_shooting.to_h` と同じ画面の最終補正へ変更 |
| `player.melee.*_hand_blows` | 攻撃できない手は `0`。画面の追加攻撃回数を `mutation_blows` に出力 |
| `character.melee.expected_damage_x100` / `expected_damage_per_round_x100` | 削除。画面と同じラウンド平均ダメージの整数2個を `expected_damage_per_round` に出力（`nil!` 表示なら `null`）。`damage_nil` と攻撃回数3個の `blows` を追加 |
| `knowledge` の `virtues[].value` | `show_actual_value` が無効なら `null` |
| `knowledge` の `uniques_alive` / `uniques_dead` | チートオプション `cheat_know` が有効なら未見の種族も含める（画面と同じ） |

#### 削除したキー

| キー | 理由と代わり |
| --- | --- |
| 所持品等の `timeout` | 残りターン数は表示されない。`charging`（`(充填中)` の有無）と、ロッドでは `charging_count`（`(N本 充填中)` の本数、1本なら1） |
| 所持品等の `fuel` | 生の燃料は表示されない。`light_turns`（`(Nターンの寿命)` の値。長寿命のエゴは2倍。アーティファクト等の寿命を表記しない光源には無い） |
| `player.skills`（`melee` / `shooting` / `saving` / `device` / `stealth` / `two_weapon` / `shield`） | 生の技能値は表示されない。`player.skill_ratings`。二刀流・盾の熟練は `~f`（`knowledge` の `skill_exp`） |
| `player.stats.<stat>.cur` | 生の能力値は表示されず、表示値から一意に復元もできない（負の修正で18/xxが丸められ、3で下げ止まる）。表示されるのは `use`・`top`・`max` |
| `character.skills`（`thn` / `thb` / `sav` / `dev` / `stl` / `dis` / `srh` / `fos` / `dig`） | `character.skill_ratings` |
| `character.alignment` | 画面は属性名と、`show_actual_value` が有効な場合だけ数値を出す。`alignment_label` / `alignment_value` |

#### Python クライアントが対応すべきこと

1. `protocol_version` が `3` のスナップショットを受け付ける（`2` は拒否）。削除したキーを `get(key, 0)` のように既定値付きで読んでいると、例外にならず `0` として誤読する（燃料0の松明・技能0等）ので、既定値を外して欠落を検出すること。
2. `inventory[]` / `equipment[]` / `store.items[]` / `knowledge` の品の `timeout` を読む箇所を `charging`（真偽）と `charging_count`（ロッドの本数）へ置き換える。
3. 同じく `fuel` を読む箇所（松明・ランタンの残量判定、燃料0の松明の除外）を `light_turns` へ置き換える。長寿命のエゴは2倍の値になり、寿命を表記しない光源（アーティファクト等）はキー自体が無い。
4. `player.skills.*` を読む箇所を `player.skill_ratings.<key>.rating`（数値が要る判定は `value`。ただし `show_actual_value` 無効時は無い）へ置き換える。`two_weapon` / `shield` の熟練は `~f` の `knowledge.skills[]` から得る（数値は `show_actual_value` 有効時のみ）。
5. `player.stats.<stat>.cur` を読む箇所を `use` / `top` / `max` / `drained` へ置き換える。
6. `character.skills.*` を `character.skill_ratings.*` へ、`character.alignment` を `alignment_label` / `alignment_value` へ置き換える。
7. 所持品等の `weapon_proficiency` が常にある前提をやめ、`weapon_proficiency_rank` を使う。
8. `knowledge` の `weapon_exp` / `skill_exp` / `spell_exp` の `exp` / `max`、`virtues[].value` が欠ける・`null` になる場合に対応する。
9. `grid_map.palette` の2番目の値を `CAVE_*` のビット列（`mark` / `cave_known` / `lite` / `view` …）として解読している箇所をやめ、表示上の照明記号番号（`0` / `1` / `2`）として読む。同じ見た目の区分は統合されるので実際の照明状態は復元できない。版2までは常に `0` だったので解読結果は全て偽だったが、版3のまま解読すると照明記号が `mark`、暗所記号が `cave_known` と誤読される。
10. JSONLの `messages` が1スナップショットで33件以上になり得る。
11. 新しい `type`（`spell_list` / `power_list` / `lore`）と `knowledge` の分類（`monsters` / `kill_count`）を、未対応なら無視できるようにする（`player_turn` と取り違えない）。
12. `player.melee` の命中・ダメージ修正と `character.ranged.to_h_b` は画面の最終値なので、武器修正や技能修正を再加算しない。攻撃できない手の `null` と、光速化中の `character.speed.base:null` を扱う。打撃回数は攻撃できない手が `0`。平均ダメージの旧 `*_x100` キーを廃止し、整数の `expected_damage_per_round` を読む（`damage_nil:true` なら `null`）。`blows` の3番目は突然変異の攻撃回数。

### quit はセーブしません

**`quit` はゲームを保存せずにプロセスを終えます。** 長時間の自動プレイの進行は失われます。
保存して終了したい場合は、ゲーム内の「セーブして終了」のキー（`^X`）を送り、
その後に出る「リターンキーか ESC キーを押して下さい。」へ ESC を送ってください。

```sh
python3 tools/bot/hbctl.py keys '^X' --quiet
python3 tools/bot/hbctl.py keys '\e' --quiet
```

ESC 以外を送るとスコアの予測表示に進んでしまい、そのままでは終了しません。
`^X` の直後の画面はセーブが終わるまで戻らないため、2 つのキーは別のリクエストで送ります。

### 応答されるタイミング

**リクエストが処理されるのは、ゲームがキー入力を待っている間だけです。** これにより、
返る画面は必ず描画が確定した状態のものになります。その代わり、長い処理やアニメーションの最中は
応答が待たされます。

### 端末の大きさ

端末の数と大きさはフロントエンドが決めます。**80x24 とは限りません。** 画面を読む前に `info` の
`width` / `height` を確認してください。`screen` の `term` にはサブウィンドウの添字も指定できます
（生成されていない添字はエラーになります）。

`lines[y]` は 1 行分のセルを連結して UTF-8 に変換した文字列、`attrs[y]` は 1 セルあたり 16 進 2 桁で
属性を並べた文字列です。日本語版では全角 1 文字が 2 セルを占めるため、`lines[y]` の文字数と
`attrs[y]` の長さ（セル数×2）は一致しません。**`attrs` 側の添字がセル座標 (x) に対応します。**
全角文字のセルの属性には色以外のビットが乗るため、色だけが必要な場合は `0x0f` との論理積を取ってください。

### 一度に注入できるキーの数

キーキューの大きさはフロントエンドが決めます（x11 のメイン端末とヘッドレスは 1023、gcu と cap は 255、
Windows 版はより大きい）。溢れる場合は 1 つも積まずにエラーを返すため、キー列の一部だけが
実行されることはありません。長い手順は `replay` のように複数回に分けて投入してください。

## 既知の制限

- gcu 日本語版では、ノンブロッキング経路がマルチバイト入力の変換を行いません。
  そのため `--control-port` を指定している間は、端末から日本語を直接入力しても正しく扱われません
  （ソケットからのキー注入には影響しません）。ヘッドレス実行では端末からの入力自体が無いため関係ありません。
