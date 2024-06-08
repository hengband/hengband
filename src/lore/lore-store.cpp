/*!
 * @brief モンスターの思い出を記憶する処理
 * @date 2020/06/09
 * @author Hourier
 */

#include "lore/lore-store.h"
#include "core/window-redrawer.h"
#include "monster/monster-info.h"
#include "system/floor-type-definition.h"
#include "system/monster-entity.h" //!< @todo 違和感、m_ptr は外から与えることとしたい.
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "tracking/lore-tracker.h"

template <class T>
static int count_lore_mflag_group(const EnumClassFlagGroup<T> &flags, const EnumClassFlagGroup<T> &r_flags)
{
    auto result_flags = flags;
    auto num = result_flags.reset(r_flags).count();
    return num;
}

/*!
 * @brief モンスターの調査による思い出補完処理
 * @param monrace_id 補完されるモンスター種族ID
 * @return 何か追加で明らかになったか否か
 */
bool lore_do_probe(MonsterRaceId monrace_id)
{
    auto &monrace = monraces_info[monrace_id];
    auto n = false;
    if (monrace.r_wake != MAX_UCHAR) {
        n = true;
    }

    if (monrace.r_ignore != MAX_UCHAR) {
        n = true;
    }

    monrace.r_wake = MAX_UCHAR;
    monrace.r_ignore = MAX_UCHAR;
    for (auto i = 0; i < 4; i++) {
        const auto &blow = monrace.blows[i];
        if ((blow.effect != RaceBlowEffectType::NONE) || (blow.method != RaceBlowMethodType::NONE)) {
            if (monrace.r_blows[i] != MAX_UCHAR) {
                n = true;
            }

            monrace.r_blows[i] = MAX_UCHAR;
        }
    }

    using Mdt = MonsterDropType;
    auto num_drops = (monrace.drop_flags.has(Mdt::DROP_4D2) ? 8 : 0);
    num_drops += (monrace.drop_flags.has(Mdt::DROP_3D2) ? 6 : 0);
    num_drops += (monrace.drop_flags.has(Mdt::DROP_2D2) ? 4 : 0);
    num_drops += (monrace.drop_flags.has(Mdt::DROP_1D2) ? 2 : 0);
    num_drops += (monrace.drop_flags.has(Mdt::DROP_90) ? 1 : 0);
    num_drops += (monrace.drop_flags.has(Mdt::DROP_60) ? 1 : 0);
    if (monrace.drop_flags.has_not(Mdt::ONLY_GOLD)) {
        if (monrace.r_drop_item != num_drops) {
            n = true;
        }

        monrace.r_drop_item = num_drops;
    }

    if (monrace.drop_flags.has_not(Mdt::ONLY_ITEM)) {
        if (monrace.r_drop_gold != num_drops) {
            n = true;
        }

        monrace.r_drop_gold = num_drops;
    }

    if (monrace.r_cast_spell != MAX_UCHAR) {
        n = true;
    }

    monrace.r_cast_spell = MAX_UCHAR;
    n |= count_lore_mflag_group(monrace.resistance_flags, monrace.r_resistance_flags) > 0;
    n |= count_lore_mflag_group(monrace.ability_flags, monrace.r_ability_flags) > 0;
    n |= count_lore_mflag_group(monrace.behavior_flags, monrace.r_behavior_flags) > 0;
    n |= count_lore_mflag_group(monrace.drop_flags, monrace.r_drop_flags) > 0;
    n |= count_lore_mflag_group(monrace.feature_flags, monrace.r_feature_flags) > 0;
    n |= count_lore_mflag_group(monrace.special_flags, monrace.r_special_flags) > 0;
    n |= count_lore_mflag_group(monrace.misc_flags, monrace.r_misc_flags) > 0;

    monrace.r_resistance_flags = monrace.resistance_flags;
    monrace.r_ability_flags = monrace.ability_flags;
    monrace.r_behavior_flags = monrace.behavior_flags;
    monrace.r_drop_flags = monrace.drop_flags;
    monrace.r_feature_flags = monrace.feature_flags;
    monrace.r_special_flags = monrace.special_flags;
    monrace.r_misc_flags = monrace.misc_flags;
    if (!monrace.r_can_evolve) {
        n = true;
    }

    monrace.r_can_evolve = true;
    if (LoreTracker::get_instance().is_tracking(monrace_id)) {
        RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::MONSTER_LORE);
    }

    return n;
}

/*!
 * @brief モンスターの撃破に伴うドロップ情報の記憶処理
 * @param num_item 手に入れたアイテム数
 * @param num_gold 手に入れた財宝の単位数
 */
void lore_treasure(const MonsterEntity &monster, int num_item, int num_gold)
{
    auto &monrace = monster.get_monrace();
    if (!monster.is_original_ap()) {
        return;
    }

    if (num_item > monrace.r_drop_item) {
        monrace.r_drop_item = num_item;
    }

    if (num_gold > monrace.r_drop_gold) {
        monrace.r_drop_gold = num_gold;
    }

    if (monrace.drop_flags.has(MonsterDropType::DROP_GOOD)) {
        monrace.r_drop_flags.set(MonsterDropType::DROP_GOOD);
    }

    if (monrace.drop_flags.has(MonsterDropType::DROP_GREAT)) {
        monrace.r_drop_flags.set(MonsterDropType::DROP_GREAT);
    }

    if (LoreTracker::get_instance().is_tracking(monster.r_idx)) {
        RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::MONSTER_LORE);
    }
}
