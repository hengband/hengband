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
#include "effect/attribute-types.h"
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
#include "player-info/class-info.h"
#include "player-info/race-types.h"
#include "player/attack-defense-types.h"
#include "spell-kind/spells-world.h"
#include "spell-realm/spells-hex.h"
#include "spell/range-calc.h"
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
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y1 判定を行いたいマスのY座標
 * @param x1 判定を行いたいマスのX座標
 * @return 召還に相応しいならばTRUEを返す
 */
bool summon_possible(PlayerType *player_ptr, POSITION y1, POSITION x1)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    for (POSITION y = y1 - 2; y <= y1 + 2; y++) {
        for (POSITION x = x1 - 2; x <= x1 + 2; x++) {
            if (!in_bounds(floor_ptr, y, x)) {
                continue;
            }

            if (distance(y1, x1, y, x) > 2) {
                continue;
            }

            if (pattern_tile(floor_ptr, y, x)) {
                continue;
            }

            if (is_cave_empty_bold(player_ptr, y, x) && projectable(player_ptr, y1, x1, y, x) && projectable(player_ptr, y, x, y1, x1)) {
                return true;
            }
        }
    }

    return false;
}

/*!
 * @brief モンスターにとって死者復活を行うべき状態かどうかを返す /
 * Determine if there is a space near the player in which a summoned creature can appear
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_ptr 判定を行いたいモンスターの構造体参照ポインタ
 * @return 死者復活が有効な状態ならばTRUEを返す。
 */
