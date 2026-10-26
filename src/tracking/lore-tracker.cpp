/*!
 * @brief サブウィンドウに表示するモンスター種族IDを保持するクラス
 * @author Hourier
 * @date 2024/06/08
 */

#include "tracking/lore-tracker.h"
#include "system/deferred-update-requests.h"
#include "system/monrace/monrace-list.h"

LoreTracker LoreTracker::instance{};

LoreTracker &LoreTracker::get_instance()
{
    return instance;
}

bool LoreTracker::is_tracking() const
{
    return MonraceList::is_valid(this->monrace_id);
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

/*!
 * @brief 思い出を表示するモンスター種族を変える
 * @param new_monrace_id 新しく表示するモンスター種族ID
 * @details 思い出の値の変化は表示した思い出との比較 (MonsterLoreSnapshot) で拾うが、
 * 同じ端末にほかのサブウィンドウも割り当てられていると、思い出の値が変わらなくても上書きされていることがある。
 * 種族を指定したときは表示し直したいので、同じ種族でも描き直しを要求する。
 */
void LoreTracker::set_trackee(MonraceId new_monrace_id)
{
    this->monrace_id = new_monrace_id;
    DeferredUpdateRequests::get_instance().set_flag(SubWindowRedrawingFlag::MONSTER_LORE);
}
