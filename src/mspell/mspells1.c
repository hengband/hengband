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

#include "mspell/mspells1.h"
#include "blue-magic/blue-magic-checker.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "effect/effect-characteristics.h"
#include "floor/cave.h"
#include "floor/floor.h"
#include "game-option/birth-options.h"
#include "grid/grid.h"
#include "io/targeting.h"
#include "monster-floor/monster-move.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-ability1.h"
#include "monster-race/race-flags-ability2.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags4.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-flags9.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/smart-learn-types.h"
#include "mspell/assign-monster-spell.h"
#include "mspell/mspell-learn-checker.h"
#include "mspell/mspell-mask-definitions.h"
#include "mspell/mspell-util.h"
#include "mspell/mspell-judgement.h"
#include "object-enchant/object-curse.h"
#include "player/attack-defense-types.h"
#include "player/player-class.h"
#include "player/player-race-types.h"
#include "player/player-race.h"
#include "spell-kind/spells-world.h"
#include "spell-realm/spells-hex.h"
#include "spell/process-effect.h"
#include "spell/range-calc.h"
#include "spell/spell-types.h"
#include "status/element-resistance.h"
#include "system/floor-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"

#define DO_SPELL_NONE 0
#define DO_SPELL_BR_LITE 1
#define DO_SPELL_BR_DISI 2
#define DO_SPELL_BA_LITE 3

/*!
 * @brief モンスターがプレイヤーの弱点をついた選択を取るかどうかの判定 /
 * Internal probability routine
 * @param r_ptr モンスター種族の構造体参照ポインタ
 * @param prob 基本確率(%)
 * @return 適した選択を取るならばTRUEを返す。
 */
static bool int_outof(monster_race *r_ptr, PERCENTAGE prob)
{
    if (!(r_ptr->flags2 & RF2_SMART))
        prob = prob / 2;

    return (randint0(100) < prob);
}

/*!
 * @brief モンスターの魔法一覧から戦術的に適さない魔法を除外する /
 * Remove the "bad" spells from a spell list
 * @param m_idx モンスターの構造体参照ポインタ
 * @param f4p モンスター魔法のフラグリスト1
 * @param f5p モンスター魔法のフラグリスト2
 * @param f6p モンスター魔法のフラグリスト3
 * @return なし
 */
