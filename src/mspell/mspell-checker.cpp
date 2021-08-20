/*!
 * @brief モンスター魔法の実装 / Monster spells (attack player)
 * @date 2014/01/17
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * 2014 Deskull rearranged comment for Doxygen.
 */

#include "mspell/mspell-checker.h"
#include "blue-magic/blue-magic-checker.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "floor/line-of-sight.h"
#include "monster-floor/monster-move.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-ability-mask.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "mspell/assign-monster-spell.h"
#include "mspell/improper-mspell-remover.h"
#include "mspell/mspell-judgement.h"
#include "mspell/mspell-learn-checker.h"
#include "mspell/mspell-selector.h"
#include "mspell/mspell-util.h"
#include "object-enchant/object-curse.h"
#include "player/attack-defense-types.h"
#include "player/player-class.h"
#include "player/player-race-types.h"
#include "spell-kind/spells-world.h"
#include "spell-realm/spells-hex.h"
#include "spell/range-calc.h"
#include "spell/spell-types.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief モンスターにとって所定の地点が召還に相応しい地点かどうかを返す。 /
 * Determine if there is a space near the player in which a summoned creature can appear
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y1 判定を行いたいマスのY座標
 * @param x1 判定を行いたいマスのX座標
 * @return 召還に相応しいならばTRUEを返す
 */
bool summon_possible(player_type *target_ptr, POSITION y1, POSITION x1)
{
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    for (POSITION y = y1 - 2; y <= y1 + 2; y++) {
        for (POSITION x = x1 - 2; x <= x1 + 2; x++) {
            if (!in_bounds(floor_ptr, y, x))
                continue;

            if (distance(y1, x1, y, x) > 2)
                continue;

            if (pattern_tile(floor_ptr, y, x))
                continue;

            if (is_cave_empty_bold(target_ptr, y, x) && projectable(target_ptr, y1, x1, y, x) && projectable(target_ptr, y, x, y1, x1))
                return true;
        }
    }

    return false;
}

/*!
 * @brief モンスターにとって死者復活を行うべき状態かどうかを返す /
 * Determine if there is a space near the player in which a summoned creature can appear
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_ptr 判定を行いたいモンスターの構造体参照ポインタ
 * @return 死者復活が有効な状態ならばTRUEを返す。
 */
bool raise_possible(player_type *target_ptr, monster_type *m_ptr)
{
    POSITION y = m_ptr->fy;
    POSITION x = m_ptr->fx;
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    for (POSITION xx = x - 5; xx <= x + 5; xx++) {
        grid_type *g_ptr;
        for (POSITION yy = y - 5; yy <= y + 5; yy++) {
            if (distance(y, x, yy, xx) > 5)
                continue;
            if (!los(target_ptr, y, x, yy, xx))
                continue;
            if (!projectable(target_ptr, y, x, yy, xx))
                continue;

            g_ptr = &floor_ptr->grid_array[yy][xx];
            for (const auto this_o_idx : g_ptr->o_idx_list) {
                object_type *o_ptr = &floor_ptr->o_list[this_o_idx];
                if (o_ptr->tval == TV_CORPSE) {
                    if (!monster_has_hostile_align(target_ptr, m_ptr, 0, 0, &r_info[o_ptr->pval]))
                        return true;
                }
            }
        }
    }

    return false;
}

/*!
 * @brief モンスターにとってボルト型魔法が有効な状態かを返す /
 * Determine if a bolt spell will hit the player.
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y1 ボルト魔法発射地点のY座標
 * @param x1 ボルト魔法発射地点のX座標
 * @param y2 ボルト魔法目標地点のY座標
 * @param x2 ボルト魔法目標地点のX座標
 * @param is_friend モンスターがプレイヤーに害意を持たない(ペットか友好的)ならばTRUEをつける
 * @return ボルト型魔法が有効ならばTRUEを返す。
 * @details
 * Originally, it was possible for a friendly to shoot another friendly.\n
 * Change it so a "clean shot" means no equally friendly monster is\n
 * between the attacker and target.\n
 *\n
 * This is exactly like "projectable", but it will\n
 * return FALSE if a monster is in the way.\n
 * no equally friendly monster is\n
 * between the attacker and target.\n
 */
bool clean_shot(player_type *target_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2, bool is_friend)
{
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    uint16_t grid_g[512];
    int grid_n = projection_path(target_ptr, grid_g, get_max_range(target_ptr), y1, x1, y2, x2, 0);
    if (!grid_n)
        return false;

    POSITION y = get_grid_y(grid_g[grid_n - 1]);
    POSITION x = get_grid_x(grid_g[grid_n - 1]);
    if ((y != y2) || (x != x2))
        return false;

    for (int i = 0; i < grid_n; i++) {
        y = get_grid_y(grid_g[i]);
        x = get_grid_x(grid_g[i]);

        if ((floor_ptr->grid_array[y][x].m_idx > 0) && !((y == y2) && (x == x2))) {
            monster_type *m_ptr = &floor_ptr->m_list[floor_ptr->grid_array[y][x].m_idx];
            if (is_friend == is_pet(m_ptr)) {
                return false;
            }
        }

        if (player_bold(target_ptr, y, x) && is_friend)
            return false;
    }

    return true;
}

