﻿#include "mspell/mspell-damage-calculator.h"
#include "inventory/inventory-slot-types.h"
#include "game-option/birth-options.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags2.h"
#include "monster/monster-status.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"

/*!
 * @brief モンスターの使う呪文の威力を決定する /
 * @param dam 定数値
 * @param dice_num ダイス数
 * @param dice_side ダイス面
 * @param mult ダイス倍率
 * @param div ダイス倍率
 * @param TYPE  DAM_MAXで最大値を返し、DAM_MINで最小値を返す。DAM_ROLLはダイスを振って値を決定する。
 * @return 攻撃呪文のダメージを返す。攻撃呪文以外は-1を返す。
 */
static HIT_POINT monspell_damage_roll(HIT_POINT dam, int dice_num, int dice_side, int mult, int div, int TYPE)
{
    switch (TYPE) {
    case DAM_MAX:
        dam += maxroll(dice_num, dice_side) * mult / div;
        break;
    case DAM_MIN:
        dam += dice_num * 1 * mult / div;
        break;
    case DAM_ROLL:
        dam += damroll(dice_num, dice_side) * mult / div;
        break;
    case DICE_NUM:
        return dice_num;
    case DICE_SIDE:
        return dice_side;
    case DICE_MULT:
        return mult;
    case DICE_DIV:
        return div;
    case BASE_DAM:
        return dam;
    }

    if (dam < 1)
        dam = 1;
    return dam;
}

/*!
 * @brief モンスターの使う呪文の威力を返す /
 * @param target_ptr プレーヤーへの参照ポインタ (破滅の手用)
 * @param SPELL_NUM 呪文番号
 * @param hp 呪文を唱えるモンスターの体力
 * @param rlev 呪文を唱えるモンスターのレベル
 * @param powerful 呪文を唱えるモンスターのpowerfulフラグ
 * @param shoot_dd 射撃のダイス数
 * @param shoot_ds 射撃のダイス面
 * @param shoot_base 射撃の固定威力値
 * @param TYPE  DAM_MAXで最大値を返し、DAM_MINで最小値を返す。DAM_ROLLはダイスを振って値を決定する。
 * @return 攻撃呪文のダメージを返す。攻撃呪文以外は-1を返す。
 */