static void remove_bad_spells(MONSTER_IDX m_idx, player_type *target_ptr, u32b *f4p, u32b *f5p, u32b *f6p)
{
    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    u32b f4 = (*f4p);
    u32b f5 = (*f5p);
    u32b f6 = (*f6p);
    u32b smart = 0L;
    if (r_ptr->flags2 & RF2_STUPID)
        return;

    if (!smart_cheat && !smart_learn)
        return;

    if (smart_learn) {
        if (m_ptr->smart && (randint0(100) < 1))
            m_ptr->smart &= (SM_FRIENDLY | SM_PET | SM_CLONED);

        smart = m_ptr->smart;
    }

    if (smart_cheat) {
        if (target_ptr->resist_acid)
            smart |= (SM_RES_ACID);
        if (is_oppose_acid(target_ptr))
            smart |= (SM_OPP_ACID);
        if (target_ptr->immune_acid)
            smart |= (SM_IMM_ACID);
        if (target_ptr->resist_elec)
            smart |= (SM_RES_ELEC);
        if (is_oppose_elec(target_ptr))
            smart |= (SM_OPP_ELEC);
        if (target_ptr->immune_elec)
            smart |= (SM_IMM_ELEC);
        if (target_ptr->resist_fire)
            smart |= (SM_RES_FIRE);
        if (is_oppose_fire(target_ptr))
            smart |= (SM_OPP_FIRE);
        if (target_ptr->immune_fire)
            smart |= (SM_IMM_FIRE);
        if (target_ptr->resist_cold)
            smart |= (SM_RES_COLD);
        if (is_oppose_cold(target_ptr))
            smart |= (SM_OPP_COLD);
        if (target_ptr->immune_cold)
            smart |= (SM_IMM_COLD);
        if (target_ptr->resist_pois)
            smart |= (SM_RES_POIS);
        if (is_oppose_pois(target_ptr))
            smart |= (SM_OPP_POIS);

        if (target_ptr->resist_neth)
            smart |= (SM_RES_NETH);
        if (target_ptr->resist_lite)
            smart |= (SM_RES_LITE);
        if (target_ptr->resist_dark)
            smart |= (SM_RES_DARK);
        if (target_ptr->resist_fear)
            smart |= (SM_RES_FEAR);
        if (target_ptr->resist_conf)
            smart |= (SM_RES_CONF);
        if (target_ptr->resist_chaos)
            smart |= (SM_RES_CHAOS);
        if (target_ptr->resist_disen)
            smart |= (SM_RES_DISEN);
        if (target_ptr->resist_blind)
            smart |= (SM_RES_BLIND);
        if (target_ptr->resist_nexus)
            smart |= (SM_RES_NEXUS);
        if (target_ptr->resist_sound)
            smart |= (SM_RES_SOUND);
        if (target_ptr->resist_shard)
            smart |= (SM_RES_SHARD);
        if (target_ptr->reflect)
            smart |= (SM_IMM_REFLECT);

        if (target_ptr->free_act)
            smart |= (SM_IMM_FREE);
        if (!target_ptr->msp)
            smart |= (SM_IMM_MANA);
    }

    if (!smart)
        return;

    if (smart & SM_IMM_ACID) {
        f4 &= ~(RF4_BR_ACID);
        f5 &= ~(RF5_BA_ACID);
        f5 &= ~(RF5_BO_ACID);
    } else if ((smart & (SM_OPP_ACID)) && (smart & (SM_RES_ACID))) {
        if (int_outof(r_ptr, 80))
            f4 &= ~(RF4_BR_ACID);
        if (int_outof(r_ptr, 80))
            f5 &= ~(RF5_BA_ACID);
        if (int_outof(r_ptr, 80))
            f5 &= ~(RF5_BO_ACID);
    } else if ((smart & (SM_OPP_ACID)) || (smart & (SM_RES_ACID))) {
        if (int_outof(r_ptr, 30))
            f4 &= ~(RF4_BR_ACID);
        if (int_outof(r_ptr, 30))
            f5 &= ~(RF5_BA_ACID);
        if (int_outof(r_ptr, 30))
            f5 &= ~(RF5_BO_ACID);
    }

    if (smart & (SM_IMM_ELEC)) {
        f4 &= ~(RF4_BR_ELEC);
        f5 &= ~(RF5_BA_ELEC);
        f5 &= ~(RF5_BO_ELEC);
    } else if ((smart & (SM_OPP_ELEC)) && (smart & (SM_RES_ELEC))) {
        if (int_outof(r_ptr, 80))
            f4 &= ~(RF4_BR_ELEC);
        if (int_outof(r_ptr, 80))
            f5 &= ~(RF5_BA_ELEC);
        if (int_outof(r_ptr, 80))
            f5 &= ~(RF5_BO_ELEC);
    } else if ((smart & (SM_OPP_ELEC)) || (smart & (SM_RES_ELEC))) {
        if (int_outof(r_ptr, 30))
            f4 &= ~(RF4_BR_ELEC);
        if (int_outof(r_ptr, 30))
            f5 &= ~(RF5_BA_ELEC);
        if (int_outof(r_ptr, 30))
            f5 &= ~(RF5_BO_ELEC);
    }

    if (smart & (SM_IMM_FIRE)) {
        f4 &= ~(RF4_BR_FIRE);
        f5 &= ~(RF5_BA_FIRE);
        f5 &= ~(RF5_BO_FIRE);
    } else if ((smart & (SM_OPP_FIRE)) && (smart & (SM_RES_FIRE))) {
        if (int_outof(r_ptr, 80))
            f4 &= ~(RF4_BR_FIRE);
        if (int_outof(r_ptr, 80))
            f5 &= ~(RF5_BA_FIRE);
        if (int_outof(r_ptr, 80))
            f5 &= ~(RF5_BO_FIRE);
    } else if ((smart & (SM_OPP_FIRE)) || (smart & (SM_RES_FIRE))) {
        if (int_outof(r_ptr, 30))
            f4 &= ~(RF4_BR_FIRE);
        if (int_outof(r_ptr, 30))
            f5 &= ~(RF5_BA_FIRE);
        if (int_outof(r_ptr, 30))
            f5 &= ~(RF5_BO_FIRE);
    }

    if (smart & (SM_IMM_COLD)) {
        f4 &= ~(RF4_BR_COLD);
        f5 &= ~(RF5_BA_COLD);
        f5 &= ~(RF5_BO_COLD);
        f5 &= ~(RF5_BO_ICEE);
    } else if ((smart & (SM_OPP_COLD)) && (smart & (SM_RES_COLD))) {
        if (int_outof(r_ptr, 80))
            f4 &= ~(RF4_BR_COLD);
        if (int_outof(r_ptr, 80))
            f5 &= ~(RF5_BA_COLD);
        if (int_outof(r_ptr, 80))
            f5 &= ~(RF5_BO_COLD);
        if (int_outof(r_ptr, 80))
            f5 &= ~(RF5_BO_ICEE);
    } else if ((smart & (SM_OPP_COLD)) || (smart & (SM_RES_COLD))) {
        if (int_outof(r_ptr, 30))
            f4 &= ~(RF4_BR_COLD);
        if (int_outof(r_ptr, 30))
            f5 &= ~(RF5_BA_COLD);
        if (int_outof(r_ptr, 30))
            f5 &= ~(RF5_BO_COLD);
        if (int_outof(r_ptr, 20))
            f5 &= ~(RF5_BO_ICEE);
    }

    if ((smart & (SM_OPP_POIS)) && (smart & (SM_RES_POIS))) {
        if (int_outof(r_ptr, 80))
            f4 &= ~(RF4_BR_POIS);
        if (int_outof(r_ptr, 80))
            f5 &= ~(RF5_BA_POIS);
        if (int_outof(r_ptr, 60))
            f4 &= ~(RF4_BA_NUKE);
        if (int_outof(r_ptr, 60))
            f4 &= ~(RF4_BR_NUKE);
    } else if ((smart & (SM_OPP_POIS)) || (smart & (SM_RES_POIS))) {
        if (int_outof(r_ptr, 30))
            f4 &= ~(RF4_BR_POIS);
        if (int_outof(r_ptr, 30))
            f5 &= ~(RF5_BA_POIS);
    }

    if (smart & (SM_RES_NETH)) {
        if (is_specific_player_race(target_ptr, RACE_SPECTRE)) {
            f4 &= ~(RF4_BR_NETH);
            f5 &= ~(RF5_BA_NETH);
            f5 &= ~(RF5_BO_NETH);
        } else {
            if (int_outof(r_ptr, 20))
                f4 &= ~(RF4_BR_NETH);
            if (int_outof(r_ptr, 50))
                f5 &= ~(RF5_BA_NETH);
            if (int_outof(r_ptr, 50))
                f5 &= ~(RF5_BO_NETH);
        }
    }

    if (smart & (SM_RES_LITE)) {
        if (int_outof(r_ptr, 50))
            f4 &= ~(RF4_BR_LITE);
        if (int_outof(r_ptr, 50))
            f5 &= ~(RF5_BA_LITE);
    }

    if (smart & (SM_RES_DARK)) {
        if (is_specific_player_race(target_ptr, RACE_VAMPIRE)) {
            f4 &= ~(RF4_BR_DARK);
            f5 &= ~(RF5_BA_DARK);
        } else {
            if (int_outof(r_ptr, 50))
                f4 &= ~(RF4_BR_DARK);
            if (int_outof(r_ptr, 50))
                f5 &= ~(RF5_BA_DARK);
        }
    }

    if (smart & (SM_RES_FEAR)) {
        f5 &= ~(RF5_SCARE);
    }

    if (smart & (SM_RES_CONF)) {
        f5 &= ~(RF5_CONF);
        if (int_outof(r_ptr, 50))
            f4 &= ~(RF4_BR_CONF);
    }

    if (smart & (SM_RES_CHAOS)) {
        if (int_outof(r_ptr, 20))
            f4 &= ~(RF4_BR_CHAO);
        if (int_outof(r_ptr, 50))
            f4 &= ~(RF4_BA_CHAO);
    }

    if (smart & (SM_RES_DISEN)) {
        if (int_outof(r_ptr, 40))
            f4 &= ~(RF4_BR_DISE);
    }

    if (smart & (SM_RES_BLIND)) {
        f5 &= ~(RF5_BLIND);
    }

    if (smart & (SM_RES_NEXUS)) {
        if (int_outof(r_ptr, 50))
            f4 &= ~(RF4_BR_NEXU);
        f6 &= ~(RF6_TELE_LEVEL);
    }

    if (smart & (SM_RES_SOUND)) {
        if (int_outof(r_ptr, 50))
            f4 &= ~(RF4_BR_SOUN);
    }

    if (smart & (SM_RES_SHARD)) {
        if (int_outof(r_ptr, 40))
            f4 &= ~(RF4_BR_SHAR);
    }

    if (smart & (SM_IMM_REFLECT)) {
        if (int_outof(r_ptr, 150))
            f5 &= ~(RF5_BO_COLD);
        if (int_outof(r_ptr, 150))
            f5 &= ~(RF5_BO_FIRE);
        if (int_outof(r_ptr, 150))
            f5 &= ~(RF5_BO_ACID);
        if (int_outof(r_ptr, 150))
            f5 &= ~(RF5_BO_ELEC);
        if (int_outof(r_ptr, 150))
            f5 &= ~(RF5_BO_NETH);
        if (int_outof(r_ptr, 150))
            f5 &= ~(RF5_BO_WATE);
        if (int_outof(r_ptr, 150))
            f5 &= ~(RF5_BO_MANA);
        if (int_outof(r_ptr, 150))
            f5 &= ~(RF5_BO_PLAS);
        if (int_outof(r_ptr, 150))
            f5 &= ~(RF5_BO_ICEE);
        if (int_outof(r_ptr, 150))
            f5 &= ~(RF5_MISSILE);
    }

    if (smart & (SM_IMM_FREE)) {
        f5 &= ~(RF5_HOLD);
        f5 &= ~(RF5_SLOW);
    }

    if (smart & (SM_IMM_MANA)) {
        f5 &= ~(RF5_DRAIN_MANA);
    }

    (*f4p) = f4;
    (*f5p) = f5;
    (*f6p) = f6;
}

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
    POSITION y, x;
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    for (y = y1 - 2; y <= y1 + 2; y++) {
        for (x = x1 - 2; x <= x1 + 2; x++) {
            if (!in_bounds(floor_ptr, y, x))
                continue;

            if (distance(y1, x1, y, x) > 2)
                continue;

            if (pattern_tile(floor_ptr, y, x))
                continue;

            if (is_cave_empty_bold(target_ptr, y, x) && projectable(target_ptr, y1, x1, y, x) && projectable(target_ptr, y, x, y1, x1))
                return TRUE;
        }
    }

    return FALSE;
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
            OBJECT_IDX this_o_idx, next_o_idx = 0;
            for (this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx) {
                object_type *o_ptr = &floor_ptr->o_list[this_o_idx];
                next_o_idx = o_ptr->next_o_idx;
                if (o_ptr->tval == TV_CORPSE) {
                    if (!monster_has_hostile_align(target_ptr, m_ptr, 0, 0, &r_info[o_ptr->pval]))
                        return TRUE;
                }
            }
        }
    }

    return FALSE;
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
    u16b grid_g[512];
    int grid_n = project_path(target_ptr, grid_g, get_max_range(target_ptr), y1, x1, y2, x2, 0);
    if (!grid_n)
        return FALSE;

    POSITION y = GRID_Y(grid_g[grid_n - 1]);
    POSITION x = GRID_X(grid_g[grid_n - 1]);
    if ((y != y2) || (x != x2))
        return FALSE;

    for (int i = 0; i < grid_n; i++) {
        y = GRID_Y(grid_g[i]);
        x = GRID_X(grid_g[i]);

        if ((floor_ptr->grid_array[y][x].m_idx > 0) && !((y == y2) && (x == x2))) {
            monster_type *m_ptr = &floor_ptr->m_list[floor_ptr->grid_array[y][x].m_idx];
            if (is_friend == is_pet(m_ptr)) {
                return FALSE;
            }
        }

        if (player_bold(target_ptr, y, x) && is_friend)
            return FALSE;
    }

    return TRUE;
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
 * @return なし
 */
