#include "info-reader/legend-object-trap-validator.h"
#include "info-reader/general-parser.h"
#include "info-reader/quest-reader.h"
#include "info-reader/random-grid-effect-types.h"
#include "locale/language-switcher.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include <fmt/format.h>
#include <nlohmann/json.hpp>

/*!
 * @brief 品と罠の配置分岐で片方が無視される組み合わせを検出する。
 * @details ランダムな品とランダムな罠の組み合わせは抽選配置なので許可する。
 * 地形の遅延解決にも対応するため、罠の種別は呼び出し側で判定する。
 */
bool has_legend_object_trap_conflict(const dungeon_grid &grid, bool object_is_quest_reward, bool fixed_trap, bool random_trap)
{
    const auto random_object = (grid.random & RANDOM_OBJECT) != 0;
    const auto fixed_object = object_is_quest_reward || (!random_object && grid.object != 0);
    return (fixed_object && (fixed_trap || random_trap)) || (random_object && fixed_trap);
}

/*! @brief 品と罠の配置衝突を説明する。 */
std::string_view describe_legend_object_trap_conflict()
{
    return _("品と罠の指定の片方が配置時に無視される組み合わせです", "This object/trap combination silently ignores one placement");
}

/*!
 * @brief 町の凡例のセルの trap のタグが固定の罠として扱われることを、配置衝突の理由に添える注記を作る
 * @param cell_data 凡例のセルの定義
 * @return 先頭に空白の付いた注記。trap が文字列のタグでないか、NONE・* なら空文字列
 * @details 罠の地形を指すタグなら誤記の可能性に触れない。
 * 罠ではない地形のタグ (FLOOR など) は一覧にあっても誤記の可能性があるため、短くしない。
 */
std::string describe_town_fixed_trap_tag(const nlohmann::json &cell_data)
{
    if (!cell_data.is_object() || !cell_data.contains("trap") || !cell_data["trap"].is_string()) {
        return {};
    }
    const auto &tag = cell_data["trap"].get_ref<const std::string &>();
    if (tag == "NONE" || tag == "*") {
        return {};
    }
    short terrain_id = 0;
    auto random_flags = 0;
    const auto is_trap_tag = resolve_legend_terrain(tag, terrain_id, random_flags, RANDOM_NONE) &&
                             TerrainList::get_instance().get_terrain(terrain_id).has(TerrainCharacteristics::TRAP);
    if (is_trap_tag) {
        return fmt::format(_(" (trap '{}' は固定の罠として扱われます)", " (trap '{}' is treated as a fixed trap)"), tag);
    }
    return fmt::format(_(" (trap '{}' は固定の罠として扱われます。未知のタグや誤記も固定の罠になります)",
                           " (trap '{}' is treated as a fixed trap; unknown tags and typos are treated the same way)"),
        tag);
}