static HIT_POINT monspell_damage_base(
    player_type *target_ptr, monster_spell_type ms_type, int hp, int rlev, bool powerful, int shoot_dd, int shoot_ds, int shoot_base, int TYPE)
{
    HIT_POINT dam = 0, dice_num = 0, dice_side = 0, mult = 1, div = 1;

    switch (ms_type) {
    case MS_SHRIEK:
        return -1;
    case MS_XXX1:
        return -1;
    case MS_DISPEL:
        return -1;
    case MS_ROCKET:
        dam = (hp / 4) > 800 ? 800 : (hp / 4);
        break;
    case MS_SHOOT:
        dice_num = shoot_dd;
        dice_side = shoot_ds;
        dam = shoot_base;
        break;
    case MS_XXX2:
        return -1;
    case MS_XXX3:
        return -1;
    case MS_XXX4:
        return -1;

    case MS_BR_ACID:
    case MS_BR_ELEC:
    case MS_BR_FIRE:
    case MS_BR_COLD:
        dam = ((hp / 3) > 1600 ? 1600 : (hp / 3));
        break;
    case MS_BR_POIS:
        dam = ((hp / 3) > 800 ? 800 : (hp / 3));
        break;
    case MS_BR_NETHER:
        dam = ((hp / 6) > 550 ? 550 : (hp / 6));
        break;
    case MS_BR_LITE:
    case MS_BR_DARK:
        dam = ((hp / 6) > 400 ? 400 : (hp / 6));
        break;
    case MS_BR_CONF:
    case MS_BR_SOUND:
        dam = ((hp / 6) > 450 ? 450 : (hp / 6));
        break;
    case MS_BR_CHAOS:
        dam = ((hp / 6) > 600 ? 600 : (hp / 6));
        break;
    case MS_BR_DISEN:
        dam = ((hp / 6) > 500 ? 500 : (hp / 6));
        break;
    case MS_BR_NEXUS:
        dam = ((hp / 3) > 250 ? 250 : (hp / 3));
        break;
    case MS_BR_TIME:
        dam = ((hp / 3) > 150 ? 150 : (hp / 3));
        break;
    case MS_BR_INERTIA:
    case MS_BR_GRAVITY:
        dam = ((hp / 6) > 200 ? 200 : (hp / 6));
        break;
    case MS_BR_SHARDS:
        dam = ((hp / 6) > 500 ? 500 : (hp / 6));
        break;
    case MS_BR_PLASMA:
        dam = ((hp / 6) > 150 ? 150 : (hp / 6));
        break;
    case MS_BR_FORCE:
        dam = ((hp / 6) > 200 ? 200 : (hp / 6));
        break;
    case MS_BR_MANA:
        dam = ((hp / 3) > 250 ? 250 : (hp / 3));
        break;
    case MS_BALL_NUKE:
        mult = powerful ? 2 : 1;
        dam = rlev * (mult / div);
        dice_num = 10;
        dice_side = 6;
        break;
    case MS_BR_NUKE:
        dam = ((hp / 3) > 800 ? 800 : (hp / 3));
        break;
    case MS_BALL_CHAOS:
        dam = (powerful ? (rlev * 3) : (rlev * 2));
        dice_num = 10;
        dice_side = 10;
        break;
    case MS_BR_DISI:
        dam = ((hp / 6) > 150 ? 150 : (hp / 6));
        break;
    case MS_BALL_ACID:
        if (powerful) {
            dam = (rlev * 4) + 50;
            dice_num = 10;
            dice_side = 10;
        } else {
            dam = 15;
            dice_num = 1;
            dice_side = rlev * 3;
        }

        break;
    case MS_BALL_ELEC:
        if (powerful) {
            dam = (rlev * 4) + 50;
            dice_num = 10;
            dice_side = 10;
        } else {
            dam = 8;
            dice_num = 1;
            dice_side = rlev * 3 / 2;
        }

        break;
    case MS_BALL_FIRE:
        if (powerful) {
            dam = (rlev * 4) + 50;
            dice_num = 10;
            dice_side = 10;
        } else {
            dam = 10;
            dice_num = 1;
            dice_side = rlev * 7 / 2;
        }

        break;
    case MS_BALL_COLD:
        if (powerful) {
            dam = (rlev * 4) + 50;
            dice_num = 10;
            dice_side = 10;
        } else {
            dam = 10;
            dice_num = 1;
            dice_side = rlev * 3 / 2;
        }

        break;
    case MS_BALL_POIS:
        mult = powerful ? 2 : 1;
        dice_num = 12;
        dice_side = 2;
        break;
    case MS_BALL_NETHER:
        dam = 50 + rlev * (powerful ? 2 : 1);
        dice_num = 10;
        dice_side = 10;
        break;
    case MS_BALL_WATER:
        dam = 50;
        dice_num = 1;
        dice_side = powerful ? (rlev * 3) : (rlev * 2);
        break;
    case MS_BALL_MANA:
    case MS_BALL_DARK:
        dam = (rlev * 4) + 50;
        dice_num = 10;
        dice_side = 10;
        break;
    case MS_DRAIN_MANA:
        dam = rlev;
        div = 1;
        dice_num = 1;
        dice_side = rlev;
        break;
    case MS_MIND_BLAST:
        dice_num = 7;
        dice_side = 7;
        break;
    case MS_BRAIN_SMASH:
        dice_num = 12;
        dice_side = 12;
        break;
    case MS_CAUSE_1:
        dice_num = 3;
        dice_side = 8;
        break;
    case MS_CAUSE_2:
        dice_num = 8;
        dice_side = 8;
        break;
    case MS_CAUSE_3:
        dice_num = 10;
        dice_side = 15;
        break;
    case MS_CAUSE_4:
        dice_num = 15;
        dice_side = 15;
        break;
    case MS_BOLT_ACID:
        mult = powerful ? 2 : 1;
        dam = rlev / 3 * (mult / div);
        dice_num = 7;
        dice_side = 8;
        break;
    case MS_BOLT_ELEC:
        mult = powerful ? 2 : 1;
        dam = rlev / 3 * (mult / div);
        dice_num = 4;
        dice_side = 8;
        break;
    case MS_BOLT_FIRE:
        mult = powerful ? 2 : 1;
        dam = rlev / 3 * (mult / div);
        dice_num = 9;
        dice_side = 8;
        break;
    case MS_BOLT_COLD:
        mult = powerful ? 2 : 1;
        dam = rlev / 3 * (mult / div);
        dice_num = 6;
        dice_side = 8;
        break;
    case MS_STARBURST:
        dam = (rlev * 4) + 50;
        dice_num = 10;
        dice_side = 10;
        break;
    case MS_BOLT_NETHER:
        dam = 30 + (rlev * 4) / (powerful ? 2 : 3);
        dice_num = 5;
        dice_side = 5;
        break;
    case MS_BOLT_WATER:
        dam = (rlev * 3 / (powerful ? 2 : 3));
        dice_num = 10;
        dice_side = 10;
        break;
    case MS_BOLT_MANA:
        dam = 50;
        dice_num = 1;
        dice_side = rlev * 7 / 2;
        break;
    case MS_BOLT_PLASMA:
        dam = 10 + (rlev * 3 / (powerful ? 2 : 3));
        dice_num = 8;
        dice_side = 7;
        break;
    case MS_BOLT_ICE:
        dam = (rlev * 3 / (powerful ? 2 : 3));
        dice_num = 6;
        dice_side = 6;
        break;
    case MS_MAGIC_MISSILE:
        dam = (rlev / 3);
        dice_num = 2;
        dice_side = 6;
        break;
    case MS_SCARE:
        return -1;
    case MS_BLIND:
        return -1;
    case MS_CONF:
        return -1;
    case MS_SLOW:
        return -1;
    case MS_SLEEP:
        return -1;
    case MS_SPEED:
        return -1;

    case MS_HAND_DOOM:
        mult = target_ptr->chp;
        div = 100;
        dam = 40 * (mult / div);
        dice_num = 1;
        dice_side = 20;
        break;

    case MS_HEAL:
        return -1;
    case MS_INVULNER:
        return -1;
    case MS_BLINK:
        return -1;
    case MS_TELEPORT:
        return -1;
    case MS_WORLD:
        return -1;
    case MS_SPECIAL:
        return -1;
    case MS_TELE_TO:
        return -1;
    case MS_TELE_AWAY:
        return -1;
    case MS_TELE_LEVEL:
        return -1;

    case MS_PSY_SPEAR:
        dam = powerful ? 150 : 100;
        dice_num = 1;
        dice_side = powerful ? (rlev * 2) : (rlev * 3 / 2);
        break;

    case MS_DARKNESS:
        return -1;
    case MS_MAKE_TRAP:
        return -1;
    case MS_FORGET:
        return -1;
    case MS_RAISE_DEAD:
        return -1;
    case MS_S_KIN:
        return -1;
    case MS_S_CYBER:
        return -1;
    case MS_S_MONSTER:
        return -1;
    case MS_S_MONSTERS:
        return -1;
    case MS_S_ANT:
        return -1;
    case MS_S_SPIDER:
        return -1;
    case MS_S_HOUND:
        return -1;
    case MS_S_HYDRA:
        return -1;
    case MS_S_ANGEL:
        return -1;
    case MS_S_DEMON:
        return -1;
    case MS_S_UNDEAD:
        return -1;
    case MS_S_DRAGON:
        return -1;
    case MS_S_HI_UNDEAD:
        return -1;
    case MS_S_HI_DRAGON:
        return -1;
    case MS_S_AMBERITE:
        return -1;
    case MS_S_UNIQUE:
        return -1;
    }

    return monspell_damage_roll(dam, dice_num, dice_side, mult, div, TYPE);
}