void bolt(player_type *target_ptr, MONSTER_IDX m_idx, POSITION y, POSITION x, EFFECT_ID typ, int dam_hp, int monspell, int target_type)
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
    bool learnable = spell_learnable(target_ptr, m_idx);
    (void)project(target_ptr, m_idx, 0, y, x, dam_hp, typ, flg, (learnable ? monspell : -1));
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
 * @return なし
 */
void beam(player_type *target_ptr, MONSTER_IDX m_idx, POSITION y, POSITION x, EFFECT_ID typ, int dam_hp, int monspell, int target_type)
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

    bool learnable = spell_learnable(target_ptr, m_idx);
    (void)project(target_ptr, m_idx, 0, y, x, dam_hp, typ, flg, (learnable ? monspell : -1));
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
 * @return なし
 */
void breath(
    player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, EFFECT_ID typ, int dam_hp, POSITION rad, bool breath, int monspell, int target_type)
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

    bool learnable = spell_learnable(target_ptr, m_idx);
    (void)project(target_ptr, m_idx, rad, y, x, dam_hp, typ, flg, (learnable ? monspell : -1));
}

/*!
 * @brief ID値が正しいモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell is good for hurting the player (directly).
 * @param spell 判定対象のID
 * @return 正しいIDならばTRUEを返す。
 */
static bool spell_attack(byte spell)
{
    /* All RF4 spells hurt (except for shriek and dispel) */
    if (spell < 128 && spell > 98)
        return TRUE;

    /* Various "ball" spells */
    if (spell >= 128 && spell <= 128 + 8)
        return TRUE;

    /* "Cause wounds" and "bolt" spells */
    if (spell >= 128 + 12 && spell < 128 + 27)
        return TRUE;

    /* Hand of Doom */
    if (spell == 160 + 1)
        return TRUE;

    /* Psycho-Spear */
    if (spell == 160 + 11)
        return TRUE;

    /* Doesn't hurt */
    return FALSE;
}

