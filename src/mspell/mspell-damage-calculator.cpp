#include "mspell/mspell-damage-calculator.h"
#include "game-option/birth-options.h"
#include "inventory/inventory-slot-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-ability-flags.h"
#include "monster-race/race-flags2.h"
#include "monster/monster-status.h"
#include "player-info/equipment-info.h"
#include "system/floor-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

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
static int monspell_damage_roll(int dam, int dice_num, int dice_side, int mult, int div, int TYPE)
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

    if (dam < 1) {
        dam = 1;
    }
    return dam;
}

/*!
 * @brief モンスターの使う呪文の威力を返す /
 * @param player_ptr プレイヤーへの参照ポインタ (破滅の手用)
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
static int monspell_damage_base(
    PlayerType *player_ptr, MonsterAbilityType ms_type, int hp, int rlev, bool powerful, int shoot_dd, int shoot_ds, int shoot_base, int TYPE)
{
    int dam = 0, dice_num = 0, dice_side = 0, mult = 1, div = 1;

    switch (ms_type) {
    case MonsterAbilityType::SHRIEK:
        return -1;
    case MonsterAbilityType::XXX1:
        return -1;
    case MonsterAbilityType::DISPEL:
        return -1;
    case MonsterAbilityType::ROCKET:
        dam = (hp / 4) > 800 ? 800 : (hp / 4);
        break;
    case MonsterAbilityType::SHOOT:
        dice_num = shoot_dd;
        dice_side = shoot_ds;
        dam = shoot_base;
        break;
    case MonsterAbilityType::XXX2:
        return -1;
    case MonsterAbilityType::XXX3:
        return -1;
    case MonsterAbilityType::XXX4:
        return -1;

    case MonsterAbilityType::BR_ACID:
    case MonsterAbilityType::BR_ELEC:
    case MonsterAbilityType::BR_FIRE:
    case MonsterAbilityType::BR_COLD:
        dam = ((hp / 3) > 1600 ? 1600 : (hp / 3));
        break;
    case MonsterAbilityType::BR_POIS:
        dam = ((hp / 3) > 800 ? 800 : (hp / 3));
        break;
    case MonsterAbilityType::BR_NETH:
        dam = ((hp / 6) > 550 ? 550 : (hp / 6));
        break;
    case MonsterAbilityType::BR_LITE:
    case MonsterAbilityType::BR_DARK:
        dam = ((hp / 6) > 400 ? 400 : (hp / 6));
        break;
    case MonsterAbilityType::BR_CONF:
    case MonsterAbilityType::BR_SOUN:
        dam = ((hp / 6) > 450 ? 450 : (hp / 6));
        break;
    case MonsterAbilityType::BR_CHAO:
        dam = ((hp / 6) > 600 ? 600 : (hp / 6));
        break;
    case MonsterAbilityType::BR_DISE:
        dam = ((hp / 6) > 500 ? 500 : (hp / 6));
        break;
    case MonsterAbilityType::BR_NEXU:
        dam = ((hp / 3) > 250 ? 250 : (hp / 3));
        break;
    case MonsterAbilityType::BR_TIME:
        dam = ((hp / 3) > 150 ? 150 : (hp / 3));
        break;
    case MonsterAbilityType::BR_INER:
    case MonsterAbilityType::BR_GRAV:
        dam = ((hp / 6) > 200 ? 200 : (hp / 6));
        break;
    case MonsterAbilityType::BR_SHAR:
        dam = ((hp / 6) > 500 ? 500 : (hp / 6));
        break;
    case MonsterAbilityType::BR_PLAS:
        dam = ((hp / 6) > 150 ? 150 : (hp / 6));
        break;
    case MonsterAbilityType::BR_FORC:
        dam = ((hp / 6) > 200 ? 200 : (hp / 6));
        break;
    case MonsterAbilityType::BR_MANA:
        dam = ((hp / 3) > 250 ? 250 : (hp / 3));
        break;
    case MonsterAbilityType::BA_NUKE:
        mult = powerful ? 2 : 1;
        dam = rlev * (mult / div);
        dice_num = 10;
        dice_side = 6;
        break;
    case MonsterAbilityType::BR_NUKE:
        dam = ((hp / 3) > 800 ? 800 : (hp / 3));
        break;
    case MonsterAbilityType::BA_CHAO:
        dam = (powerful ? (rlev * 3) : (rlev * 2));
        dice_num = 10;
        dice_side = 10;
        break;
    case MonsterAbilityType::BR_DISI:
        dam = ((hp / 6) > 150 ? 150 : (hp / 6));
        break;
    case MonsterAbilityType::BR_VOID:
        dam = ((hp / 3) > 250 ? 250 : (hp / 6));
        break;
    case MonsterAbilityType::BR_ABYSS:
        dam = ((hp / 3) > 250 ? 250 : (hp / 6));
        break;
    case MonsterAbilityType::BA_ACID:
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
    case MonsterAbilityType::BA_ELEC:
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
    case MonsterAbilityType::BA_FIRE:
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
    case MonsterAbilityType::BA_COLD:
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
    case MonsterAbilityType::BA_POIS:
        mult = powerful ? 2 : 1;
        dice_num = 12;
        dice_side = 2;
        break;
    case MonsterAbilityType::BA_NETH:
        dam = 50 + rlev * (powerful ? 2 : 1);
        dice_num = 10;
        dice_side = 10;
        break;
    case MonsterAbilityType::BA_WATE:
        dam = 50;
        dice_num = 1;
        dice_side = powerful ? (rlev * 3) : (rlev * 2);
        break;
    case MonsterAbilityType::BA_MANA:
    case MonsterAbilityType::BA_DARK:
        dam = (rlev * 4) + 50;
        dice_num = 10;
        dice_side = 10;
        break;
    case MonsterAbilityType::BA_VOID:
        dam = (powerful ? (rlev * 3) : (rlev * 2));
        dice_num = 10;
        dice_side = 10;
        break;
    case MonsterAbilityType::BA_ABYSS:
        dam = (powerful ? (rlev * 3) : (rlev * 2));
        dice_num = 10;
        dice_side = 10;
        break;
    case MonsterAbilityType::BA_METEOR:
        dam = 50 + rlev / 2;
        dice_num = rlev * 5 / 2;
        dice_side = 2;
        break;
    case MonsterAbilityType::DRAIN_MANA:
        dam = rlev;
        div = 1;
        dice_num = 1;
        dice_side = rlev;
        break;
    case MonsterAbilityType::MIND_BLAST:
        dice_num = 7;
        dice_side = 7;
        break;
    case MonsterAbilityType::BRAIN_SMASH:
        dice_num = 12;
        dice_side = 12;
        break;
    case MonsterAbilityType::CAUSE_1:
        dice_num = 3;
        dice_side = 8;
        break;
    case MonsterAbilityType::CAUSE_2:
        dice_num = 8;
        dice_side = 8;
        break;
    case MonsterAbilityType::CAUSE_3:
        dice_num = 10;
        dice_side = 15;
        break;
    case MonsterAbilityType::CAUSE_4:
        dice_num = 15;
        dice_side = 15;
        break;
    case MonsterAbilityType::BO_ACID:
        mult = powerful ? 2 : 1;
        dam = rlev / 3 * (mult / div);
        dice_num = 7;
        dice_side = 8;
        break;
    case MonsterAbilityType::BO_ELEC:
        mult = powerful ? 2 : 1;
        dam = rlev / 3 * (mult / div);
        dice_num = 4;
        dice_side = 8;
        break;
    case MonsterAbilityType::BO_FIRE:
        mult = powerful ? 2 : 1;
        dam = rlev / 3 * (mult / div);
        dice_num = 9;
        dice_side = 8;
        break;
    case MonsterAbilityType::BO_COLD:
        mult = powerful ? 2 : 1;
        dam = rlev / 3 * (mult / div);
        dice_num = 6;
        dice_side = 8;
        break;
    case MonsterAbilityType::BA_LITE:
        dam = (rlev * 4) + 50;
        dice_num = 10;
        dice_side = 10;
        break;
    case MonsterAbilityType::BO_NETH:
        dam = 30 + (rlev * 4) / (powerful ? 2 : 3);
        dice_num = 5;
        dice_side = 5;
        break;
    case MonsterAbilityType::BO_WATE:
        dam = (rlev * 3 / (powerful ? 2 : 3));
        dice_num = 10;
        dice_side = 10;
        break;
    case MonsterAbilityType::BO_MANA:
        dam = 50;
        dice_num = 1;
        dice_side = rlev * 7 / 2;
        break;
    case MonsterAbilityType::BO_PLAS:
        dam = 10 + (rlev * 3 / (powerful ? 2 : 3));
        dice_num = 8;
        dice_side = 7;
        break;
    case MonsterAbilityType::BO_ICEE:
        dam = (rlev * 3 / (powerful ? 2 : 3));
        dice_num = 6;
        dice_side = 6;
        break;
    case MonsterAbilityType::BO_VOID:
        dam = 10 + (rlev * 3 / (powerful ? 2 : 3));
        dice_num = 13;
        dice_side = 14;
        break;
    case MonsterAbilityType::BO_ABYSS:
        dam = 10 + (rlev * 3 / (powerful ? 2 : 3));
        dice_num = 13;
        dice_side = 14;
        break;
    case MonsterAbilityType::BO_METEOR:
        dam = 30 + rlev * 2;
        dice_num = 1;
        dice_side = rlev;
        break;
    case MonsterAbilityType::MISSILE:
        dam = (rlev / 3);
        dice_num = 2;
        dice_side = 6;
        break;
    case MonsterAbilityType::SCARE:
        return -1;
    case MonsterAbilityType::BLIND:
        return -1;
    case MonsterAbilityType::CONF:
        return -1;
    case MonsterAbilityType::SLOW:
        return -1;
    case MonsterAbilityType::HOLD:
        return -1;
    case MonsterAbilityType::HASTE:
        return -1;

    case MonsterAbilityType::HAND_DOOM:
        mult = player_ptr->chp;
        div = 100;
        dam = 40 * (mult / div);
        dice_num = 1;
        dice_side = 20;
        break;

    case MonsterAbilityType::HEAL:
        return -1;
    case MonsterAbilityType::INVULNER:
        return -1;
    case MonsterAbilityType::BLINK:
        return -1;
    case MonsterAbilityType::TPORT:
        return -1;
    case MonsterAbilityType::WORLD:
        return -1;
    case MonsterAbilityType::SPECIAL:
        return -1;
    case MonsterAbilityType::TELE_TO:
        return -1;
    case MonsterAbilityType::TELE_AWAY:
        return -1;
    case MonsterAbilityType::TELE_LEVEL:
        return -1;

    case MonsterAbilityType::PSY_SPEAR:
        dam = powerful ? 150 : 100;
        dice_num = 1;
        dice_side = powerful ? (rlev * 2) : (rlev * 3 / 2);
        break;

    case MonsterAbilityType::DARKNESS:
        return -1;
    case MonsterAbilityType::TRAPS:
        return -1;
    case MonsterAbilityType::FORGET:
        return -1;
    case MonsterAbilityType::RAISE_DEAD:
        return -1;
    case MonsterAbilityType::S_KIN:
        return -1;
    case MonsterAbilityType::S_CYBER:
        return -1;
    case MonsterAbilityType::S_MONSTER:
        return -1;
    case MonsterAbilityType::S_MONSTERS:
        return -1;
    case MonsterAbilityType::S_ANT:
        return -1;
    case MonsterAbilityType::S_SPIDER:
        return -1;
    case MonsterAbilityType::S_HOUND:
        return -1;
    case MonsterAbilityType::S_HYDRA:
        return -1;
    case MonsterAbilityType::S_ANGEL:
        return -1;
    case MonsterAbilityType::S_DEMON:
        return -1;
    case MonsterAbilityType::S_UNDEAD:
        return -1;
    case MonsterAbilityType::S_DRAGON:
        return -1;
    case MonsterAbilityType::S_HI_UNDEAD:
        return -1;
    case MonsterAbilityType::S_HI_DRAGON:
        return -1;
    case MonsterAbilityType::S_AMBERITES:
        return -1;
    case MonsterAbilityType::S_UNIQUE:
        return -1;
    case MonsterAbilityType::S_DEAD_UNIQUE:
        return -1;
    case MonsterAbilityType::MAX:
        return -1;
    }

    return monspell_damage_roll(dam, dice_num, dice_side, mult, div, TYPE);
}

/*!
 * @brief モンスターの使う射撃のダイス情報を返す /
 * @param r_ptr モンスター種族への参照ポインタ
 * @param dd ダイス数への参照ポインタ
 * @param ds ダイス面への参照ポインタ
 */
