#include "autopick/autopick-util.h"
#include "autopick/autopick-menu-data-table.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "game-option/input-options.h"
#include "main/sound-of-music.h"
#include "monster-race/race-indice-types.h"
#include "object-enchant/item-feeling.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "util/quarks.h"

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

/*!
 * @brief Free memory of lines_list.
 */
void free_text_lines(std::vector<concptr> &lines_list)
{
    for (int lines = 0; lines_list[lines]; lines++) {
        string_free(lines_list[lines]);
    }

    lines_list.clear();
}

/*!
 * @brief Find a command by 'key'.
 */
int get_com_id(char key)
{
    for (int i = 0; menu_data[i].name; i++) {
        if (menu_data[i].key == key) {
            return menu_data[i].com_id;
        }
    }

    return 0;
}

/*!
 * @brief Auto inscription
 */
void auto_inscribe_item(PlayerType *player_ptr, ItemEntity *o_ptr, int idx)
{
    if (idx < 0 || autopick_list[idx].insc.empty()) {
        return;
    }

    if (!o_ptr->is_inscribed()) {
        o_ptr->inscription = autopick_list[idx].insc;
    }

    player_ptr->window_flags |= (PW_EQUIP | PW_INVEN);
    player_ptr->update |= (PU_BONUS);
    player_ptr->update |= (PU_COMBINE);
}

/*!
 * @brief 行数をカウントする
 * @param tb text_body_type
 * @return 行数
 */
int count_line(text_body_type *tb)
{
    int num_lines;
    for (num_lines = 0; tb->lines_list[num_lines]; num_lines++) {
        ;
    }

    return num_lines;
}