/*!
 * @brief ID値が退避目的に適したモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell is good for escaping.
 * @param spell 判定対象のID
 * @return 適した魔法のIDならばTRUEを返す。
 */
static bool spell_escape(byte spell)
{
    /* Blink or Teleport */
    if (spell == 160 + 4 || spell == 160 + 5)
        return TRUE;

    /* Teleport the player away */
    if (spell == 160 + 9 || spell == 160 + 10)
        return TRUE;

    /* Isn't good for escaping */
    return FALSE;
}

/*!
 * @brief ID値が妨害目的に適したモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell is good for annoying the player.
 * @param spell 判定対象のID
 * @return 適した魔法のIDならばTRUEを返す。
 */
static bool spell_annoy(byte spell)
{
    /* Shriek */
    if (spell == 96 + 0)
        return TRUE;

    /* Brain smash, et al (added curses) */
    if (spell >= 128 + 9 && spell <= 128 + 14)
        return TRUE;

    /* Scare, confuse, blind, slow, paralyze */
    if (spell >= 128 + 27 && spell <= 128 + 31)
        return TRUE;

    /* Teleport to */
    if (spell == 160 + 8)
        return TRUE;

    /* Teleport level */
    if (spell == 160 + 10)
        return TRUE;

    /* Darkness, make traps, cause amnesia */
    if (spell >= 160 + 12 && spell <= 160 + 14)
        return TRUE;

    /* Doesn't annoy */
    return FALSE;
}

/*!
 * @brief ID値が召喚型のモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell is good for annoying the player.
 * @param spell 判定対象のID
 * @return 召喚型魔法のIDならばTRUEを返す。
 */
static bool spell_summon(byte spell)
{
    /* All summon spells */
    if (spell >= 160 + 16)
        return TRUE;

    /* Doesn't summon */
    return FALSE;
}

/*!
 * @brief ID値が死者復活処理かどうかを返す /
 * Return TRUE if a spell is good for annoying the player.
 * @param spell 判定対象のID
 * @return 死者復活の処理ならばTRUEを返す。
 */
static bool spell_raise(byte spell)
{
    /* All raise-dead spells */
    if (spell == 160 + 15)
        return TRUE;

    /* Doesn't summon */
    return FALSE;
}

/*!
 * @brief ID値が戦術的なモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell is good in a tactical situation.
 * @param spell 判定対象のID
 * @return 戦術的な魔法のIDならばTRUEを返す。
 */
static bool spell_tactic(byte spell)
{
    /* Blink */
    if (spell == 160 + 4)
        return TRUE;

    /* Not good */
    return FALSE;
}

/*!
 * @brief ID値が無敵化するモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell makes invulnerable.
 * @param spell 判定対象のID
 * @return 召喚型魔法のIDならばTRUEを返す。
 */
static bool spell_invulner(byte spell)
{
    /* Invulnerability */
    if (spell == 160 + 3)
        return TRUE;

    /* No invulnerability */
    return FALSE;
}

/*!
 * @brief ID値が加速するモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell hastes.
 * @param spell 判定対象のID
 * @return 召喚型魔法のIDならばTRUEを返す。
 */
static bool spell_haste(byte spell)
{
    /* Haste self */
    if (spell == 160 + 0)
        return TRUE;

    /* Not a haste spell */
    return FALSE;
}

/*!
 * @brief ID値が時間停止を行うモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell world.
 * @param spell 判定対象のID
 * @return 時間停止魔法のIDならばTRUEを返す。
 */
static bool spell_world(byte spell)
{
    if (spell == 160 + 6)
        return TRUE;
    return FALSE;
}

/*!
 * @brief ID値が特別効果のモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell special.
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param spell 判定対象のID
 * @return 特別効果魔法のIDならばTRUEを返す。
 */
static bool spell_special(player_type *target_ptr, byte spell)
{
    if (target_ptr->phase_out)
        return FALSE;
    if (spell == 160 + 7)
        return TRUE;
    return FALSE;
}

/*!
 * @brief ID値が光の剣のモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell psycho-spear.
 * @param spell 判定対象のID
 * @return 光の剣のIDならばTRUEを返す。
 */
static bool spell_psy_spe(byte spell)
{
    /* world */
    if (spell == 160 + 11)
        return TRUE;

    /* Not a haste spell */
    return FALSE;
}

/*!
 * @brief ID値が治癒魔法かどうかを返す /
 * Return TRUE if a spell is good for healing.
 * @param spell 判定対象のID
 * @return 治癒魔法のIDならばTRUEを返す。
 */
static bool spell_heal(byte spell)
{
    /* Heal */
    if (spell == 160 + 2)
        return TRUE;

    /* No healing */
    return FALSE;
}

/*!
 * @brief ID値が魔力消去かどうかを返す /
 * Return TRUE if a spell is good for dispel.
 * @param spell 判定対象のID
 * @return 魔力消去のIDならばTRUEを返す。
 */
static bool spell_dispel(byte spell)
{
    /* Dispel */
    if (spell == 96 + 2)
        return TRUE;

    /* No dispel */
    return FALSE;
}

/*!
 * todo 長過ぎる。切り分けが必要
 * @brief モンスターの魔法選択ルーチン
 * Have a monster choose a spell from a list of "useful" spells.
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスターの構造体配列ID
 * @param spells 候補魔法IDをまとめた配列
 * @param num spellsの長さ
 * @return 選択したモンスター魔法のID
 * @details
 * Note that this list does NOT include spells that will just hit\n
 * other monsters, and the list is restricted when the monster is\n
 * "desperate".  Should that be the job of this function instead?\n
 *\n
 * Stupid monsters will just pick a spell randomly.  Smart monsters\n
 * will choose more "intelligently".\n
 *\n
 * Use the helper functions above to put spells into categories.\n
 *\n
 * This function may well be an efficiency bottleneck.\n
 */