void monspell_shoot_dice(MonsterRaceInfo *r_ptr, int *dd, int *ds)
{
    int p = -1; /* Position of SHOOT */
    int n = 0; /* Number of blows */
    const int max_blows = 4;
    for (int m = 0; m < max_blows; m++) {
        if (r_ptr->blows[m].method != RaceBlowMethodType::NONE) {
            n++;
        } /* Count blows */

        if (r_ptr->blows[m].method == RaceBlowMethodType::SHOOT) {
            p = m; /* Remember position */
            break;
        }
    }

    /* When full blows, use a first damage */
    if (n == max_blows) {
        p = 0;
    }

    if (p < 0) {
        (*dd) = 0;
        (*ds) = 0;
    } else {
        (*dd) = r_ptr->blows[p].d_dice;
        (*ds) = r_ptr->blows[p].d_side;
    }
}

/*!
 * @brief モンスターの使う呪文の威力を返す /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param ms_type 呪文番号
 * @param m_idx 呪文を唱えるモンスターID
 * @param TYPE  DAM_MAXで最大値を返し、DAM_MINで最小値を返す。DAM_ROLLはダイスを振って値を決定する。
 * @return 攻撃呪文のダメージを返す。攻撃呪文以外は-1を返す。
 */
int monspell_damage(PlayerType *player_ptr, MonsterAbilityType ms_type, MONSTER_IDX m_idx, int TYPE)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[m_idx];
    auto *r_ptr = &m_ptr->get_monrace();
    DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
    int hp = (TYPE == DAM_ROLL) ? m_ptr->hp : m_ptr->max_maxhp;
    int shoot_dd, shoot_ds;

    monspell_shoot_dice(r_ptr, &shoot_dd, &shoot_ds);
    return monspell_damage_base(player_ptr, ms_type, hp, rlev, monster_is_powerful(floor_ptr, m_idx), shoot_dd, shoot_ds, 0, TYPE);
}

