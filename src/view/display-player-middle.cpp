#include "view/display-player-middle.h"
#include "combat/shoot.h"
#include "game-option/birth-options.h"
#include "game-option/special-options.h"
#include "inventory/inventory-slot-types.h"
#include "mind/stances-table.h"
#include "monster/monster-status.h"
#include "object-enchant/special-object-flags.h"
#include "perception/object-perception.h"
#include "player-status/player-hand-types.h"
#include "player/attack-defense-types.h"
#include "player-info/equipment-info.h"
#include "player/player-race-types.h"
#include "player/player-skill.h"
#include "player/player-status-flags.h"
#include "player/player-status-table.h"
#include "player/player-status.h"
#include "sv-definition/sv-bow-types.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "term/term-color-types.h"
#include "view/display-util.h"
#include "view/status-first-page.h"
#include "world/world.h"

/*!
 * @brief プレイヤーの打撃能力修正を表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param hand 武器の装備部位ID
 * @param hand_entry 項目ID
 */
static void display_player_melee_bonus(player_type *creature_ptr, int hand, int hand_entry)
{
    HIT_PROB show_tohit = creature_ptr->dis_to_h[hand];
    HIT_POINT show_todam = creature_ptr->dis_to_d[hand];
    object_type *o_ptr = &creature_ptr->inventory_list[INVEN_MAIN_HAND + hand];

    if (object_is_known(o_ptr))
        show_tohit += o_ptr->to_h;
    if (object_is_known(o_ptr))
        show_todam += o_ptr->to_d;

    show_tohit += creature_ptr->skill_thn / BTH_PLUS_ADJ;

    char buf[160];
    sprintf(buf, "(%+d,%+d)", (int)show_tohit, (int)show_todam);

    if (!has_melee_weapon(creature_ptr, INVEN_MAIN_HAND) && !has_melee_weapon(creature_ptr, INVEN_SUB_HAND))
        display_player_one_line(ENTRY_BARE_HAND, buf, TERM_L_BLUE);
    else if (has_two_handed_weapons(creature_ptr))
        display_player_one_line(ENTRY_TWO_HANDS, buf, TERM_L_BLUE);
    else
        display_player_one_line(hand_entry, buf, TERM_L_BLUE);
}

/*!
 * @brief 右手に比べて左手の表示ルーチンが複雑なので分離
 * @param creature_ptr プレーヤーへの参照ポインタ
 */
static void display_sub_hand(player_type *creature_ptr)
{
    if (can_attack_with_sub_hand(creature_ptr)) {
        display_player_melee_bonus(creature_ptr, 1, left_hander ? ENTRY_RIGHT_HAND2 : ENTRY_LEFT_HAND2);
        return;
    }

    if ((creature_ptr->pclass != CLASS_MONK) || ((empty_hands(creature_ptr, true) & EMPTY_HAND_MAIN) == 0))
        return;

    if ((creature_ptr->special_defense & KAMAE_MASK) == 0) {
        display_player_one_line(ENTRY_POSTURE, _("構えなし", "none"), TERM_YELLOW);
        return;
    }

    int kamae_num;
    for (kamae_num = 0; kamae_num < MAX_KAMAE; kamae_num++) {
        if ((creature_ptr->special_defense >> kamae_num) & KAMAE_GENBU)
            break;
    }

    if (kamae_num < MAX_KAMAE) {
        display_player_one_line(ENTRY_POSTURE, format(_("%sの構え", "%s form"), monk_stances[kamae_num].desc), TERM_YELLOW);
    }
}

/*!
 * @brief 武器による命中率とダメージの補正を表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 */
static void display_hit_damage(player_type *creature_ptr)
{
    object_type *o_ptr = &creature_ptr->inventory_list[INVEN_BOW];
    HIT_PROB show_tohit = creature_ptr->dis_to_h_b;
    HIT_POINT show_todam = 0;
    if (object_is_known(o_ptr))
        show_tohit += o_ptr->to_h;
    if (object_is_known(o_ptr))
        show_todam += o_ptr->to_d;

    if ((o_ptr->sval == SV_LIGHT_XBOW) || (o_ptr->sval == SV_HEAVY_XBOW))
        show_tohit += creature_ptr->weapon_exp[0][o_ptr->sval] / 400;
    else
        show_tohit += (creature_ptr->weapon_exp[0][o_ptr->sval] - (WEAPON_EXP_MASTER / 2)) / 200;

    show_tohit += creature_ptr->skill_thb / BTH_PLUS_ADJ;

    display_player_one_line(ENTRY_SHOOT_HIT_DAM, format("(%+d,%+d)", show_tohit, show_todam), TERM_L_BLUE);
}