static int choose_attack_spell(player_type *target_ptr, MONSTER_IDX m_idx, byte spells[], byte num)
{
    byte escape[96], escape_num = 0;
    byte attack[96], attack_num = 0;
    byte summon[96], summon_num = 0;
    byte tactic[96], tactic_num = 0;
    byte annoy[96], annoy_num = 0;
    byte invul[96], invul_num = 0;
    byte haste[96], haste_num = 0;
    byte world[96], world_num = 0;
    byte special[96], special_num = 0;
    byte psy_spe[96], psy_spe_num = 0;
    byte raise[96], raise_num = 0;
    byte heal[96], heal_num = 0;
    byte dispel[96], dispel_num = 0;

    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    if (r_ptr->flags2 & (RF2_STUPID))
        return (spells[randint0(num)]);

    for (int i = 0; i < num; i++) {
        if (spell_escape(spells[i]))
            escape[escape_num++] = spells[i];

        if (spell_attack(spells[i]))
            attack[attack_num++] = spells[i];

        if (spell_summon(spells[i]))
            summon[summon_num++] = spells[i];

        if (spell_tactic(spells[i]))
            tactic[tactic_num++] = spells[i];

        if (spell_annoy(spells[i]))
            annoy[annoy_num++] = spells[i];

        if (spell_invulner(spells[i]))
            invul[invul_num++] = spells[i];

        if (spell_haste(spells[i]))
            haste[haste_num++] = spells[i];

        if (spell_world(spells[i]))
            world[world_num++] = spells[i];

        if (spell_special(target_ptr, spells[i]))
            special[special_num++] = spells[i];

        if (spell_psy_spe(spells[i]))
            psy_spe[psy_spe_num++] = spells[i];

        if (spell_raise(spells[i]))
            raise[raise_num++] = spells[i];

        if (spell_heal(spells[i]))
            heal[heal_num++] = spells[i];

        if (spell_dispel(spells[i]))
            dispel[dispel_num++] = spells[i];
    }

    if (world_num && (randint0(100) < 15) && !current_world_ptr->timewalk_m_idx) {
        return (world[randint0(world_num)]);
    }

    if (special_num) {
        bool success = FALSE;
        switch (m_ptr->r_idx) {
        case MON_BANOR:
        case MON_LUPART:
            if ((m_ptr->hp < m_ptr->maxhp / 2) && r_info[MON_BANOR].max_num && r_info[MON_LUPART].max_num)
                success = TRUE;
            break;
        default:
            break;
        }

        if (success)
            return (special[randint0(special_num)]);
    }

    if (m_ptr->hp < m_ptr->maxhp / 3 && one_in_(2)) {
        if (heal_num)
            return (heal[randint0(heal_num)]);
    }

    if (((m_ptr->hp < m_ptr->maxhp / 3) || monster_fear_remaining(m_ptr)) && one_in_(2)) {
        if (escape_num)
            return (escape[randint0(escape_num)]);
    }

    if (special_num) {
        bool success = FALSE;
        switch (m_ptr->r_idx) {
        case MON_OHMU:
        case MON_BANOR:
        case MON_LUPART:
            break;
        case MON_BANORLUPART:
            if (randint0(100) < 70)
                success = TRUE;
            break;
        case MON_ROLENTO:
            if (randint0(100) < 40)
                success = TRUE;
            break;
        default:
            if (randint0(100) < 50)
                success = TRUE;
            break;
        }
        if (success)
            return (special[randint0(special_num)]);
    }

    if ((distance(target_ptr->y, target_ptr->x, m_ptr->fy, m_ptr->fx) < 4) && (attack_num || (r_ptr->a_ability_flags2 & RF6_TRAPS)) && (randint0(100) < 75)
        && !current_world_ptr->timewalk_m_idx) {
        if (tactic_num)
            return (tactic[randint0(tactic_num)]);
    }

    if (summon_num && (randint0(100) < 40)) {
        return (summon[randint0(summon_num)]);
    }

    if (dispel_num && one_in_(2)) {
        if (dispel_check(target_ptr, m_idx)) {
            return (dispel[randint0(dispel_num)]);
        }
    }

    if (raise_num && (randint0(100) < 40)) {
        return (raise[randint0(raise_num)]);
    }

    if (is_invuln(target_ptr)) {
        if (psy_spe_num && (randint0(100) < 50)) {
            return (psy_spe[randint0(psy_spe_num)]);
        } else if (attack_num && (randint0(100) < 40)) {
            return (attack[randint0(attack_num)]);
        }
    } else if (attack_num && (randint0(100) < 85)) {
        return (attack[randint0(attack_num)]);
    }

    if (tactic_num && (randint0(100) < 50) && !current_world_ptr->timewalk_m_idx) {
        return (tactic[randint0(tactic_num)]);
    }

    if (invul_num && !m_ptr->mtimed[MTIMED_INVULNER] && (randint0(100) < 50)) {
        return (invul[randint0(invul_num)]);
    }

    if ((m_ptr->hp < m_ptr->maxhp * 3 / 4) && (randint0(100) < 25)) {
        if (heal_num)
            return (heal[randint0(heal_num)]);
    }

    if (haste_num && (randint0(100) < 20) && !monster_fast_remaining(m_ptr)) {
        return (haste[randint0(haste_num)]);
    }

    if (annoy_num && (randint0(100) < 80)) {
        return (annoy[randint0(annoy_num)]);
    }

    return 0;
}

/*!
 * @brief ID値が非魔術的な特殊技能かどうかを返す /
 * Return TRUE if a spell is inate spell.
 * @param spell 判定対象のID
 * @return 非魔術的な特殊技能ならばTRUEを返す。
 */
bool spell_is_inate(SPELL_IDX spell)
{
    if (spell < 32 * 4) /* Set RF4 */
    {
        if ((1L << (spell - 32 * 3)) & RF4_NOMAGIC_MASK)
            return TRUE;
    }

    if (spell < 32 * 5) /* Set RF5 */
    {
        if ((1L << (spell - 32 * 4)) & RF5_NOMAGIC_MASK)
            return TRUE;
    }

    if (spell < 32 * 6) /* Set RF6 */
    {
        if ((1L << (spell - 32 * 5)) & RF6_NOMAGIC_MASK)
            return TRUE;
    }

    /* This spell is not "inate" */
    return FALSE;
}

