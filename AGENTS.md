# AGENTS.md

このファイルは、このリポジトリで作業する AI コーディングエージェント向けの手引きである。
個人の環境や好みに依存する設定はここに書かず、各エージェントの個人用設定
（Claude Code なら `CLAUDE.local.md` など、Git の管理外のファイル）に書くこと。

## プロジェクト概要

変愚蛮怒 (Hengband) は Moria/Angband 系ローグライクゲームの C++20 バリアント。Zangband から派生し、100 階層のダンジョンを探索してラストボスを撃破することが目標。

本家のリポジトリは <https://github.com/hengband/hengband> で、開発ブランチは `develop`。
フォークで作業する場合、作業ブランチはフォークの `develop`（遅れていることがある）ではなく
本家の `develop` から切る。

ビルドは Unix 系（Linux、macOS など）の autotools と、Windows の Visual Studio (MSBuild) の 2 系統があり、
CI は両方でビルドとテストを行う。片方の環境でしか作業できない場合でも、もう片方のビルドを壊さないようにする
（「ソースファイルの追加・削除」「MSVC だけで失敗する警告」を参照）。
日本語版と英語版もそれぞれビルドされるので、日英で分岐するコード（`#ifdef JP`、`_()` の文字列、
文字コードの処理）を変えたときは両方でビルドが通ることを確かめる。

## ビルド（Unix 系: autotools）

依存パッケージは CI の `.github/workflows/build-with-autotools.yml` を参照（macOS の環境変数の設定もここにある）。
Ubuntu/Debian の最小構成は次のとおり（X11 版には `libx11-dev` なども要る）。

```bash
sudo apt-get install build-essential autoconf automake pkg-config libncurses-dev libcurl4-openssl-dev nkf
```

ビルド手順:

```bash
./bootstrap                                  # configure スクリプトを生成
./configure CXXFLAGS="-O2 -Wall -Wextra"     # 日本語版（デフォルト）。英語版は --disable-japanese を足す
make -j$(getconf _NPROCESSORS_ONLN)          # src/hengband が生成される
```

- **`make` には必ず `-j` で並列数を付ける。** `.cpp` が 900 を超え、並列化しないと非常に時間がかかる。
  `getconf _NPROCESSORS_ONLN` は Linux でも macOS でも CPU の数を返す（macOS には `nproc` が無い）。
- `CXXFLAGS` には最低限 `-Wall -Wextra` を付ける。プリコンパイル済みヘッダは `--disable-pch` で無効にできる。
- 日本語版は `-fexec-charset=euc-jp-ms` で文字列リテラルを EUC-JP に変換してビルドされる（ソースは UTF-8。
  Windows 版は SJIS）。

ccache を使うと日英の切り替えが速くなる。`CXX` は configure 時に `src/Makefile` へ焼き込まれるため、
**`make` ではなく `configure` に渡す**（configure を回し直すたびに必要）。

```bash
# base_dir と -fdebug-prefix-map は、パスの違う git worktree 間でもキャッシュを共有するため
CXX="ccache base_dir=$PWD g++" CXXFLAGS="-g -O2 -Wall -Wextra -fdebug-prefix-map=$PWD=." ./configure
# プリコンパイル済みヘッダのために要る設定（CI と同じ）
ccache --set-config=sloppiness=pch_defines,time_macros,include_file_mtime,include_file_ctime
ccache --set-config=pch_external_checksum=true
```

## ビルド（Windows: Visual Studio）

Visual Studio 2026（ツールセット v145）でビルドする。ソリューションは `VisualStudio/Hengband.sln` で、
次の 3 プロジェクトから成る。

| プロジェクト   | 種別                   | 出力                                             |
| -------------- | ---------------------- | ------------------------------------------------ |
| `HengbandCore` | スタティックライブラリ | ゲーム本体のコード（`main-win.cpp` 等を除く）    |
| `Hengband`     | アプリケーション       | リポジトリのトップの `Hengband.exe`              |
| `HengbandTest` | コンソールアプリ       | `VisualStudio/Hengband/<構成>/hengband-test.exe` |

