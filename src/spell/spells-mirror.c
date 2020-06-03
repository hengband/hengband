#include "spell/spells-mirror.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-monster.h"
#include "floor/floor.h"
#include "spell/spells-type.h"

/*!
 * @brief 鏡魔法「鏡の封印」の効果処理
 * @param dam ダメージ量
 * @return 効果があったらTRUEを返す
 */
void seal_of_mirror(player_type *caster_ptr, HIT_POINT dam)
{
    for (POSITION x = 0; x < caster_ptr->current_floor_ptr->width; x++) {
        for (POSITION y = 0; y < caster_ptr->current_floor_ptr->height; y++) {
            if (!is_mirror_grid(&caster_ptr->current_floor_ptr->grid_array[y][x]))
                continue;

            if (!affect_monster(caster_ptr, 0, 0, y, x, dam, GF_GENOCIDE, (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP), TRUE))
                continue;

            if (!caster_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
                remove_mirror(caster_ptr, y, x);
            }
        }
    }
}