/*!
 * @brief モンスターがプレイヤーにダメージを与えるための最適な座標を算出する /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_ptr 技能を使用するモンスター構造体の参照ポインタ
 * @param yp 最適な目標地点のY座標を返す参照ポインタ
 * @param xp 最適な目標地点のX座標を返す参照ポインタ
 * @param f_flag 射線に入れるのを避ける地形の所持フラグ
 * @param path_check 射線を判定するための関数ポインタ
 * @return 有効な座標があった場合TRUEを返す
 */
static bool adjacent_grid_check(player_type *target_ptr, monster_type *m_ptr, POSITION *yp, POSITION *xp, int f_flag,
    bool (*path_check)(player_type *, POSITION, POSITION, POSITION, POSITION))
{
    static int tonari_y[4][8] = { { -1, -1, -1, 0, 0, 1, 1, 1 }, { -1, -1, -1, 0, 0, 1, 1, 1 }, { 1, 1, 1, 0, 0, -1, -1, -1 }, { 1, 1, 1, 0, 0, -1, -1, -1 } };
    static int tonari_x[4][8] = { { -1, 0, 1, -1, 1, -1, 0, 1 }, { 1, 0, -1, 1, -1, 1, 0, -1 }, { -1, 0, 1, -1, 1, -1, 0, 1 }, { 1, 0, -1, 1, -1, 1, 0, -1 } };

    int next;
    if (m_ptr->fy < target_ptr->y && m_ptr->fx < target_ptr->x)
        next = 0;
    else if (m_ptr->fy < target_ptr->y)
        next = 1;
    else if (m_ptr->fx < target_ptr->x)
        next = 2;
    else
        next = 3;

    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    for (int i = 0; i < 8; i++) {
        int next_x = *xp + tonari_x[next][i];
        int next_y = *yp + tonari_y[next][i];
        grid_type *g_ptr;
        g_ptr = &floor_ptr->grid_array[next_y][next_x];
        if (!cave_have_flag_grid(g_ptr, f_flag))
            continue;

        if (path_check(target_ptr, m_ptr->fy, m_ptr->fx, next_y, next_x)) {
            *yp = next_y;
            *xp = next_x;
            return TRUE;
        }
    }

    return FALSE;
}

/*!
 * todo メインルーチンの割に長過ぎる。要分割
 * @brief モンスターの特殊技能メインルーチン /
 * Creatures can cast spells, shoot missiles, and breathe.
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスター構造体配列のID
 * @return 実際に特殊技能を利用したらTRUEを返す
 */