構成は `Debug` / `Release`（日本語版）と `English-Debug` / `English-Release`（英語版）。プラットフォームは `Win32` のみ。

MSBuild が PATH に無い場合（Developer PowerShell 以外）は vswhere で探して呼ぶ。

```powershell
$msbuild = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" `
    -latest -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
# リポジトリのトップで実行する
& $msbuild .\VisualStudio\Hengband.sln /m /v:minimal /nologo /p:Configuration=Debug /p:Platform=Win32
```

- 英語版は `Configuration=English-Debug` などにする。特定のプロジェクトだけなら `/t:HengbandTest` などを足す
  （依存する `HengbandCore` も合わせてビルドされる）。
- CI（`.github/workflows/build-test-with-msvc.yml`）は `-warnAsError /t:Rebuild` で `Debug` と
  `English-Release` をビルドする。差分ビルドでは変更のないファイルがコンパイルされず、前回の警告が出ないので、
  プッシュ前の最終確認は CI と同じく `-warnAsError /t:Rebuild` で行う。
- `Hengband.exe` は全構成で同じパス（リポジトリのトップ）に出力されるため、構成を切り替えると差分ビルドで
  リンクされず、前の構成の exe が残ることがある。構成を切り替えたら `Hengband.exe` を削除してからビルドする。
- `Hengband.exe` を起動したままだとリンクが失敗する（出力ファイルを上書きできない）。ビルド前に終了させる。
- Debug 構成はリンカの `LinkVerbose` が有効で、リンク時に大量のログが出る。結果だけ見たいときは
  `| Select-Object -Last 15` などで末尾に絞る。エラーの有無は `error` を含む行で判断する。

## サブモジュール

- `VisualStudio/Hengband/libcurl` — Windows 版のリンクに必須（無いと `curl/curl.h` が見つからない）。
- `lib/xtra`（BGM・効果音・タイル）— Windows 版の起動に必要。Unix 系ではインストール時にだけ使う。

Unix 系ではどちらも初期化しなくてよく、音やタイルを確かめるときだけ `git submodule update --init lib/xtra` を実行する。
Windows で `git worktree add` したワークツリーでは `git submodule update --init --recursive` を実行してから
ビルド・起動する（削除には `git worktree remove --force` が要る）。

## ソースファイルの追加・削除

autotools も MSBuild もソースを自動収集しないため、**`.cpp` / `.h` を追加・削除・移動したら次のすべてを更新する。**

- `src/Makefile.am`
- `VisualStudio/Hengband/HengbandCore.vcxproj`（`<ClCompile>` / `<ClInclude>`）と `.filters`（フォルダ分けの表示用）

テストのソースは、`src/Makefile.am` の `hengband_test_SOURCES` と `VisualStudio/Hengband/HengbandTest.vcxproj`
（と `.filters`）に登録する。CI の `check-test-registration.sh` が検出するのはテストの登録漏れだけで、`HengbandCore` の登録漏れは
MSVC のビルドの失敗で初めて分かる。vcxproj は CRLF・パス区切りは `\`（`..\..\src\...`）なので、既存の行に揃える。

## テスト

ユニットテストは [doctest](https://github.com/doctest/doctest) で記述する。
Unix 系では `cd src && make -j$(getconf _NPROCESSORS_ONLN) check`、Windows では
`.\VisualStudio\Hengband\<構成>\hengband-test.exe` で実行する（`--test-case="*SHA256*"` などで絞り込める）。
MSVC の Debug 構成（`/ZI`）では、失敗した行（`__LINE__`）がずれて表示されることがある。

テストソースは `src/test/` に `src/` のディレクトリ構造をミラーして置く。テストケース名を ASCII で書くこと、
乱数のシードを固定することなどの規約と、追加の手順は `src/test/README.md` にある。**テストを書く前に必ず読むこと。**

グローバル状態（`PlayerType`、フロア、`term`）への依存が強くユニットテストを書けないものは、
後述の制御サーバで実際にゲームを動かして確かめる。

## コーディング規約

- フォーマットは `.clang-format` に従う（CI は clang-format-18）。インクルードの並び順もこれで決まる。
- `#include` は、変愚蛮怒のソースヘッダ（`src/` 配下、`src/external-lib/` を除く）を `""`、
  外部ライブラリ（`src/external-lib/include/`）とシステムヘッダを `<>` で書く。
