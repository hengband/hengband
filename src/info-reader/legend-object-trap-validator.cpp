#include "info-reader/legend-object-trap-validator.h"
#include "info-reader/general-parser.h"
#include "info-reader/random-grid-effect-types.h"
#include "locale/language-switcher.h"

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
