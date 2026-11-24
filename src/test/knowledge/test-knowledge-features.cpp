/*!
 * @brief 地形の知識の一覧に載せる地形を集める処理と、一覧の閲覧の入力待ちの文脈のテスト
 */

#include "game-option/special-options.h"
#include "io-dump/dump-util.h"
#include "io/input-context.h"
#include "knowledge/knowledge-features.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "term/z-term.h"
#include "test/scoped-input-term.h"
#include "test/scoped-restore.h"
#include "util/finalizer.h"
#include <algorithm>
#include <doctest/doctest.h>
#include <string>
#include <utility>
#include <vector>

namespace {

/*!
 * @brief テスト用の地形の定義
 */
struct TerrainDefine {
    std::string name; //!< 地形名
    short mimic; //!< 外形地形ID
};

/*!
 * @brief 地形の一覧を指定した地形に差し替え、スコープを抜けるときに元へ戻す
 * @param defines 地形ごとの名前と外形地形ID
 */
auto replace_terrains(const std::vector<TerrainDefine> &defines)
{
    auto &terrains = TerrainList::get_instance();
    std::vector<TerrainType> backup(terrains.begin(), terrains.end());
    terrains.resize(defines.size());
    for (short id = 0; const auto &[name, mimic] : defines) {
        auto &terrain = terrains.get_terrain(id);
        terrain = TerrainType{};
        terrain.idx = id;
        terrain.name = name;
        terrain.mimic = mimic;
        ++id;
    }

    return util::make_finalizer([&terrains, backup = std::move(backup)] {
        terrains.resize(backup.size());
        std::copy(backup.begin(), backup.end(), terrains.begin());
    });
}

//! シンボルの編集をしておらず、クリップボードが空のときの、地形の一覧のキーの案内
#ifdef JP
constexpr auto KEY_GUIDE = "<\xe6\x96\xb9\xe5\x90\x91>, 'v'\xe3\x81\xa7\xe3\x82\xb7\xe3\x83\xb3\xe3\x83\x9c\xe3\x83\xab\xe5\xa4\x89\xe6\x9b\xb4, 'd'\xe3\x81\xa7\xe6\xa8\x99\xe6\xba\x96\xe5\x85\x89\xe6\xba\x90\xe5\x8a\xb9\xe6\x9e\x9c, 'c'\xe3\x81\xa7\xe3\x82\xb3\xe3\x83\x94\xe3\x83\xbc, ESC"; // <方向>, 'v'でシンボル変更, 'd'で標準光源効果, 'c'でコピー, ESC
#else
constexpr auto KEY_GUIDE = "<dir>, 'v' for visuals, 'd' for default lighting, 'c' to copy, ESC";
#endif

//! シンボルの編集中で、クリップボードが空のときの、地形の一覧のキーの案内
#ifdef JP
constexpr auto EDIT_KEY_GUIDE = "<\xe6\x96\xb9\xe5\x90\x91>, ENTER\xe3\x81\xa7\xe6\xb1\xba\xe5\xae\x9a, 'a'\xe3\x81\xa7\xe5\xaf\xbe\xe8\xb1\xa1\xe6\x98\x8e\xe5\xba\xa6\xe5\xa4\x89\xe6\x9b\xb4, 'd'\xe3\x81\xa7\xe6\xa8\x99\xe6\xba\x96\xe5\x85\x89\xe6\xba\x90\xe5\x8a\xb9\xe6\x9e\x9c, 'c'\xe3\x81\xa7\xe3\x82\xb3\xe3\x83\x94\xe3\x83\xbc, ESC"; // <方向>, ENTERで決定, 'a'で対象明度変更, 'd'で標準光源効果, 'c'でコピー, ESC
#else
constexpr auto EDIT_KEY_GUIDE = "<dir>, ENTER to accept, 'a' for lighting level, 'd' for default lighting, 'c' to copy, ESC";
#endif
}

TEST_CASE("collect_features lists every terrain")
{
    const auto restore = replace_terrains({ { "floor", 0 }, { "wall", 1 }, { "door", 2 } });

    CHECK(collect_features() == std::vector<FEAT_IDX>{ 0, 1, 2 });
}

TEST_CASE("collect_features skips unnamed and mimicking terrains")
{
    const auto restore = replace_terrains({ { "floor", 0 }, { "", 1 }, { "secret door", 0 }, { "wall", 3 } });

    CHECK(collect_features() == std::vector<FEAT_IDX>{ 0, 3 });
}

TEST_CASE("collect_features returns an empty list if no terrain is listed")
{
    const auto restore = replace_terrains({ { "", 0 }, { "secret door", 0 } });

    CHECK(collect_features().empty());
}

TEST_CASE("do_cmd_knowledge_features waits for a key without looking up a terrain if no terrain is listed")
{
    const auto restore = replace_terrains({ { "", 0 }, { "secret door", 0 } });
    // キーの案内はクリップボードにシンボルがあるかで変わるので、空にしておく
    auto &clipboard = DisplaySymbolsClipboard::get_instance();
    const auto restore_clipboard = test::scoped_restore(clipboard.symbol, clipboard.symbols);
    clipboard.symbol = {};
    clipboard.reset_symbols();
    test::ScopedInputTerm term;

    // 範囲外の地形を引かずに一覧の閲覧の待ちまで進み、ESC で閉じる
    term.expect({ InputKind::BROWSER, KEY_GUIDE, {} }, "\x1b");
    auto need_redraw = false;
    IDX lighting_level = F_LIT_STANDARD;
    do_cmd_knowledge_features(&need_redraw, false, -1, &lighting_level);
}