/*!
 * @brief 射撃武器倍率を表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 */
static void display_shoot_magnification(player_type *creature_ptr)
{
    int tmul = 0;
    if (creature_ptr->inventory_list[INVEN_BOW].k_idx) {
        tmul = bow_tmul(creature_ptr->inventory_list[INVEN_BOW].sval);
        if (creature_ptr->xtra_might)
            tmul++;

        tmul = tmul * (100 + (int)(adj_str_td[creature_ptr->stat_index[A_STR]]) - 128);
    }

    display_player_one_line(ENTRY_SHOOT_POWER, format("x%d.%02d", tmul / 100, tmul % 100), TERM_L_BLUE);
}

/*!
 * @brief プレーヤーの速度から表示色を決める
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param base_speed プレーヤーの速度
 */
static TERM_COLOR decide_speed_color(player_type *creature_ptr, const int base_speed)
{
    TERM_COLOR attr;
    if (base_speed > 0) {
        if (!creature_ptr->riding)
            attr = TERM_L_GREEN;
        else
            attr = TERM_GREEN;
    } else if (base_speed == 0) {
        if (!creature_ptr->riding)
            attr = TERM_L_BLUE;
        else
            attr = TERM_GREEN;
    } else {
        if (!creature_ptr->riding)
            attr = TERM_L_UMBER;
        else
            attr = TERM_RED;
    }

    return attr;
}

/*!
 * @brief 何らかの効果による一時的な速度変化を計算する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return プレーヤーの速度
 */
static int calc_temporary_speed(player_type *creature_ptr)
{
    int tmp_speed = 0;
    if (!creature_ptr->riding) {
        if (is_fast(creature_ptr))
            tmp_speed += 10;
        if (creature_ptr->slow)
            tmp_speed -= 10;
        if (creature_ptr->lightspeed)
            tmp_speed = 99;
    } else {
        if (monster_fast_remaining(&creature_ptr->current_floor_ptr->m_list[creature_ptr->riding]))
            tmp_speed += 10;
        if (monster_slow_remaining(&creature_ptr->current_floor_ptr->m_list[creature_ptr->riding]))
            tmp_speed -= 10;
    }

    return tmp_speed;
}

/*!
 * @brief プレーヤーの最終的な速度を表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param attr 表示色
 * @param base_speed プレーヤーの素の速度
 * @param tmp_speed アイテム等で一時的に変化した速度量
 */
static void display_player_speed(player_type *creature_ptr, TERM_COLOR attr, int base_speed, int tmp_speed)
{
    char buf[160];
    if (tmp_speed) {
        if (!creature_ptr->riding)
            if (creature_ptr->lightspeed) {
                sprintf(buf, _("光速化 (+99)", "Lightspeed (+99)"));
            } else {
                sprintf(buf, "(%+d%+d)", base_speed - tmp_speed, tmp_speed);
            }
        else
            sprintf(buf, _("乗馬中 (%+d%+d)", "Riding (%+d%+d)"), base_speed - tmp_speed, tmp_speed);

        if (tmp_speed > 0)
            attr = TERM_YELLOW;
        else
            attr = TERM_VIOLET;
    } else {
        if (!creature_ptr->riding)
            sprintf(buf, "(%+d)", base_speed);
        else
            sprintf(buf, _("乗馬中 (%+d)", "Riding (%+d)"), base_speed);
    }

    display_player_one_line(ENTRY_SPEED, buf, attr);
    display_player_one_line(ENTRY_LEVEL, format("%d", creature_ptr->lev), TERM_L_GREEN);
}

/*!
 * @brief プレーヤーの現在経験値・最大経験値・次のレベルまでに必要な経験値を表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 */
