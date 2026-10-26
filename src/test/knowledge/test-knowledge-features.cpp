/*!
 * @brief 地形の知識の一覧に載せる地形を集める処理のテスト
 */

#include "knowledge/knowledge-features.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "test/scoped-input-term.h"
#include "util/finalizer.h"
#include <algorithm>
#include <doctest/doctest.h>
#include <stdexcept>
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
    test::ScopedInputTerm term;

    // 一覧の画面は入力待ちの文脈を公開しないので、キーは渡せない。
    // 範囲外の地形を引かずに入力待ちまで進めば、キーを使い切った後の入力待ちとして例外になる
    auto need_redraw = false;
    IDX lighting_level = F_LIT_STANDARD;
    CHECK_THROWS_WITH_AS(do_cmd_knowledge_features(&need_redraw, false, -1, &lighting_level), "Unexpected extra input wait", std::runtime_error);
}
