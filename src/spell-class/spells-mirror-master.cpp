#include "spell-class/spells-mirror-master.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "game-option/map-screen-options.h"
#include "grid/grid.h"
#include "monster/monster-update.h"
#include "spell-kind/spells-teleport.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "target/grid-selector.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

SpellsMirrorMaster::SpellsMirrorMaster(PlayerType *player_ptr)
    : player_ptr(player_ptr)
{
}

void SpellsMirrorMaster::remove_mirror(int y, int x)
{
    auto *g_ptr = &this->player_ptr->current_floor_ptr->grid_array[y][x];
    reset_bits(g_ptr->info, CAVE_OBJECT);
    g_ptr->mimic = 0;
    if (d_info[this->player_ptr->dungeon_idx].flags.has(DungeonFeatureType::DARKNESS)) {
        g_ptr->info &= ~(CAVE_GLOW);
        if (!view_torch_grids) {
            g_ptr->info &= ~(CAVE_MARK);
        }

        if (g_ptr->m_idx) {
            update_monster(this->player_ptr, g_ptr->m_idx, false);
        }

        update_local_illumination(this->player_ptr, y, x);
    }

    note_spot(this->player_ptr, y, x);
    lite_spot(this->player_ptr, y, x);
}

/*!
 * @brief 全鏡の消去 / Remove all mirrors in this floor
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param explode 爆発処理を伴うならばTRUE
 */
void SpellsMirrorMaster::remove_all_mirrors(bool explode)
{
    for (auto x = 0; x < this->player_ptr->current_floor_ptr->width; x++) {
        for (auto y = 0; y < this->player_ptr->current_floor_ptr->height; y++) {
            if (!this->player_ptr->current_floor_ptr->grid_array[y][x].is_mirror()) {
                continue;
            }

            this->remove_mirror(y, x);
            if (!explode) {
                continue;
            }

            constexpr BIT_FLAGS projection = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP | PROJECT_NO_HANGEKI;
            project(this->player_ptr, 0, 2, y, x, this->player_ptr->lev / 2 + 5, AttributeType::SHARDS, projection);
        }
    }
}

/*!
 * @brief 鏡抜け処理のメインルーチン /
 * Mirror Master's Dimension Door
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return ターンを消費した場合TRUEを返す
 */
bool SpellsMirrorMaster::mirror_tunnel()
{
    int x;
    int y;
    if (!tgt_pt(this->player_ptr, &x, &y)) {
        return false;
    }

    if (exe_dimension_door(this->player_ptr, x, y)) {
        return true;
    }

    msg_print(_("鏡の世界をうまく通れなかった！", "You could not enter the mirror!"));
    return true;
}
