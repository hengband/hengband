#include "lore/monster-lore-snapshot.h"
#include "game-option/cheat-options.h"
#include "game-option/text-display-options.h"
#include "system/monrace/monrace-records.h"
#include "system/player-type-definition.h"
#include "world/world.h"
#include <range/v3/algorithm/copy.hpp>

/*!
 * @brief 思い出の表示を決める今の値を写す
 * @param player プレイヤーへの参照
 * @param monrace 思い出を表示するモンスター種族
 * @details 種族の思い出 (r_*) のうち、表示に使わない r_kind_flags は写さない
 * (種族の種別は倒した数と定義から表示する)。
 * 魔法の威力の計算には破滅の手で HP を使うが、思い出の破滅の手の威力は固定の文言 (40%-60%) なので、HP も写さない。
 */
MonsterLoreSnapshot::MonsterLoreSnapshot(const PlayerType &player, const MonraceDefinition &monrace)
    : monrace_id(monrace.idx)
    , player_level(player.lev)
    , player_max_level(player.max_plv)
    , is_wizard(AngbandWorld::get_instance().wizard)
    , knows_everything(cheat_know)
    , shows_summary(show_lore_summary)
    , shows_depth_in_feet(depth_in_feet)
    , has_been_seen(MonraceRecords::get_instance().has_been_seen(monrace.idx))
    , max_num(monrace.max_num)
    , r_deaths(monrace.r_deaths)
    , r_pkills(monrace.r_pkills)
    , r_akills(monrace.r_akills)
    , r_tkills(monrace.r_tkills)
    , r_wake(monrace.r_wake)
    , r_ignore(monrace.r_ignore)
    , r_can_evolve(monrace.r_can_evolve)
    , r_drop_gold(monrace.r_drop_gold)
    , r_drop_item(monrace.r_drop_item)
    , r_cast_spell(monrace.r_cast_spell)
    , r_ability_flags(monrace.r_ability_flags)
    , r_aura_flags(monrace.r_aura_flags)
    , r_behavior_flags(monrace.r_behavior_flags)
    , r_resistance_flags(monrace.r_resistance_flags)
    , r_drop_flags(monrace.r_drop_flags)
    , r_feature_flags(monrace.r_feature_flags)
    , r_special_flags(monrace.r_special_flags)
    , r_misc_flags(monrace.r_misc_flags)
{
    ranges::copy(monrace.r_blows, this->r_blows.begin());
}