/*!
 * @brief モンスターの使う射撃のダイス情報を返す /
 * @param r_ptr モンスター種族への参照ポインタ
 * @param dd ダイス数への参照ポインタ
 * @param ds ダイス面への参照ポインタ
 * @return なし
 */
void monspell_shoot_dice(monster_race *r_ptr, int *dd, int *ds)
{
    int p = -1; /* Position of SHOOT */
    int n = 0; /* Number of blows */
    const int max_blows = 4;
    for (int m = 0; m < max_blows; m++) {
        if (r_ptr->blow[m].method != RBM_NONE)
            n++; /* Count blows */

        if (r_ptr->blow[m].method == RBM_SHOOT) {
            p = m; /* Remember position */
            break;
        }
    }

    /* When full blows, use a first damage */
    if (n == max_blows)
        p = 0;

    if (p < 0) {
        (*dd) = 0;
        (*ds) = 0;
    } else {
        (*dd) = r_ptr->blow[p].d_dice;
        (*ds) = r_ptr->blow[p].d_side;
    }
}

/*!
 * @brief モンスターの使う呪文の威力を返す /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param ms_type 呪文番号
 * @param m_idx 呪文を唱えるモンスターID
 * @param TYPE  DAM_MAXで最大値を返し、DAM_MINで最小値を返す。DAM_ROLLはダイスを振って値を決定する。
 * @return 攻撃呪文のダメージを返す。攻撃呪文以外は-1を返す。
 */