/*!
 * @brief モンスターの使う所属としての呪文の威力を返す /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param ms_type 呪文番号
 * @param r_idx 呪文を唱えるモンスターの種族ID
 * @param TYPE  DAM_MAXで最大値を返し、DAM_MINで最小値を返す。DAM_ROLLはダイスを振って値を決定する。
 * @return 攻撃呪文のダメージを返す。攻撃呪文以外は-1を返す。
 */
int monspell_race_damage(PlayerType *player_ptr, MonsterAbilityType ms_type, MonsterRaceId r_idx, int TYPE)
{
    auto *r_ptr = &monraces_info[r_idx];
    DEPTH rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);
    bool powerful = any_bits(r_ptr->flags2, RF2_POWERFUL);
    int hp = r_ptr->hdice * (ironman_nightmare ? 2 : 1) * r_ptr->hside;
    int shoot_dd, shoot_ds;

    monspell_shoot_dice(r_ptr, &shoot_dd, &shoot_ds);
    return monspell_damage_base(player_ptr, ms_type, std::min(MONSTER_MAXHP, hp), rlev, powerful, shoot_dd, shoot_ds, 0, TYPE);
}

/*!
 * @brief 青魔導師の使う呪文の威力を返す /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param SPELL_NUM 呪文番号
 * @param plev 使用するレベル。2倍して扱う。
 * @param TYPE  DAM_MAXで最大値を返し、DAM_MINで最小値を返す。DAM_ROLLはダイスを振って値を決定する。
 * @return 攻撃呪文のダメージを返す。攻撃呪文以外は-1を返す。
 */
int monspell_bluemage_damage(PlayerType *player_ptr, MonsterAbilityType ms_type, PLAYER_LEVEL plev, int TYPE)
{
    int hp = player_ptr->chp;
    int shoot_dd = 1, shoot_ds = 1, shoot_base = 0;
    ItemEntity *o_ptr = nullptr;

    if (has_melee_weapon(player_ptr, INVEN_MAIN_HAND)) {
        o_ptr = &player_ptr->inventory_list[INVEN_MAIN_HAND];
    } else if (has_melee_weapon(player_ptr, INVEN_SUB_HAND)) {
        o_ptr = &player_ptr->inventory_list[INVEN_SUB_HAND];
    }

    if (o_ptr) {
        shoot_dd = o_ptr->dd;
        shoot_ds = o_ptr->ds;
        shoot_base = o_ptr->to_d;
    }

    return monspell_damage_base(player_ptr, ms_type, hp, plev * 2, false, shoot_dd, shoot_ds, shoot_base, TYPE);
}