TEST_CASE("do_cmd_knowledge_features publishes the browser context while browsing listed terrains")
{
    const auto restore = replace_terrains({ { "floor", 0 }, { "wall", 1 } });
    // キーの案内はクリップボードにシンボルがあるかで変わるので、空にしておく
    auto &clipboard = DisplaySymbolsClipboard::get_instance();
    const auto restore_clipboard = test::scoped_restore(clipboard.symbol, clipboard.symbols);
    clipboard.symbol = {};
    clipboard.reset_symbols();
    test::ScopedInputTerm term;

    // カーソルのキーを送って待ち直すたびに閲覧の文脈を公開し、v でシンボルの編集に入ると案内が変わる。
    // ESC で編集をやめて元の案内に戻り、もう一度 ESC で閉じる
    term.expect({ InputKind::BROWSER, KEY_GUIDE, {} }, "62v");
    term.then_expect({ InputKind::BROWSER, EDIT_KEY_GUIDE, {} }, "\x1b");
    term.then_expect({ InputKind::BROWSER, KEY_GUIDE, {} }, "\x1b");
    auto need_redraw = false;
    IDX lighting_level = F_LIT_STANDARD;
    do_cmd_knowledge_features(&need_redraw, false, -1, &lighting_level);
}

TEST_CASE("do_cmd_knowledge_features restores every lighting level when cancelling a directly specified terrain")
{
    const auto restore = replace_terrains({ { "floor", 0 } });
    auto &terrain = TerrainList::get_instance().get_terrain(0);
    const TerrainSymbols symbols{ { { 1, '.' }, { 11, '*' }, { 2, '#' } } };
    terrain.symbol_configs = symbols;
    // キーの案内はクリップボードにシンボルがあるかで変わるので、空にしておく
    auto &clipboard = DisplaySymbolsClipboard::get_instance();
    const auto restore_clipboard = test::scoped_restore(clipboard.symbol, clipboard.symbols);
    clipboard.symbol = {};
    clipboard.reset_symbols();
    test::ScopedInputTerm term;

    // 番号を指定すると明色のシンボルの編集から始まる。6 で明色の文字を変え、d でほかの段階も標準色から求め直してから、
    // ESC で取り消して閉じる。どの明るさの段階も元のまま残る
    term.expect({ InputKind::BROWSER, EDIT_KEY_GUIDE, {} }, "6d\x1b");
    auto need_redraw = false;
    IDX lighting_level = F_LIT_LITE;
    do_cmd_knowledge_features(&need_redraw, true, 0, &lighting_level);

    CHECK(terrain.symbol_configs == symbols);
}

TEST_CASE("do_cmd_knowledge_features scrolls the visual list within the terminal width")
{
    const auto restore = replace_terrains({ { "floor", 0 } });
    auto &terrain = TerrainList::get_instance().get_terrain(0);
    terrain.symbol_configs = { { { 1, '.' }, { 11, '.' }, { 2, '.' } } };
    // 表の幅は 2 マスのタイルかで変わるので、1 マスにしておく
    const auto restore_bigtile = test::scoped_restore(use_bigtile);
    use_bigtile = false;
    // キーの案内はクリップボードにシンボルがあるかで変わるので、空にしておく
    auto &clipboard = DisplaySymbolsClipboard::get_instance();
    const auto restore_clipboard = test::scoped_restore(clipboard.symbol, clipboard.symbols);
    clipboard.symbol = {};
    clipboard.reset_symbols();
    test::ScopedInputTerm term;

    // 明色の文字を 6 で '.' から 70 文字ぶん右の 't' へ送ってから、ESC で閉じる
    term.expect({ InputKind::BROWSER, EDIT_KEY_GUIDE, {} }, std::string(70, '6') + "\x1b");
    auto need_redraw = false;
    IDX lighting_level = F_LIT_LITE;
    do_cmd_knowledge_features(&need_redraw, true, 0, &lighting_level);

    // 表は 11 桁目から端末の右端までの 69 文字ぶんとして送られ、表の左端 (7 行目の 11 桁目) は '9' になる。
    // 幅を 6 文字広く求めていると、送りが遅れて '3' になる
    CHECK(game_term->scr->c[7][11].front() == '9');
}

TEST_CASE("do_cmd_knowledge_features scrolls the visual list back when 'd' resets the edited symbol")
{
    const auto restore = replace_terrains({ { "floor", 0 } });
    auto &terrain = TerrainList::get_instance().get_terrain(0);
    terrain.symbol_configs = { { { 1, '.' }, { 11, '.' }, { 2, '.' } } };
    // 表とカーソルの桁は 2 マスのタイルかで変わるので、1 マスにしておく
    const auto restore_bigtile = test::scoped_restore(use_bigtile);
    use_bigtile = false;
    // キーの案内はクリップボードにシンボルがあるかで変わるので、空にしておく
    auto &clipboard = DisplaySymbolsClipboard::get_instance();
    const auto restore_clipboard = test::scoped_restore(clipboard.symbol, clipboard.symbols);
    clipboard.symbol = {};
    clipboard.reset_symbols();
    test::ScopedInputTerm term;

    // 明色の文字を 6 で 70 文字ぶん右へ送って表を送らせてから、d で '.' に戻して ESC で閉じる
    term.expect({ InputKind::BROWSER, EDIT_KEY_GUIDE, {} }, std::string(70, '6') + "d\x1b");
    auto need_redraw = false;
    IDX lighting_level = F_LIT_LITE;
    do_cmd_knowledge_features(&need_redraw, true, 0, &lighting_level);

    // 表の左端 (7 行目の 11 桁目) は、戻した '.' の 10 文字手前の '$' まで送り戻され、カーソルは表の中の '.' の桁にある
    CHECK(game_term->scr->c[7][11].front() == '$');
    CHECK(term_locate().first == 11 + 10);
}
