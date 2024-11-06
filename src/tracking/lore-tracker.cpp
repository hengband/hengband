/*!
 * @brief サブウィンドウに表示するモンスター種族IDを保持するクラス
 * @author Hourier
 * @date 2024/06/08
 */

#include "tracking/lore-tracker.h"
#include "system/monster-race-info.h"
#include "system/redrawing-flags-updater.h"

LoreTracker LoreTracker::instance{};

LoreTracker &LoreTracker::get_instance()
{
    return instance;
}

bool LoreTracker::is_tracking() const
{
    return MonraceList::is_valid(this->monrace_id);
}

bool LoreTracker::is_tracking(MonraceId trackee) const
{
    return this->monrace_id == trackee;
}

MonraceId LoreTracker::get_trackee() const
{
    return this->monrace_id;
}

/*!
 * @brief トラッキング中のモンスター種族定義を取得する
 * @return モンスター種族定義 (トラッキングしていなければ無効モンスター)
 */
const MonraceDefinition &LoreTracker::get_tracking_monrace() const
{
    return MonraceList::get_instance().get_monrace(this->monrace_id);
}

void LoreTracker::set_trackee(MonraceId new_monrace_id)
{
    this->monrace_id = new_monrace_id;
    RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::MONSTER_LORE);
}
