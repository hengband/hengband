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
