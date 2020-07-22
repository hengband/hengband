#include "floor/floor-util.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/floor-town.h"
#include "system/floor-type-definition.h"

/*!
 * @brief 現在のマップ名を返す /
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return マップ名の文字列参照ポインタ
 */
concptr map_name(player_type *creature_ptr)
{
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    if (floor_ptr->inside_quest && is_fixed_quest_idx(floor_ptr->inside_quest) && (quest[floor_ptr->inside_quest].flags & QUEST_FLAG_PRESET))
        return _("クエスト", "Quest");
    else if (creature_ptr->wild_mode)
        return _("地上", "Surface");
    else if (floor_ptr->inside_arena)
        return _("アリーナ", "Arena");
    else if (creature_ptr->phase_out)
        return _("闘技場", "Monster Arena");
    else if (!floor_ptr->dun_level && creature_ptr->town_num)
        return town_info[creature_ptr->town_num].name;
    else
        return d_name + d_info[creature_ptr->dungeon_idx].name;
}
