# ユニットテスト

変愚蛮怒のユニットテストは [doctest](https://github.com/doctest/doctest) で記述します。
autotools（Unix 系）では `make check`、Windows では Visual Studio の `HengbandTest` プロジェクトで実行します。

## どちらのテスト手段を使うか

本プロジェクトのテストは 2 層あります。**まず対象がどちらに向いているかを判断してください。**

| 層             | 手段                                               | 向いている対象                                                                |
| -------------- | -------------------------------------------------- | ----------------------------------------------------------------------------- |
| ユニットテスト | doctest / `make check`・Visual Studio（このドキュメント） | 純粋関数、値クラス、計算ロジック（`util/`、`system/` の型、ダメージ計算など） |
| シナリオテスト | `--headless --control-port` + `tools/bot/hbctl.py` | ゲームループ、コマンド処理、画面表示、グローバル状態をまたぐ挙動              |

判断の基準は **「対象を呼び出すために `PlayerType` やフロア、ターミナル (`term`) を用意する必要があるか」** です。

- 不要 → ユニットテストを書く
- 必要 → [シナリオテスト](#テストしにくいコードをどう確かめるか)で確かめる。可能なら純粋関数として切り出してユニットテスト化する

---

## 実行方法

### autotools（Unix 系）

```sh
cd src
make check                    # テストをビルドして実行する
make check VERBOSE=1          # 失敗時にテストの出力をその場で表示する
```

`make check` はゲーム本体 (`libhengband.a`) のビルドも行うため、初回は時間がかかります。
ビルド済みであれば、テストバイナリを直接実行するほうが手軽です。

```sh
./hengband-test                              # 全テストを実行する
./hengband-test --list-test-cases            # テストケースの一覧を表示する
./hengband-test --test-case="*SHA256*"       # 名前で絞り込んで実行する
./hengband-test --success                    # 成功したアサーションも表示する
./hengband-test --help                       # doctest のオプション一覧
```

テストが失敗すると `make check` は非ゼロで終了し、`src/test-suite.log` に出力が残ります。

### Visual Studio（Windows）

`VisualStudio/Hengband.sln` の `HengbandTest` プロジェクトがテスト実行ファイルを生成します。
ソリューションをビルドすればゲーム本体と一緒にビルドされます。

コマンドラインからビルドする場合は次のとおりです。

```pwsh
MSBuild .\VisualStudio\Hengband.sln /p:Configuration=Debug
.\VisualStudio\Hengband\Debug\hengband-test.exe
```

出力先は構成ごとに分かれます（`Debug` / `English-Debug` / `Release` / `English-Release`）。

```pwsh
.\VisualStudio\Hengband\<構成>\hengband-test.exe
```

Visual Studio の IDE から実行する場合は、`HengbandTest` を右クリックして
「スタートアップ プロジェクトに設定」してから開始してください
（既定のスタートアッププロジェクトはゲーム本体の `Hengband` です）。

`--list-test-cases` などの doctest のオプションは autotools 版と共通です。

## テストの追加手順

1. **テストファイルを作る**

   `src/test/` 以下は `src/` のディレクトリ構造をミラーします。ファイル名は `test-<対象のファイル名>.cpp` です。

   | テスト対象                    | テストファイル                       |
   | ----------------------------- | ------------------------------------ |
   | `src/util/sha256.cpp`         | `src/test/util/test-sha256.cpp`      |
   | `src/combat/shoot.cpp`        | `src/test/combat/test-shoot.cpp`     |

2. **テストを書く**

   ```cpp
   /*!
    * @brief 〇〇のテスト
    */

   #include "util/sha256.h"   // テスト対象のヘッダ

   #include <doctest/doctest.h>

   TEST_CASE("SHA256 hashes an empty message")
   {
       util::SHA256 hash;
       CHECK(util::to_string(hash.digest()) == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
   }
   ```

3. **2 つのビルド定義の両方に登録する**

   **どちらも自動収集はされません。** 登録を忘れたテストはビルドも実行もされないので、
   必ず両方に追加してください（CI の `check_test_registration` が**両方**の追加忘れを検出します。
   コメントアウトによる無効化も検出しますが、vcxproj の `<ExcludedFromBuild>` は検出できません）。

   `src/Makefile.am` の `hengband_test_SOURCES`:

   ```make
   hengband_test_SOURCES = \
       test/test-main.cpp \
       \
       test/util/test-probability-table.cpp \
       test/util/test-sha256.cpp \
       ...
   ```

   `VisualStudio/Hengband/HengbandTest.vcxproj` の `<ClCompile>`:

   ```xml
   <ItemGroup>
     <ClCompile Include="..\..\src\test\test-main.cpp" />
     <ClCompile Include="..\..\src\test\util\test-probability-table.cpp" />
     <ClCompile Include="..\..\src\test\util\test-sha256.cpp" />
     ...
   </ItemGroup>
   ```

   Visual Studio 上での表示を整えるため、`HengbandTest.vcxproj.filters` にも
   同じファイルを追加しておくとよいです（こちらは登録しなくてもビルド・実行はできます）。

4. **ビルドして確認する**

   autotools では `make check` を実行します（`Makefile.am` を変更したので、`make` が
   自動的に `configure` を回し直します）。Windows では Visual Studio でソリューションを
   ビルドし、`hengband-test.exe` を実行します。

## doctest の書き方

よく使うものだけ挙げます。アサーションマクロの一覧は
[doctest のドキュメント](https://github.com/doctest/doctest/blob/master/doc/markdown/assertions.md) を参照してください。

### テストケースとアサーション

```cpp
TEST_CASE("test case name")
{
    CHECK(a == b);           // 失敗しても後続の検証を続ける（基本はこちら）
    REQUIRE(ptr != nullptr); // 失敗したらこのテストケースを即座に中断する
                             // これ以降を実行すると壊れる場合に使う
}
```

### SUBCASE

`SUBCASE` は、その `TEST_CASE` の本体を先頭から実行し直したうえで、1 つずつ実行されます。
**共通の準備処理を各サブケースで独立にやり直せる**のが利点です。

```cpp
TEST_CASE("ProbabilityTable")
{
    ProbabilityTable<int> table;  // 各SUBCASEごとに作り直される
    table.entry_item(1, 10);

    SUBCASE("entried item is counted")
    {
        CHECK(table.item_count() == 1);
    }

    SUBCASE("cleared table is empty")
    {
        table.clear();
        CHECK(table.empty());
    }
}
```

### 例外と浮動小数点数

```cpp
CHECK_THROWS_AS(table.pick_one_at_random(), std::runtime_error);
CHECK_NOTHROW(do_something());
CHECK(calculated_rate == doctest::Approx(0.25).epsilon(0.001));
```

### 失敗時の情報を増やす

ループの中で検証する場合、そのままでは何番目で落ちたのか分かりません。`CAPTURE` で変数を記録しておくと、
失敗したときにその値が表示されます。

```cpp
for (const auto &test : TEST_VECTORS) {
    CAPTURE(test.expected);
    CHECK(...);
}
```

## 守ること

### テストケース名は ASCII で書く

日本語版のビルドでは `gcc-wrap` がソース中の文字列リテラルを UTF-8 から EUC-JP に変換します。
テストケース名に日本語を使うと、UTF-8 の端末でテスト結果が文字化けします。
MSVC の日本語版構成も同様に、文字列リテラルを Shift_JIS に変換 (`/execution-charset:shift-jis`)
するため文字化けします。

**コメントはこれまでどおり日本語で構いません。** 制約を受けるのは実行時に出力される文字列（テストケース名、
`CAPTURE` する文字列など）だけです。

### ファイルスコープの定義は無名名前空間に入れる

すべてのテストは `hengband-test` という 1 つの実行ファイルにリンクされます。
テストファイル内のヘルパー関数や定数をそのままファイルスコープに置くと、
別のテストファイルの同名のものとシンボルが衝突します。

```cpp
namespace {

constexpr auto TEST_SEED = 12345;
void setup() { ... }

}
```

### テストは決定的にする

`make check` は何度実行しても同じ結果になる必要があります。

実行のたびに異なる乱数で確率的な検証を行うと、ごくまれに失敗する不安定なテストになり、
CI が信用されなくなります。

乱数を使う場合は `test/scoped-rng.h` の `test::scoped_rng()` でシードを固定してください。
戻り値を変数で受けている間だけシードが固定され、スコープを抜けると乱数生成器の状態が元に戻ります。

```cpp
#include "test/scoped-rng.h"

TEST_CASE("dice roll stays within its range")
{
    const auto restore_rng = test::scoped_rng(); // シードを指定するなら test::scoped_rng(999)

    CHECK(randint1(6) <= 6);
}
```

**戻り値は必ず変数で受けてください。** 受け損ねるとその場で復元されてシードの固定が効きません。

`[[nodiscard]]` を付けてあるので、受け損ねた場合はコンパイラが警告を出します。
CI と Visual Studio のビルドは警告をエラーとして扱う設定（`-Werror` /
`TreatWarningAsError`）なので、そこではビルドが失敗します。
手元のビルドで警告のままにしている場合は見落とさないよう注意してください。

### グローバルな状態を変えたら元に戻す

`AngbandSystem` のようなシングルトンを書き換えるテストは、他のテストに影響します。
ゲーム本体と同じく `util::make_finalizer()` を使って、スコープを抜けるときに元へ戻してください。
前節の `test::scoped_rng()` も、この仕組みで乱数生成器の状態を復元しています。

```cpp
auto &system = AngbandSystem::get_instance();
const auto restore_hoge = util::make_finalizer([&system, backup = system.get_hoge()]() { system.set_hoge(backup); });
```

そのうえで、**必要な前提は各テストケースの中で自分で設定してください。**
テストの実行順序や `--test-case=` での絞り込み実行に依存しないようにするためです。

## リンクについて

テストは、ゲーム本体のソースをエントリポイントだけ除いてすべてまとめた静的ライブラリに
リンクされます。autotools では `libhengband.a`（`main.cpp` 以外）、MSVC では
`HengbandCore.lib`（`main-win.cpp` 以外）です。
**テスト対象の `.cpp` を個別に指定する必要はありません。** ゲーム内のどの関数・クラスでも、
ヘッダを `#include` すればテストから呼び出せます。

ただし、静的ライブラリからは**未定義シンボルの解決に必要なオブジェクトしか取り込まれません。**
静的初期化子による自己登録だけを目的とする翻訳単位を追加しても、リンクされず、警告も出ないまま
無視されます。autotools には `--whole-archive` という逃げ道がありますが、**MSVC で
`/WHOLEARCHIVE` は使えません。** GUI 専用のオブジェクト（`main-win.cpp` にしか定義のない
シンボルを参照する `main-win/commandline-win.cpp` など）まで引き込んでリンクエラーになります。

## doctest の警告について

MSVC のビルドは `/Wall`（`EnableAllWarnings`）と `TreatWarningAsError` を使っていますが、
doctest 由来の警告を個別に抑止する指定は必要ありません。

doctest は `src/external-lib/include/` にあり、`check_include_style` が同ディレクトリの
ヘッダを `<>` で include するよう強制しています。そのため
`Hengband.Common.props` の `TreatAngleIncludeAsExternal` と
`ExternalWarningLevel=TurnOffAllWarnings` が必ず効きます
（`/external:templates-` を指定していないので、テンプレートの実体化も対象です）。

## テストしにくいコードをどう確かめるか

冒頭の判断基準でシナリオテスト側に振り分けたもの
（`PlayerType` やフロア、`term` を用意しないと呼べないコード）は、
ゲームを実際に起動して外部から操作・観測することで確かめます。

`--headless` を付けると端末を持たない環境でも起動でき、`--control-port` で開いたソケット越しに
`tools/bot/hbctl.py` からキー入力の注入と画面・内部状態の読み取りができます。
**起動方法・オプション・注意点は [tools/bot/README.md](../../tools/bot/README.md) を参照してください。**

テストとして使ううえでの要点は 2 つです。

- **再現性**: `--fixed-seed` に同じ値を与え、同じキー列を送れば、画面は完全に一致します。
  バグの再現手順をキー列ファイルとして記録し、`hbctl.py replay keys.txt` で再生できます。
- **何を検証するか**: 画面の文字列よりも `hbctl.py state` が返す内部状態の値を突き合わせるほうが、
  表示の変更に影響されにくく壊れにくいテストになります。

シナリオテストを自動実行する仕組みは現時点では用意されていません（`make check` の対象は
doctest によるユニットテストのみです）。

なお、計算部分をグローバル状態に触らない関数へ切り出せば、ユニットテストが書けるようになります。
テストを書きやすい形へのリファクタリングは歓迎されます。

## 制限

- **`PlayerType` やフロア、`term` を必要とするコードはユニットテストにできません。**
  冒頭の判断基準のとおり、シナリオテストで確かめてください。
- **シナリオテストを自動実行する仕組みはありません。** `make check` と MSVC の CI が
  対象とするのは doctest によるユニットテストのみです。
- **MSVC の CI がテストを実行するのは Debug 構成だけです。** 手元では 4 構成すべてで
  ビルド・実行できます。Release 系は警告の除外リストが Debug と異なる（`4711;4738` が加わる）
  ため、Release でだけ壊れる変更は CI をすり抜けます。
  `Build-Windows-Release-Package.ps1` はソリューション全体をリビルドするので、
  リリースパッケージの作成もテストプロジェクトのビルドが通ることに依存します。