/*!
 * @brief モンスターのボルト型魔法処理 /
 * Cast a bolt at the player Stop if we hit a monster Affect monsters and the player
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスターのID
 * @param y 目標のY座標
 * @param x 目標のX座標
 * @param typ 効果属性ID
 * @param dam_hp 威力
 * @param monspell モンスター魔法のID
 * @param target_type モンスターからモンスターへ撃つならMONSTER_TO_MONSTER、モンスターからプレイヤーならMONSTER_TO_PLAYER
 */
ProjectResult bolt(player_type *target_ptr, MONSTER_IDX m_idx, POSITION y, POSITION x, EFFECT_ID typ, int dam_hp, int target_type)
{
    BIT_FLAGS flg = 0;
    switch (target_type) {
    case MONSTER_TO_MONSTER:
        flg = PROJECT_STOP | PROJECT_KILL;
        break;
    case MONSTER_TO_PLAYER:
        flg = PROJECT_STOP | PROJECT_KILL | PROJECT_PLAYER;
        break;
    }

    if (typ != GF_ARROW)
        flg |= PROJECT_REFLECTABLE;

    return project(target_ptr, m_idx, 0, y, x, dam_hp, typ, flg);
}

/*!
 * @brief モンスターのビーム型魔法処理 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスターのID
 * @param y 目標のY座標
 * @param x 目標のX座標
 * @param typ 効果属性ID
 * @param dam_hp 威力
 * @param monspell モンスター魔法のID
 * @param target_type モンスターからモンスターへ撃つならMONSTER_TO_MONSTER、モンスターからプレイヤーならMONSTER_TO_PLAYER
 */
ProjectResult beam(player_type *target_ptr, MONSTER_IDX m_idx, POSITION y, POSITION x, EFFECT_ID typ, int dam_hp, int target_type)
{
    BIT_FLAGS flg = 0;
    switch (target_type) {
    case MONSTER_TO_MONSTER:
        flg = PROJECT_BEAM | PROJECT_KILL | PROJECT_THRU;
        break;
    case MONSTER_TO_PLAYER:
        flg = PROJECT_BEAM | PROJECT_KILL | PROJECT_THRU | PROJECT_PLAYER;
        break;
    }

    return project(target_ptr, m_idx, 0, y, x, dam_hp, typ, flg);
}

/*!
 * @brief モンスターのボール型＆ブレス型魔法処理 /
 * Cast a breath (or ball) attack at the player Pass over any monsters that may be in the way Affect grids, objects, monsters, and the player
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 目標地点のY座標
 * @param x 目標地点のX座標
 * @param m_idx モンスターのID
 * @param typ 効果属性ID
 * @param dam_hp 威力
 * @param rad 半径
 * @param breath
 * @param monspell モンスター魔法のID
 * @param target_type モンスターからモンスターへ撃つならMONSTER_TO_MONSTER、モンスターからプレイヤーならMONSTER_TO_PLAYER
 */
ProjectResult breath(
    player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, EFFECT_ID typ, int dam_hp, POSITION rad, bool breath, int target_type)
{
    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    BIT_FLAGS flg = 0x00;
    switch (target_type) {
    case MONSTER_TO_MONSTER:
        flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
        break;
    case MONSTER_TO_PLAYER:
        flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_PLAYER;
        break;
    }

    if ((rad < 1) && breath)
        rad = (r_ptr->flags2 & (RF2_POWERFUL)) ? 3 : 2;

    if (breath)
        rad = 0 - rad;

    switch (typ) {
    case GF_ROCKET:
        flg |= PROJECT_STOP;
        break;
    case GF_DRAIN_MANA:
    case GF_MIND_BLAST:
    case GF_BRAIN_SMASH:
    case GF_CAUSE_1:
    case GF_CAUSE_2:
    case GF_CAUSE_3:
    case GF_CAUSE_4:
    case GF_HAND_DOOM:
        flg |= (PROJECT_HIDE | PROJECT_AIMED);
        break;
    }

    return project(target_ptr, m_idx, rad, y, x, dam_hp, typ, flg);
}

/*!
 * @brief ID値が非魔術的な特殊技能かどうかを返す /
 * Return TRUE if a spell is inate spell.
 * @param spell 判定対象のID
 * @return 非魔術的な特殊技能ならばTRUEを返す。
 */
bool spell_is_inate(RF_ABILITY spell)
{
    return RF_ABILITY_NOMAGIC_MASK.has(spell);
}