HIT_POINT monspell_damage(player_type *target_ptr, monster_spell_type ms_type, MONSTER_IDX m_idx, int TYPE)
{
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
    HIT_POINT hp = (TYPE == DAM_ROLL) ? m_ptr->hp : m_ptr->max_maxhp;
    int shoot_dd, shoot_ds;

    monspell_shoot_dice(r_ptr, &shoot_dd, &shoot_ds);
    return monspell_damage_base(target_ptr, ms_type, hp, rlev, monster_is_powerful(floor_ptr, m_idx), shoot_dd, shoot_ds, 0, TYPE);
}

/*!
 * @brief モンスターの使う所属としての呪文の威力を返す /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param ms_type 呪文番号
 * @param r_idx 呪文を唱えるモンスターの種族ID
 * @param TYPE  DAM_MAXで最大値を返し、DAM_MINで最小値を返す。DAM_ROLLはダイスを振って値を決定する。
 * @return 攻撃呪文のダメージを返す。攻撃呪文以外は-1を返す。
 */
HIT_POINT monspell_race_damage(player_type *target_ptr, monster_spell_type ms_type, MONRACE_IDX r_idx, int TYPE)
{
    monster_race *r_ptr = &r_info[r_idx];
    DEPTH rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);
    bool powerful = r_ptr->flags2 & RF2_POWERFUL ? TRUE : FALSE;
    HIT_POINT hp = r_ptr->hdice * (ironman_nightmare ? 2 : 1) * r_ptr->hside;
    int shoot_dd, shoot_ds;

    monspell_shoot_dice(r_ptr, &shoot_dd, &shoot_ds);
    return monspell_damage_base(target_ptr, ms_type, MIN(30000, hp), rlev, powerful, shoot_dd, shoot_ds, 0, TYPE);
}

/*!
 * @brief 青魔導師の使う呪文の威力を返す /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param SPELL_NUM 呪文番号
 * @param plev 使用するレベル。2倍して扱う。
 * @param TYPE  DAM_MAXで最大値を返し、DAM_MINで最小値を返す。DAM_ROLLはダイスを振って値を決定する。
 * @return 攻撃呪文のダメージを返す。攻撃呪文以外は-1を返す。
 */
HIT_POINT monspell_bluemage_damage(player_type *target_ptr, monster_spell_type ms_type, PLAYER_LEVEL plev, int TYPE)
{
    int hp = target_ptr->chp;
    int shoot_dd = 1, shoot_ds = 1, shoot_base = 0;
    object_type *o_ptr = NULL;

    if (has_melee_weapon(target_ptr, INVEN_MAIN_HAND))
        o_ptr = &target_ptr->inventory_list[INVEN_MAIN_HAND];
    else if (has_melee_weapon(target_ptr, INVEN_SUB_HAND))
        o_ptr = &target_ptr->inventory_list[INVEN_SUB_HAND];

    if (o_ptr) {
        shoot_dd = o_ptr->dd;
        shoot_ds = o_ptr->ds;
        shoot_base = o_ptr->to_d;
    }

    return monspell_damage_base(target_ptr, ms_type, hp, plev * 2, FALSE, shoot_dd, shoot_ds, shoot_base, TYPE);
}
