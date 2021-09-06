/*!
 * @file status-first-page.c
 * @brief キャラ基本情報及び技能値の表示
 * @date 2020/02/23
 * @author Hourier
 */

#include "view/status-first-page.h"
#include "artifact/fixed-art-types.h"
#include "combat/attack-power-table.h"
#include "combat/shoot.h"
#include "display-util.h"
#include "game-option/text-display-options.h"
#include "inventory/inventory-slot-types.h"
#include "mutation/mutation-flag-types.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object/object-flags.h"
#include "perception/object-perception.h"
#include "player-info/equipment-info.h"
#include "player-status/player-hand-types.h"
#include "player/player-status-flags.h"
#include "player/special-defense-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "term/term-color-types.h"
#include "util/bit-flags-calculator.h"

static TERM_COLOR likert_color = TERM_WHITE;

/*!
 * @brief
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 装備中の弓への参照ポインタ
 * @param shots 射撃回数
 * @param shot_frac 射撃速度
 */
static void calc_shot_params(player_type *creature_ptr, object_type *o_ptr, int *shots, int *shot_frac)
{
    if (o_ptr->k_idx == 0)
        return;

    ENERGY energy_fire = bow_energy(o_ptr->sval);
    *shots = creature_ptr->num_fire * 100;
    *shot_frac = ((*shots) * 100 / energy_fire) % 100;
    *shots = (*shots) / energy_fire;
    if (o_ptr->name1 != ART_CRIMSON)
        return;

    *shots = 1;
    *shot_frac = 0;
    if (creature_ptr->pclass != CLASS_ARCHER)
        return;

    if (creature_ptr->lev >= 10)
        (*shots)++;
    if (creature_ptr->lev >= 30)
        (*shots)++;
    if (creature_ptr->lev >= 45)
        (*shots)++;
}

/*!
 * @brief 武器装備に制限のあるクラスで、直接攻撃のダメージを計算する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param hand 手 (利き手が0、反対の手が1…のはず)
 * @param damage 直接攻撃のダメージ
 * @param basedam 素手における直接攻撃のダメージ
 * @param o_ptr 装備中の武器への参照ポインタ
 * @return 利き手ならTRUE、反対の手ならFALSE
 */
static bool calc_weapon_damage_limit(player_type *creature_ptr, int hand, int *damage, int *basedam, object_type *o_ptr)
{
    PLAYER_LEVEL level = creature_ptr->lev;
    if (hand > 0) {
        damage[hand] = 0;
        return false;
    }

    if (creature_ptr->pclass == CLASS_FORCETRAINER)
        level = MAX(1, level - 3);
    if (creature_ptr->special_defense & KAMAE_BYAKKO)
        *basedam = monk_ave_damage[level][1];
    else if (creature_ptr->special_defense & (KAMAE_GENBU | KAMAE_SUZAKU))
        *basedam = monk_ave_damage[level][2];
    else
        *basedam = monk_ave_damage[level][0];

    damage[hand] += *basedam;
    if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_POISON_NEEDLE))
        damage[hand] = 1;
    if (damage[hand] < 0)
        damage[hand] = 0;

    return true;
}

/*!
 * @brief 片手あたりのダメージ量を計算する
 * @param o_ptr 装備中の武器への参照ポインタ
 * @param hand 手
 * @param damage 直接攻撃のダメージ
 * @param basedam 素手における直接攻撃のダメージ
 * @return 素手ならFALSE、武器を持っていればTRUE
 */
static bool calc_weapon_one_hand(object_type *o_ptr, int hand, int *damage, int *basedam)
{
    if (o_ptr->k_idx == 0)
        return false;

    *basedam = 0;
    damage[hand] += *basedam;
    if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_POISON_NEEDLE))
        damage[hand] = 1;

    if (damage[hand] < 0)
        damage[hand] = 0;

    return true;
}

/*!
 * @brief ヴォーパル武器等によるダメージ強化
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 装備中の武器への参照ポインタ
 * @param basedam 素手における直接攻撃のダメージ
 * @param flgs オブジェクトフラグ群
 * @return 強化後の素手ダメージ
 */
static int strengthen_basedam(player_type *creature_ptr, object_type *o_ptr, int basedam, const TrFlags &flgs)
{
    if (o_ptr->is_fully_known() && ((o_ptr->name1 == ART_VORPAL_BLADE) || (o_ptr->name1 == ART_CHAINSWORD))) {
        /* vorpal blade */
        basedam *= 5;
        basedam /= 3;
    } else if (flgs.has(TR_VORPAL)) {
        /* vorpal flag only */
        basedam *= 11;
        basedam /= 9;
    }

    // 理力
    bool is_force = creature_ptr->pclass != CLASS_SAMURAI;
    is_force &= flgs.has(TR_FORCE_WEAPON);
    is_force &= creature_ptr->csp > (o_ptr->dd * o_ptr->ds / 5);
    if (is_force)
        basedam = basedam * 7 / 2;

    return basedam;
}