- 関数の Doxygen コメント（`@brief` / `@param` / `@return` / `@details`）は、ヘッダの宣言側ではなく
  **`.cpp` の定義側に書く。** ファイル全体の説明や、型・構造体・定数の説明はヘッダに書いてよい。
- ranges のアルゴリズムやビューは、標準ライブラリのもの（`std::ranges::lower_bound`、`std::views` など）が
  環境によって使えないことがあるため、同梱の range-v3（`#include <range/v3/algorithm.hpp>`、
  `ranges::lower_bound` など）を使う。`std::ranges::forward_range` などのコンセプトや、
  `std::map::contains` のような C++20 の機能は使ってよい。
- **1 つの式の中で乱数を 2 回以上引かない。** 関数・コンストラクタの引数の評価順や、多くの二項演算子の
  両辺の評価順は規定されておらず（GCC は右から、clang は左から評価する）、コンパイラによってゲームの進行が
  食い違う。`Pos2D(randint0(h), randint0(w))` は一時変数に分けるか、左から順に評価される波括弧の初期化
  `Pos2D{ randint0(h), randint0(w) }` にする。CI の `check-rng-evaluation-order.py` が検出する。
- コード内のコメントは日本語で書く。

### MSVC だけで失敗する警告

Windows の CI は警告をエラーとして扱う。次の警告は gcc では検出できないことが多いので、
Windows でビルドできない環境で作業するときは特に注意する。

- **C4458**（ローカル変数がメンバー変数を隠す）: メンバー関数にローカル変数を足したら、
  名前がメンバーと重ならないことを確かめる。`g++ -Wshadow -fsyntax-only` で検出できる。
- **C4242**（縮小変換）: `int` の式を `byte` や `int16_t` の変数・引数に渡すと失敗する。
  三項演算子の両側の型を揃える（`TERM_COLOR{ 255 }` など）、受け側の型で一時変数を受けるなどで避ける。
  gcc の `-Wconversion` でも多くは検出できるが、値の範囲を推定できる式などでは警告せず、MSVC だけが失敗することがある。
  clang の `-Wimplicit-int-conversion` も併用し、変更したファイルの警告を変更前と比べる。
- **C4626 / C5027**（代入演算子が暗黙に削除される）: 参照のメンバーを持つクラスで出る。
  コピー・ムーブの代入演算子を `= delete` で明示する（コピー・ムーブ自体が要らないクラスでは、
  既存のクラスと同じくコンストラクタも合わせて `= delete` にする）。

## コミット

コミット前に、`.github/scripts/` にある CI のチェックのうち変更に関係するものを手元で流す
（Windows では Git Bash で実行できる）。

```bash
# upstream は本家 (hengband/hengband) を指すリモート名に読み替える。
# --merge-base は分岐点と作業ツリーを比べるので、まだコミットしていない変更も対象になる（新しいファイルは git add しておく）
# 変更したファイルだけ整形する（check-cpp-format.sh は src/ 全体にかける）
clang-format-18 -i $(git diff --name-only --diff-filter=d --merge-base upstream/develop -- 'src/*.cpp' 'src/*.h' ':!src/external-lib')
python3 .github/scripts/check-include-style.py
sh .github/scripts/check-test-registration.sh
# lib/ の JSON を変えたとき（check-json-format.sh は prettier を npm install -g して lib/ 全体にかける）
npx prettier --check $(git diff --name-only --diff-filter=d --merge-base upstream/develop -- 'lib/*.json' 'lib/*.jsonc')
```

ほかに `check-newline.sh`、`check-bom.sh` がある。

- コミットは 1 つの論理的な変更ごとに分け、各コミットが単独でビルドとテストを通るようにする。
  1 本の PR で複数段階のリファクタリングを行うときも、段階ごとにコミットを分ける。