bool make_attack_spell(MONSTER_IDX m_idx, player_type *target_ptr)
{
#ifdef JP
#else
    char m_poss[80];
#endif
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];

    if (monster_confused_remaining(m_ptr)) {
        reset_target(m_ptr);
        return FALSE;
    }

    if (m_ptr->mflag & MFLAG_NICE)
        return FALSE;
    if (!is_hostile(m_ptr))
        return FALSE;

    bool no_inate = FALSE;
    if (randint0(100) >= (r_ptr->freq_spell * 2))
        no_inate = TRUE;

    BIT_FLAGS f4 = r_ptr->flags4;
    BIT_FLAGS f5 = r_ptr->a_ability_flags1;
    BIT_FLAGS f6 = r_ptr->a_ability_flags2;

    if ((m_ptr->cdis > get_max_range(target_ptr)) && !m_ptr->target_y)
        return FALSE;

    POSITION x = target_ptr->x;
    POSITION y = target_ptr->y;
    POSITION x_br_lite = 0;
    POSITION y_br_lite = 0;
    if (f4 & RF4_BR_LITE) {
        y_br_lite = y;
        x_br_lite = x;

        if (los(target_ptr, m_ptr->fy, m_ptr->fx, y_br_lite, x_br_lite)) {
            feature_type *f_ptr = &f_info[floor_ptr->grid_array[y_br_lite][x_br_lite].feat];

            if (!have_flag(f_ptr->flags, FF_LOS)) {
                if (have_flag(f_ptr->flags, FF_PROJECT) && one_in_(2))
                    f4 &= ~(RF4_BR_LITE);
            }
        } else if (!adjacent_grid_check(target_ptr, m_ptr, &y_br_lite, &x_br_lite, FF_LOS, los))
            f4 &= ~(RF4_BR_LITE);

        if (!(f4 & RF4_BR_LITE)) {
            y_br_lite = 0;
            x_br_lite = 0;
        }
    }

    bool do_spell = DO_SPELL_NONE;
    if (projectable(target_ptr, m_ptr->fy, m_ptr->fx, y, x)) {
        feature_type *f_ptr = &f_info[floor_ptr->grid_array[y][x].feat];
        if (!have_flag(f_ptr->flags, FF_PROJECT)) {
            if ((f4 & RF4_BR_DISI) && have_flag(f_ptr->flags, FF_HURT_DISI) && one_in_(2))
                do_spell = DO_SPELL_BR_DISI;
            else if ((f4 & RF4_BR_LITE) && have_flag(f_ptr->flags, FF_LOS) && one_in_(2))
                do_spell = DO_SPELL_BR_LITE;
        }
    } else {
        bool success = FALSE;
        if ((f4 & RF4_BR_DISI) && (m_ptr->cdis < get_max_range(target_ptr) / 2) && in_disintegration_range(floor_ptr, m_ptr->fy, m_ptr->fx, y, x)
            && (one_in_(10) || (projectable(target_ptr, y, x, m_ptr->fy, m_ptr->fx) && one_in_(2)))) {
            do_spell = DO_SPELL_BR_DISI;
            success = TRUE;
        } else if ((f4 & RF4_BR_LITE) && (m_ptr->cdis < get_max_range(target_ptr) / 2) && los(target_ptr, m_ptr->fy, m_ptr->fx, y, x) && one_in_(5)) {
            do_spell = DO_SPELL_BR_LITE;
            success = TRUE;
        } else if ((f5 & RF5_BA_LITE) && (m_ptr->cdis <= get_max_range(target_ptr))) {
            POSITION by = y, bx = x;
            get_project_point(target_ptr, m_ptr->fy, m_ptr->fx, &by, &bx, 0L);
            if ((distance(by, bx, y, x) <= 3) && los(target_ptr, by, bx, y, x) && one_in_(5)) {
                do_spell = DO_SPELL_BA_LITE;
                success = TRUE;
            }
        }

        if (!success)
            success = adjacent_grid_check(target_ptr, m_ptr, &y, &x, FF_PROJECT, projectable);

        if (!success) {
            if (m_ptr->target_y && m_ptr->target_x) {
                y = m_ptr->target_y;
                x = m_ptr->target_x;
                f4 &= (RF4_INDIRECT_MASK);
                f5 &= (RF5_INDIRECT_MASK);
                f6 &= (RF6_INDIRECT_MASK);
                success = TRUE;
            }

            if (y_br_lite && x_br_lite && (m_ptr->cdis < get_max_range(target_ptr) / 2) && one_in_(5)) {
                if (!success) {
                    y = y_br_lite;
                    x = x_br_lite;
                    do_spell = DO_SPELL_BR_LITE;
                    success = TRUE;
                } else
                    f4 |= (RF4_BR_LITE);
            }
        }

        if (!success)
            return FALSE;
    }

    reset_target(m_ptr);
    DEPTH rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);
    if (no_inate) {
        f4 &= ~(RF4_NOMAGIC_MASK);
        f5 &= ~(RF5_NOMAGIC_MASK);
        f6 &= ~(RF6_NOMAGIC_MASK);
    }

    bool can_use_lite_area = FALSE;
    if (f6 & RF6_DARKNESS) {
        if ((target_ptr->pclass == CLASS_NINJA) && !(r_ptr->flags3 & (RF3_UNDEAD | RF3_HURT_LITE)) && !(r_ptr->flags7 & RF7_DARK_MASK))
            can_use_lite_area = TRUE;

        if (!(r_ptr->flags2 & RF2_STUPID)) {
            if (d_info[target_ptr->dungeon_idx].flags1 & DF1_DARKNESS)
                f6 &= ~(RF6_DARKNESS);
            else if ((target_ptr->pclass == CLASS_NINJA) && !can_use_lite_area)
                f6 &= ~(RF6_DARKNESS);
        }
    }

    bool in_no_magic_dungeon = (d_info[target_ptr->dungeon_idx].flags1 & DF1_NO_MAGIC) && floor_ptr->dun_level
        && (!floor_ptr->inside_quest || is_fixed_quest_idx(floor_ptr->inside_quest));
    if (in_no_magic_dungeon && !(r_ptr->flags2 & RF2_STUPID)) {
        f4 &= (RF4_NOMAGIC_MASK);
        f5 &= (RF5_NOMAGIC_MASK);
        f6 &= (RF6_NOMAGIC_MASK);
    }

    if (r_ptr->flags2 & RF2_SMART) {
        if ((m_ptr->hp < m_ptr->maxhp / 10) && (randint0(100) < 50)) {
            f4 &= (RF4_INT_MASK);
            f5 &= (RF5_INT_MASK);
            f6 &= (RF6_INT_MASK);
        }

        if ((f6 & RF6_TELE_LEVEL) && is_teleport_level_ineffective(target_ptr, 0)) {
            f6 &= ~(RF6_TELE_LEVEL);
        }
    }

    if (!f4 && !f5 && !f6)
        return FALSE;

    remove_bad_spells(m_idx, target_ptr, &f4, &f5, &f6);
    if (floor_ptr->inside_arena || target_ptr->phase_out) {
        f4 &= ~(RF4_SUMMON_MASK);
        f5 &= ~(RF5_SUMMON_MASK);
        f6 &= ~(RF6_SUMMON_MASK | RF6_TELE_LEVEL);

        if (m_ptr->r_idx == MON_ROLENTO)
            f6 &= ~(RF6_SPECIAL);
    }

    if (!f4 && !f5 && !f6)
        return FALSE;

    if (!(r_ptr->flags2 & RF2_STUPID)) {
        if (!target_ptr->csp)
            f5 &= ~(RF5_DRAIN_MANA);

        if (((f4 & RF4_BOLT_MASK) || (f5 & RF5_BOLT_MASK) || (f6 & RF6_BOLT_MASK))
            && !clean_shot(target_ptr, m_ptr->fy, m_ptr->fx, target_ptr->y, target_ptr->x, FALSE)) {
            f4 &= ~(RF4_BOLT_MASK);
            f5 &= ~(RF5_BOLT_MASK);
            f6 &= ~(RF6_BOLT_MASK);
        }

        if (((f4 & RF4_SUMMON_MASK) || (f5 & RF5_SUMMON_MASK) || (f6 & RF6_SUMMON_MASK)) && !(summon_possible(target_ptr, y, x))) {
            f4 &= ~(RF4_SUMMON_MASK);
            f5 &= ~(RF5_SUMMON_MASK);
            f6 &= ~(RF6_SUMMON_MASK);
        }

        if ((f6 & RF6_RAISE_DEAD) && !raise_possible(target_ptr, m_ptr)) {
            f6 &= ~(RF6_RAISE_DEAD);
        }

        if (f6 & RF6_SPECIAL) {
            if ((m_ptr->r_idx == MON_ROLENTO) && !summon_possible(target_ptr, y, x)) {
                f6 &= ~(RF6_SPECIAL);
            }
        }

        if (!f4 && !f5 && !f6)
            return FALSE;
    }

    byte spell[96], num = 0;
    for (int k = 0; k < 32; k++) {
        if (f4 & (1L << k))
            spell[num++] = k + RF4_SPELL_START;
    }

    for (int k = 0; k < 32; k++) {
        if (f5 & (1L << k))
            spell[num++] = k + RF5_SPELL_START;
    }

    for (int k = 0; k < 32; k++) {
        if (f6 & (1L << k))
            spell[num++] = k + RF6_SPELL_START;
    }

    if (!num)
        return FALSE;

    if (!target_ptr->playing || target_ptr->is_dead)
        return FALSE;

    if (target_ptr->leaving)
        return FALSE;

    GAME_TEXT m_name[MAX_NLEN];
    monster_desc(target_ptr, m_name, m_ptr, 0x00);

