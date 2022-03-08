/*!
 * @brief 鏡使いの鏡魔法効果処理
 * @date 2022/03/07
 * @author Hourier
 * @todo 作りかけの部分複数あり
 */

#include "spell-class/spells-mirror-master.h"
#include "core/player-redraw-types.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-monster.h"
#include "effect/effect-processor.h"
#include "floor/cave.h"
#include "game-option/map-screen-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "monster/monster-update.h"
#include "pet/pet-util.h"
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
        reset_bits(g_ptr->info, CAVE_GLOW);
        if (!view_torch_grids) {
            reset_bits(g_ptr->info, CAVE_MARK);
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
    const auto *floor_ptr = this->player_ptr->current_floor_ptr;
    for (auto x = 0; x < floor_ptr->width; x++) {
        for (auto y = 0; y < floor_ptr->height; y++) {
            if (!floor_ptr->grid_array[y][x].is_mirror()) {
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

/*!
 * @brief 鏡設置処理
 * @return 実際に設置が行われた場合TRUEを返す
 */
bool SpellsMirrorMaster::place_mirror()
{
    auto y = this->player_ptr->y;
    auto x = this->player_ptr->x;
    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    if (!cave_clean_bold(floor_ptr, y, x)) {
        msg_print(_("床上のアイテムが呪文を跳ね返した。", "The object resists the spell."));
        return false;
    }

    /* Create a mirror */
    auto *g_ptr = &floor_ptr->grid_array[y][x];
    set_bits(g_ptr->info, CAVE_OBJECT);
    g_ptr->mimic = feat_mirror;

    /* Turn on the light */
    set_bits(g_ptr->info, CAVE_GLOW);

    note_spot(this->player_ptr, y, x);
    lite_spot(this->player_ptr, y, x);
    update_local_illumination(this->player_ptr, y, x);
    return true;
}

/*!
 * @brief 静水
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return ペットを操っている場合を除きTRUE
 */
bool SpellsMirrorMaster::mirror_concentration()
{
    if (total_friends) {
        msg_print(_("今はペットを操ることに集中していないと。", "Your pets demand all of your attention."));
        return false;
    }

    if (!this->player_ptr->current_floor_ptr->grid_array[this->player_ptr->y][this->player_ptr->x].is_mirror()) {
        msg_print(_("鏡の上でないと集中できない！", "There's no mirror here!"));
        return true;
    }

    msg_print(_("少し頭がハッキリした。", "You feel your head clear a little."));
    this->player_ptr->csp += (5 + this->player_ptr->lev * this->player_ptr->lev / 100);
    if (this->player_ptr->csp >= this->player_ptr->msp) {
        this->player_ptr->csp = this->player_ptr->msp;
        this->player_ptr->csp_frac = 0;
    }

    set_bits(this->player_ptr->redraw, PR_MANA);
    return true;
}

/*!
 * @brief 鏡魔法「鏡の封印」の効果処理
 * @param dam ダメージ量
 * @return 効果があったらTRUEを返す
 */
void SpellsMirrorMaster::seal_of_mirror(const int dam)
{
    const auto *floor_ptr = this->player_ptr->current_floor_ptr;
    for (auto x = 0; x < floor_ptr->width; x++) {
        for (auto y = 0; y < floor_ptr->height; y++) {
            const auto &g_ref = floor_ptr->grid_array[y][x];
            if (!g_ref.is_mirror()) {
                continue;
            }

            constexpr BIT_FLAGS flags = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP;
            if (!affect_monster(this->player_ptr, 0, 0, y, x, dam, AttributeType::GENOCIDE, flags, true)) {
                continue;
            }

            if (g_ref.m_idx == 0) {
                this->remove_mirror(y, x);
            }
        }
    }
}
