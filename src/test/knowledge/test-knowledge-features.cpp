/*!
 * @brief 地形の知識の一覧に載せる地形を集める処理と、一覧の閲覧の入力待ちの文脈のテスト
 */

#include "io-dump/dump-util.h"
#include "io/input-context.h"
#include "knowledge/knowledge-features.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
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
#ifdef SJIS
constexpr auto KEY_GUIDE = "<\x95\xfb\x8c\xfc>, 'v'\x82\xc5\x83\x56\x83\x93\x83\x7b\x83\x8b\x95\xcf\x8d\x58, 'd'\x82\xc5\x95\x57\x8f\x80\x8c\xf5\x8c\xb9\x8c\xf8\x89\xca, 'c'\x82\xc5\x83\x52\x83\x73\x81\x5b, ESC"; // <方向>, 'v'でシンボル変更, 'd'で標準光源効果, 'c'でコピー, ESC
#else
constexpr auto KEY_GUIDE = "<\xca\xfd\xb8\xfe>, 'v'\xa4\xc7\xa5\xb7\xa5\xf3\xa5\xdc\xa5\xeb\xca\xd1\xb9\xb9, 'd'\xa4\xc7\xc9\xb8\xbd\xe0\xb8\xf7\xb8\xbb\xb8\xfa\xb2\xcc, 'c'\xa4\xc7\xa5\xb3\xa5\xd4\xa1\xbc, ESC"; // <方向>, 'v'でシンボル変更, 'd'で標準光源効果, 'c'でコピー, ESC
#endif
#else
constexpr auto KEY_GUIDE = "<dir>, 'v' for visuals, 'd' for default lighting, 'c' to copy, ESC";
#endif

//! シンボルの編集中で、クリップボードが空のときの、地形の一覧のキーの案内
#ifdef JP
#ifdef SJIS
constexpr auto EDIT_KEY_GUIDE = "<\x95\xfb\x8c\xfc>, ENTER\x82\xc5\x8c\x88\x92\xe8, 'a'\x82\xc5\x91\xce\x8f\xdb\x96\xbe\x93\x78\x95\xcf\x8d\x58, 'd'\x82\xc5\x95\x57\x8f\x80\x8c\xf5\x8c\xb9\x8c\xf8\x89\xca, 'c'\x82\xc5\x83\x52\x83\x73\x81\x5b, ESC"; // <方向>, ENTERで決定, 'a'で対象明度変更, 'd'で標準光源効果, 'c'でコピー, ESC
#else
constexpr auto EDIT_KEY_GUIDE = "<\xca\xfd\xb8\xfe>, ENTER\xa4\xc7\xb7\xe8\xc4\xea, 'a'\xa4\xc7\xc2\xd0\xbe\xdd\xcc\xc0\xc5\xd9\xca\xd1\xb9\xb9, 'd'\xa4\xc7\xc9\xb8\xbd\xe0\xb8\xf7\xb8\xbb\xb8\xfa\xb2\xcc, 'c'\xa4\xc7\xa5\xb3\xa5\xd4\xa1\xbc, ESC"; // <方向>, ENTERで決定, 'a'で対象明度変更, 'd'で標準光源効果, 'c'でコピー, ESC
#endif
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