#ifdef JP
#else
    /* Get the monster possessive ("his"/"her"/"its") */
    monster_desc(target_ptr, m_poss, m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE);
#endif

    SPELL_IDX thrown_spell = 0;
    switch (do_spell) {
    case DO_SPELL_NONE: {
        int attempt = 10;
        while (attempt--) {
            thrown_spell = choose_attack_spell(target_ptr, m_idx, spell, num);
            if (thrown_spell)
                break;
        }
        break;
    }
    case DO_SPELL_BR_LITE:
        thrown_spell = 96 + 14; /* RF4_BR_LITE */
        break;
    case DO_SPELL_BR_DISI:
        thrown_spell = 96 + 31; /* RF4_BR_DISI */
        break;
    case DO_SPELL_BA_LITE:
        thrown_spell = 128 + 20; /* RF5_BA_LITE */
        break;
    default:
        return FALSE;
    }

    if (!thrown_spell)
        return FALSE;

    PERCENTAGE failrate = 25 - (rlev + 3) / 4;

    if (r_ptr->flags2 & RF2_STUPID)
        failrate = 0;

    if (!spell_is_inate(thrown_spell) && (in_no_magic_dungeon || (monster_stunned_remaining(m_ptr) && one_in_(2)) || (randint0(100) < failrate))) {
        disturb(target_ptr, TRUE, TRUE);
        msg_format(_("%^sは呪文を唱えようとしたが失敗した。", "%^s tries to cast a spell, but fails."), m_name);

        return TRUE;
    }

    if (!spell_is_inate(thrown_spell) && magic_barrier(target_ptr, m_idx)) {
        msg_format(_("反魔法バリアが%^sの呪文をかき消した。", "Anti magic barrier cancels the spell which %^s casts."), m_name);
        return TRUE;
    }

    bool direct = player_bold(target_ptr, y, x);
    bool can_remember = is_original_ap_and_seen(target_ptr, m_ptr);
    if (!direct) {
        switch (thrown_spell) {
        case 96 + 2: /* RF4_DISPEL */
        case 96 + 4: /* RF4_SHOOT */
        case 128 + 9: /* RF5_DRAIN_MANA */
        case 128 + 10: /* RF5_MIND_BLAST */
        case 128 + 11: /* RF5_BRAIN_SMASH */
        case 128 + 12: /* RF5_CAUSE_1 */
        case 128 + 13: /* RF5_CAUSE_2 */
        case 128 + 14: /* RF5_CAUSE_3 */
        case 128 + 15: /* RF5_CAUSE_4 */
        case 128 + 16: /* RF5_BO_ACID */
        case 128 + 17: /* RF5_BO_ELEC */
        case 128 + 18: /* RF5_BO_FIRE */
        case 128 + 19: /* RF5_BO_COLD */
        case 128 + 21: /* RF5_BO_NETH */
        case 128 + 22: /* RF5_BO_WATE */
        case 128 + 23: /* RF5_BO_MANA */
        case 128 + 24: /* RF5_BO_PLAS */
        case 128 + 25: /* RF5_BO_ICEE */
        case 128 + 26: /* RF5_MISSILE */
        case 128 + 27: /* RF5_SCARE */
        case 128 + 28: /* RF5_BLIND */
        case 128 + 29: /* RF5_CONF */
        case 128 + 30: /* RF5_SLOW */
        case 128 + 31: /* RF5_HOLD */
        case 160 + 1: /* RF6_HAND_DOOM */
        case 160 + 8: /* RF6_TELE_TO */
        case 160 + 9: /* RF6_TELE_AWAY */
        case 160 + 10: /* RF6_TELE_LEVEL */
        case 160 + 11: /* RF6_PSY_SPEAR */
        case 160 + 12: /* RF6_DARKNESS */
        case 160 + 14: /* RF6_FORGET */
            return FALSE;
        }
    }

    int dam = monspell_to_player(target_ptr, thrown_spell, y, x, m_idx);
    if (dam < 0)
        return FALSE;

    if ((target_ptr->action == ACTION_LEARN) && thrown_spell > 175) {
        learn_spell(target_ptr, thrown_spell - 96);
    }

    bool seen = (!target_ptr->blind && m_ptr->ml);
    bool maneable = player_has_los_bold(target_ptr, m_ptr->fy, m_ptr->fx);
    if (seen && maneable && !current_world_ptr->timewalk_m_idx && (target_ptr->pclass == CLASS_IMITATOR)) {
        if (thrown_spell != 167) /* Not RF6_SPECIAL */
        {
            if (target_ptr->mane_num == MAX_MANE) {
                int i;
                target_ptr->mane_num--;
                for (i = 0; i < target_ptr->mane_num; i++) {
                    target_ptr->mane_spell[i] = target_ptr->mane_spell[i + 1];
                    target_ptr->mane_dam[i] = target_ptr->mane_dam[i + 1];
                }
            }
            target_ptr->mane_spell[target_ptr->mane_num] = thrown_spell - 96;
            target_ptr->mane_dam[target_ptr->mane_num] = dam;
            target_ptr->mane_num++;
            target_ptr->new_mane = TRUE;

            target_ptr->redraw |= (PR_IMITATION);
        }
    }

    if (can_remember) {
        if (thrown_spell < 32 * 4) {
            r_ptr->r_flags4 |= (1L << (thrown_spell - 32 * 3));
            if (r_ptr->r_cast_spell < MAX_UCHAR)
                r_ptr->r_cast_spell++;
        } else if (thrown_spell < 32 * 5) {
            r_ptr->r_flags5 |= (1L << (thrown_spell - 32 * 4));
            if (r_ptr->r_cast_spell < MAX_UCHAR)
                r_ptr->r_cast_spell++;
        } else if (thrown_spell < 32 * 6) {
            r_ptr->r_flags6 |= (1L << (thrown_spell - 32 * 5));
            if (r_ptr->r_cast_spell < MAX_UCHAR)
                r_ptr->r_cast_spell++;
        }
    }

    if (target_ptr->is_dead && (r_ptr->r_deaths < MAX_SHORT) && !floor_ptr->inside_arena) {
        r_ptr->r_deaths++;
    }

    return TRUE;
}