static void display_player_exp(player_type *creature_ptr)
{
    int e = (creature_ptr->prace == player_race_type::ANDROID) ? ENTRY_EXP_ANDR : ENTRY_CUR_EXP;
    if (creature_ptr->exp >= creature_ptr->max_exp)
        display_player_one_line(e, format("%ld", creature_ptr->exp), TERM_L_GREEN);
    else
        display_player_one_line(e, format("%ld", creature_ptr->exp), TERM_YELLOW);

    if (creature_ptr->prace != player_race_type::ANDROID)
        display_player_one_line(ENTRY_MAX_EXP, format("%ld", creature_ptr->max_exp), TERM_L_GREEN);

    e = (creature_ptr->prace == player_race_type::ANDROID) ? ENTRY_EXP_TO_ADV_ANDR : ENTRY_EXP_TO_ADV;

    if (creature_ptr->lev >= PY_MAX_LEVEL)
        display_player_one_line(e, "*****", TERM_L_GREEN);
    else if (creature_ptr->prace == player_race_type::ANDROID)
        display_player_one_line(e, format("%ld", (s32b)(player_exp_a[creature_ptr->lev - 1] * creature_ptr->expfact / 100L)), TERM_L_GREEN);
    else
        display_player_one_line(e, format("%ld", (s32b)(player_exp[creature_ptr->lev - 1] * creature_ptr->expfact / 100L)), TERM_L_GREEN);
}

/*!
 * @brief ゲーム内の経過時間を表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 */
static void display_playtime_in_game(player_type *creature_ptr)
{
    int day, hour, min;
    extract_day_hour_min(creature_ptr, &day, &hour, &min);

    char buf[160];
    if (day < MAX_DAYS)
        sprintf(buf, _("%d日目 %2d:%02d", "Day %d %2d:%02d"), day, hour, min);
    else
        sprintf(buf, _("*****日目 %2d:%02d", "Day ***** %2d:%02d"), hour, min);

    display_player_one_line(ENTRY_DAY, buf, TERM_L_GREEN);

    if (creature_ptr->chp >= creature_ptr->mhp)
        display_player_one_line(ENTRY_HP, format("%4d/%4d", creature_ptr->chp, creature_ptr->mhp), TERM_L_GREEN);
    else if (creature_ptr->chp > (creature_ptr->mhp * hitpoint_warn) / 10)
        display_player_one_line(ENTRY_HP, format("%4d/%4d", creature_ptr->chp, creature_ptr->mhp), TERM_YELLOW);
    else
        display_player_one_line(ENTRY_HP, format("%4d/%4d", creature_ptr->chp, creature_ptr->mhp), TERM_RED);

    if (creature_ptr->csp >= creature_ptr->msp)
        display_player_one_line(ENTRY_SP, format("%4d/%4d", creature_ptr->csp, creature_ptr->msp), TERM_L_GREEN);
    else if (creature_ptr->csp > (creature_ptr->msp * mana_warn) / 10)
        display_player_one_line(ENTRY_SP, format("%4d/%4d", creature_ptr->csp, creature_ptr->msp), TERM_YELLOW);
    else
        display_player_one_line(ENTRY_SP, format("%4d/%4d", creature_ptr->csp, creature_ptr->msp), TERM_RED);
}

/*!
 * @brief 現実世界におけるプレイ時間を表示する
 * @param なし
 * @param なし
 */
static void display_real_playtime(void)
{
    u32b play_hour = current_world_ptr->play_time / (60 * 60);
    u32b play_min = (current_world_ptr->play_time / 60) % 60;
    u32b play_sec = current_world_ptr->play_time % 60;
    display_player_one_line(ENTRY_PLAY_TIME, format("%.2lu:%.2lu:%.2lu", play_hour, play_min, play_sec), TERM_L_GREEN);
}

/*!
 * @brief プレイヤーステータス表示の中央部分を表示するサブルーチン
 * @param creature_ptr プレーヤーへの参照ポインタ
 * Prints the following information on the screen.
 */
void display_player_middle(player_type *creature_ptr)
{
    if (can_attack_with_main_hand(creature_ptr))
        display_player_melee_bonus(creature_ptr, 0, left_hander ? ENTRY_LEFT_HAND1 : ENTRY_RIGHT_HAND1);

    display_sub_hand(creature_ptr);
    display_hit_damage(creature_ptr);
    display_shoot_magnification(creature_ptr);
    display_player_one_line(ENTRY_BASE_AC, format("[%d,%+d]", creature_ptr->dis_ac, creature_ptr->dis_to_a), TERM_L_BLUE);

    int base_speed = creature_ptr->pspeed - 110;
    if (creature_ptr->action == ACTION_SEARCH)
        base_speed += 10;

    TERM_COLOR attr = decide_speed_color(creature_ptr, base_speed);
    int tmp_speed = calc_temporary_speed(creature_ptr);
    display_player_speed(creature_ptr, attr, base_speed, tmp_speed);
    display_player_exp(creature_ptr);
    display_player_one_line(ENTRY_GOLD, format("%ld", creature_ptr->au), TERM_L_GREEN);
    display_playtime_in_game(creature_ptr);
    display_real_playtime();
}
