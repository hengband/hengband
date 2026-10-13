#include "autopick/autopick-util.h"
#include "autopick/autopick-menu-data-table.h"
#include "core/window-redrawer.h"
#include "game-option/input-options.h"
#include "main/sound-of-music.h"
#include "object-enchant/item-feeling.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/item/item-entity.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include <algorithm>

/*!
 * @brief 自動拾い/破壊設定のリストに関する変数 / List for auto-picker/destroyer entries
 */
std::vector<autopick_type> autopick_list; /*!< 自動拾い/破壊設定構造体の配列 */

/*!
 * @brief Automatically destroy an item if it is to be destroyed
 * @details
 * When always_pickup is 'yes', we disable auto-destroyer function of
 * auto-picker/destroyer, and do only easy-auto-destroyer.
 */
ItemEntity autopick_last_destroyed_object;

bool autopick_type::has(int flag) const
{
    return this->flags[flag / 32] & (1UL << (flag % 32));
}

void autopick_type::add(int flag)
{
    set_bits(this->flags[flag / 32], 1UL << (flag % 32));
}

void autopick_type::remove(int flag)
{
    reset_bits(this->flags[flag / 32], 1UL << (flag % 32));
}

/*!
 * @brief Auto inscription
 */
bool auto_inscribe_item(ItemEntity &item, int idx)
{
    if (idx < 0 || autopick_list[idx].insc.empty() || item.is_inscribed()) {
        return false;
    }

    item.inscription = autopick_list[idx].insc;
    return true;
}

/*!
 * @brief 行数をカウントする
 * @return 行数
 */
int AutopickTextBody::count_line() const
{
    const auto begin = this->lines_list.begin();
    const auto it = std::find_if(begin, this->lines_list.end(), [](const auto &line) {
        return !line;
    });
    return std::distance(begin, it);
}