bool raise_possible(PlayerType *player_ptr, monster_type *m_ptr)
{
    POSITION y = m_ptr->fy;
    POSITION x = m_ptr->fx;
    auto *floor_ptr = player_ptr->current_floor_ptr;
    for (POSITION xx = x - 5; xx <= x + 5; xx++) {
        grid_type *g_ptr;
        for (POSITION yy = y - 5; yy <= y + 5; yy++) {
            if (distance(y, x, yy, xx) > 5) {
                continue;
            }
            if (!los(player_ptr, y, x, yy, xx)) {
                continue;
            }
            if (!projectable(player_ptr, y, x, yy, xx)) {
                continue;
            }

            g_ptr = &floor_ptr->grid_array[yy][xx];
            for (const auto this_o_idx : g_ptr->o_idx_list) {
                auto *o_ptr = &floor_ptr->o_list[this_o_idx];
                if (o_ptr->tval == ItemKindType::CORPSE) {
                    auto corpse_r_idx = i2enum<MonsterRaceId>(o_ptr->pval);
                    if (!monster_has_hostile_align(player_ptr, m_ptr, 0, 0, &monraces_info[corpse_r_idx])) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

/*!
 * @brief モンスターにとってボルト型魔法が有効な状態かを返す /
 * Determine if a bolt spell will hit the player.
 * @param player_ptr プレイヤーへの参照ポインタ
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
bool clean_shot(PlayerType *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2, bool is_friend)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    projection_path grid_g(player_ptr, get_max_range(player_ptr), y1, x1, y2, x2, 0);
    if (grid_g.path_num() == 0) {
        return false;
    }

    const auto [last_y, last_x] = grid_g.back();
    if ((last_y != y2) || (last_x != x2)) {
        return false;
    }

    for (const auto &[y, x] : grid_g) {
        if ((floor_ptr->grid_array[y][x].m_idx > 0) && (y != y2 || x != x2)) {
            auto *m_ptr = &floor_ptr->m_list[floor_ptr->grid_array[y][x].m_idx];
            if (is_friend == m_ptr->is_pet()) {
                return false;
            }
        }

        if (player_bold(player_ptr, y, x) && is_friend) {
            return false;
        }
    }

    return true;
}

/*!
 * @brief モンスターのボルト型魔法処理 /
 * Cast a bolt at the player Stop if we hit a monster Affect monsters and the player
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターのID
 * @param y 目標のY座標
 * @param x 目標のX座標
 * @param typ 効果属性ID
 * @param dam_hp 威力
 * @param monspell モンスター魔法のID
 * @param target_type モンスターからモンスターへ撃つならMONSTER_TO_MONSTER、モンスターからプレイヤーならMONSTER_TO_PLAYER
 */
ProjectResult bolt(PlayerType *player_ptr, MONSTER_IDX m_idx, POSITION y, POSITION x, AttributeType typ, int dam_hp, int target_type)
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

    if (typ != AttributeType::MONSTER_SHOOT) {
        flg |= PROJECT_REFLECTABLE;
    }

    return project(player_ptr, m_idx, 0, y, x, dam_hp, typ, flg);
}

/*!
 * @brief モンスターのビーム型魔法処理 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターのID
 * @param y 目標のY座標
 * @param x 目標のX座標
 * @param typ 効果属性ID
 * @param dam_hp 威力
 * @param monspell モンスター魔法のID
 * @param target_type モンスターからモンスターへ撃つならMONSTER_TO_MONSTER、モンスターからプレイヤーならMONSTER_TO_PLAYER
 */
ProjectResult beam(PlayerType *player_ptr, MONSTER_IDX m_idx, POSITION y, POSITION x, AttributeType typ, int dam_hp, int target_type)
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

    return project(player_ptr, m_idx, 0, y, x, dam_hp, typ, flg);
}

ProjectResult ball(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, AttributeType typ, int dam_hp, POSITION rad, int target_type)
{
    BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
    if (target_type == MONSTER_TO_PLAYER) {
        flg |= PROJECT_PLAYER;
    }

    return project(player_ptr, m_idx, rad, y, x, dam_hp, typ, flg);
}

/*!
 * @brief モンスターのボール型＆ブレス型魔法処理 /
 * Cast a breath (or ball) attack at the player Pass over any monsters that may be in the way Affect grids, objects, monsters, and the player
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 目標地点のY座標
 * @param x 目標地点のX座標
 * @param m_idx モンスターのID
 * @param typ 効果属性ID
 * @param dam_hp 威力
 * @param rad 半径
 * @param monspell モンスター魔法のID
 * @param target_type モンスターからモンスターへ撃つならMONSTER_TO_MONSTER、モンスターからプレイヤーならMONSTER_TO_PLAYER
 */
ProjectResult breath(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, AttributeType typ, int dam_hp, POSITION rad, int target_type)
{
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    auto *r_ptr = &monraces_info[m_ptr->r_idx];
    BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_BREATH;
    if (target_type == MONSTER_TO_PLAYER) {
        flg |= PROJECT_PLAYER;
    }

    if (rad < 1) {
        rad = (r_ptr->flags2 & (RF2_POWERFUL)) ? 3 : 2;
    }

    return project(player_ptr, m_idx, rad, y, x, dam_hp, typ, flg);
}

ProjectResult pointed(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, AttributeType typ, int dam_hp, int target_type)
{
    BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_HIDE | PROJECT_AIMED;
    if (target_type == MONSTER_TO_PLAYER) {
        flg |= PROJECT_PLAYER;
    }

    return project(player_ptr, m_idx, 0, y, x, dam_hp, typ, flg);
}

ProjectResult rocket(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, AttributeType typ, int dam_hp, POSITION rad, int target_type)
{
    BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_STOP;
    if (target_type == MONSTER_TO_PLAYER) {
        flg |= PROJECT_PLAYER;
    }

    return project(player_ptr, m_idx, rad, y, x, dam_hp, typ, flg);
}

/*!
 * @brief ID値が非魔術的な特殊技能かどうかを返す /
 * Return TRUE if a spell is inate spell.
 * @param spell 判定対象のID
 * @return 非魔術的な特殊技能ならばTRUEを返す。
 */
bool spell_is_inate(MonsterAbilityType spell)
{
    return RF_ABILITY_NOMAGIC_MASK.has(spell);
}