/*!
 * @brief 技能ランクの表示基準を定める
 * Returns a "rating" of x depending on y
 * @param x 技能値
 * @param y 技能値に対するランク基準比
 */
static concptr likert(int x, int y)
{
    static char dummy[20] = "", dummy2[20] = "";
    memset(dummy, 0, strlen(dummy));
    memset(dummy2, 0, strlen(dummy2));
    if (y <= 0)
        y = 1;

    if (show_actual_value)
        sprintf(dummy, "%3d-", x);

    if (x < 0) {
        likert_color = TERM_L_DARK;
        strcat(dummy, _("最低", "Very Bad"));
        return dummy;
    }

    switch ((x / y)) {
    case 0:
    case 1: {
        likert_color = TERM_RED;
        strcat(dummy, _("悪い", "Bad"));
        break;
    }
    case 2: {
        likert_color = TERM_L_RED;
        strcat(dummy, _("劣る", "Poor"));
        break;
    }
    case 3:
    case 4: {
        likert_color = TERM_ORANGE;
        strcat(dummy, _("普通", "Fair"));
        break;
    }
    case 5: {
        likert_color = TERM_YELLOW;
        strcat(dummy, _("良い", "Good"));
        break;
    }
    case 6: {
        likert_color = TERM_YELLOW;
        strcat(dummy, _("大変良い", "Very Good"));
        break;
    }
    case 7:
    case 8: {
        likert_color = TERM_L_GREEN;
        strcat(dummy, _("卓越", "Excellent"));
        break;
    }
    case 9:
    case 10:
    case 11:
    case 12:
    case 13: {
        likert_color = TERM_GREEN;
        strcat(dummy, _("超越", "Superb"));
        break;
    }
    case 14:
    case 15:
    case 16:
    case 17: {
        likert_color = TERM_BLUE;
        strcat(dummy, _("英雄的", "Heroic"));
        break;
    }
    default: {
        likert_color = TERM_VIOLET;
        sprintf(dummy2, _("伝説的[%d]", "Legendary[%d]"), (int)((((x / y) - 17) * 5) / 2));
        strcat(dummy, dummy2);
        break;
    }
    }

    return dummy;
}

/*!
 * @brief 弓＋両手の武器それぞれについてダメージを計算する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param damage 直接攻撃のダメージ
 * @param to_h 命中補正
 */
static void calc_two_hands(player_type *creature_ptr, int *damage, int *to_h)
{
    object_type *o_ptr;
    o_ptr = &creature_ptr->inventory_list[INVEN_BOW];

    for (int i = 0; i < 2; i++) {
        int basedam;
        damage[i] = creature_ptr->dis_to_d[i] * 100;
        if (((creature_ptr->pclass == CLASS_MONK) || (creature_ptr->pclass == CLASS_FORCETRAINER)) && (empty_hands(creature_ptr, true) & EMPTY_HAND_MAIN)) {
            if (!calc_weapon_damage_limit(creature_ptr, i, damage, &basedam, o_ptr))
                break;

            continue;
        }

        o_ptr = &creature_ptr->inventory_list[INVEN_MAIN_HAND + i];
        if (!calc_weapon_one_hand(o_ptr, i, damage, &basedam))
            continue;

        to_h[i] = 0;
        bool poison_needle = false;
        if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_POISON_NEEDLE))
            poison_needle = true;
        if (o_ptr->is_known()) {
            damage[i] += o_ptr->to_d * 100;
            to_h[i] += o_ptr->to_h;
        }

        basedam = ((o_ptr->dd + creature_ptr->to_dd[i]) * (o_ptr->ds + creature_ptr->to_ds[i] + 1)) * 50;
        auto flgs = object_flags_known(o_ptr);

        bool impact = creature_ptr->impact != 0;
        basedam = calc_expect_crit(creature_ptr, o_ptr->weight, to_h[i], basedam, creature_ptr->dis_to_h[i], poison_needle, impact);
        basedam = strengthen_basedam(creature_ptr, o_ptr, basedam, flgs);
        damage[i] += basedam;
        if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_POISON_NEEDLE))
            damage[i] = 1;
        if (damage[i] < 0)
            damage[i] = 0;
    }
}