- 件名は `[タグ] #<Issue 番号> 説明` の形（Issue が無ければ番号は省く）。説明は日本語・現在形で書く。
  - 主なタグ: `[Feature]`（機能追加。`[Add]` ではなく）、`[Fix]`、`[Refactor]`、`[Chore]`、`[Doc]`、`[Test]`
  - `[Fix]` の件名は不具合の症状で止め、「〜を修正した」のようなタグと重複する語を付けない。
    例: `[Fix] #1234 武器匠で折れた武器を一度に複数修復できてしまう`
- 件名だけにせず本文を付ける。空行の後に「何がどうなっていたか（原因と症状）」、
  さらに空行の後に「どう直すか」を書く。
- PR への force-push は認められている。PR を出した後の修正も `git commit --fixup` と
  `git rebase --autosquash` で該当コミットに折り込み、`git push --force-with-lease` で反映してよい。

## ゲームの動作確認（制御サーバ）

`--control-port=<port>` を付けて起動すると、127.0.0.1 の当該ポートで**画面内容の読み取りとキー入力の注入**を
受け付ける。`tools/bot/hbctl.py`（`screen`、`keys`、`state --no-map` など）で操作し、変更の効果を実際に確かめられる。
`--headless` を付けるとウィンドウを持たずに動く。使う前に **`tools/bot/README.md` を読むこと。**

```sh
# Unix 系
W=<作業用ディレクトリ>/hb; mkdir -p $W/save $W/user
printf 'Y:allow_debug_opts\n' > $W/user/pref.prf   # デバッグコマンド (^A) を使う場合
# ポートが使用中だと起動に失敗し、hbctl.py が既存のゲームにつながってしまう。未使用のポートを選び、両方に同じものを渡す
PORT=9001
src/hengband --headless --control-port=$PORT --fixed-seed=12345 \
    -ds=$W/save -du=$W/user -uBotTest -n < /dev/null 2> $W/headless.log &
python3 tools/bot/hbctl.py --port=$PORT screen
```

README を補う注意点:

- **Unix 系では必ず `-ds` と `-du` を付け、作業用ディレクトリに向ける。** 省略するとセーブデータや `playrecord-*.txt`、
  自動拾い設定などが `~/.angband/Hengband` に書き込まれ、普段使いの環境を汚してしまう。
- Windows でデバッグコマンド (`^A`) を使うには、`lib/user/pref.prf` に `Y:allow_debug_opts` を書く。
- Windows 版は多重起動できない。README のとおり 2 つ目を `--headless` で起動すると非ゼロで終了するが、
  ウィンドウ表示で起動すると「すでに起動しています」のダイアログで止まり、閉じると終了コード 0 で終わる。
- 確認用に起動したプロセスは、起動したときと同じポートを指定して `hbctl.py --port=$PORT quit` で止める（セーブしない）。
  ポートを省くと既定の 9000 番に送られ、そこで動いている別のゲームを止めてしまう。応答しない場合も
  `Stop-Process -Name Hengband` などで名前を指定して止めない（手元で遊んでいるゲームまでセーブせずに止めてしまう）。

## アーキテクチャ

- `src/` — C++ ソースコード。機能ドメインごとにサブディレクトリに分かれている（`ls src/` で一覧できる）。
  - `system/` — ゲーム全体の状態、`term/` — 端末の描画、`util/` — 汎用ユーティリティ
  - `external-lib/` — 同梱の外部ライブラリ（range-v3、doctest、fmt など）
  - フロントエンドは `main-gcu.cpp`（curses）、`main-x11.cpp`（X11）、`main-unix/`（Unix 系の補助）、
    `main-win.cpp` と `main-win/`（Windows）。エントリポイントは Unix 系が `main.cpp`、Windows が `main-win.cpp`
- `lib/` — ゲームデータ。`edit/` のモンスター・アイテム・地形などの定義は JSON で、スキーマは `schema/` にある
- `VisualStudio/` — Windows 版のソリューションとプロジェクト
- `tools/bot/` — 制御サーバのクライアント (`hbctl.py`)
