/*!
 * @brief サブウィンドウに表示するベースアイテムIDを保持するクラス
 * @author Hourier
 * @date 2024/06/08
 */

#include "tracking/baseitem-tracker.h"
#include "system/item-entity.h"

BaseitemTracker BaseitemTracker::instance{};

BaseitemTracker &BaseitemTracker::get_instance()
{
    return instance;
}

bool BaseitemTracker::is_tracking() const
{
    return this->bi_id > 0;
}

ItemEntity BaseitemTracker::get_trackee() const
{
    return ItemEntity(this->bi_id);
}

void BaseitemTracker::set_trackee(short new_bi_id)
{
    this->bi_id = new_bi_id;
}