/*!
 * @brief キャラ基本情報及び技能値をメインウィンドウに表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param xthb 武器等を含めた最終命中率
 * @param damage 打撃修正
 * @param shots 射撃回数
 * @param shot_frac 射撃速度
 * @param display_player_one_line 1行表示用のコールバック関数
 */
static void display_first_page(player_type *creature_ptr, int xthb, int *damage, int shots, int shot_frac)
{
    int xthn = creature_ptr->skill_thn + (creature_ptr->to_h_m * BTH_PLUS_ADJ);

    int muta_att = 0;
    if (creature_ptr->muta.has(MUTA::HORNS))
        muta_att++;
    if (creature_ptr->muta.has(MUTA::SCOR_TAIL))
        muta_att++;
    if (creature_ptr->muta.has(MUTA::BEAK))
        muta_att++;
    if (creature_ptr->muta.has(MUTA::TRUNK))
        muta_att++;
    if (creature_ptr->muta.has(MUTA::TENTACLES))
        muta_att++;

    int blows1 = can_attack_with_main_hand(creature_ptr) ? creature_ptr->num_blow[0] : 0;
    int blows2 = can_attack_with_sub_hand(creature_ptr) ? creature_ptr->num_blow[1] : 0;
    int xdis = creature_ptr->skill_dis;
    int xdev = creature_ptr->skill_dev;
    int xsav = creature_ptr->skill_sav;
    int xstl = creature_ptr->skill_stl;
    int xsrh = creature_ptr->skill_srh;
    int xfos = creature_ptr->skill_fos;
    int xdig = creature_ptr->skill_dig;

    concptr desc = likert(xthn, 12);
    display_player_one_line(ENTRY_SKILL_FIGHT, desc, likert_color);

    desc = likert(xthb, 12);
    display_player_one_line(ENTRY_SKILL_SHOOT, desc, likert_color);

    desc = likert(xsav, 7);
    display_player_one_line(ENTRY_SKILL_SAVING, desc, likert_color);

    desc = likert((xstl > 0) ? xstl : -1, 1);
    display_player_one_line(ENTRY_SKILL_STEALTH, desc, likert_color);

    desc = likert(xfos, 6);
    display_player_one_line(ENTRY_SKILL_PERCEP, desc, likert_color);

    desc = likert(xsrh, 6);
    display_player_one_line(ENTRY_SKILL_SEARCH, desc, likert_color);

    desc = likert(xdis, 8);
    display_player_one_line(ENTRY_SKILL_DISARM, desc, likert_color);

    desc = likert(xdev, 6);
    display_player_one_line(ENTRY_SKILL_DEVICE, desc, likert_color);

    desc = likert(xdev, 6);
    display_player_one_line(ENTRY_SKILL_DEVICE, desc, likert_color);

    desc = likert(xdig, 4);
    display_player_one_line(ENTRY_SKILL_DIG, desc, likert_color);

    if (!muta_att)
        display_player_one_line(ENTRY_BLOWS, format("%d+%d", blows1, blows2), TERM_L_BLUE);
    else
        display_player_one_line(ENTRY_BLOWS, format("%d+%d+%d", blows1, blows2, muta_att), TERM_L_BLUE);

    display_player_one_line(ENTRY_SHOTS, format("%d.%02d", shots, shot_frac), TERM_L_BLUE);

    if ((damage[0] + damage[1]) == 0)
        desc = "nil!";
    else
        desc = format("%d+%d", blows1 * damage[0] / 100, blows2 * damage[1] / 100);

    display_player_one_line(ENTRY_AVG_DMG, desc, TERM_L_BLUE);
    display_player_one_line(ENTRY_INFRA, format("%d feet", creature_ptr->see_infra * 10), TERM_WHITE);
}

/*!
 * @brief プレイヤーステータスの1ページ目各種詳細をまとめて表示する
 * Prints ratings on certain abilities
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param display_player_one_line 1行表示用のコールバック関数
 * @details
 * This code is "imitated" elsewhere to "dump" a character sheet.
 */
void display_player_various(player_type *creature_ptr)
{
    object_type *o_ptr;
    o_ptr = &creature_ptr->inventory_list[INVEN_BOW];
    int tmp = creature_ptr->to_h_b + o_ptr->to_h;
    int xthb = creature_ptr->skill_thb + (tmp * BTH_PLUS_ADJ);
    int shots = 0;
    int shot_frac = 0;
    calc_shot_params(creature_ptr, o_ptr, &shots, &shot_frac);

    int damage[2];
    int to_h[2];
    calc_two_hands(creature_ptr, damage, to_h);
    display_first_page(creature_ptr, xthb, damage, shots, shot_frac);
}
