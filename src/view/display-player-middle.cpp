#include "view/display-player-middle.h"
#include "combat/shoot.h"
#include "game-option/birth-options.h"
#include "game-option/special-options.h"
#include "inventory/inventory-slot-types.h"
#include "mind/stances-table.h"
#include "monster/monster-status.h"
#include "object-enchant/special-object-flags.h"
#include "object/tval-types.h"
#include "perception/object-perception.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/equipment-info.h"
#include "player-info/monk-data-type.h"
#include "player-info/race-types.h"
#include "player-status/player-hand-types.h"
#include "player/attack-defense-types.h"
#include "player/player-skill.h"
#include "player/player-status-flags.h"
#include "player/player-status-table.h"
#include "player/player-status.h"
#include "sv-definition/sv-bow-types.h"
#include "system/floor-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "timed-effect/player-deceleration.h"
#include "timed-effect/timed-effects.h"
#include "view/display-util.h"
#include "view/status-first-page.h"
#include "world/world.h"

/*!
 * @brief プレイヤーの打撃能力修正を表示する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param hand 武器の装備部位ID
 * @param hand_entry 項目ID
 */
static void display_player_melee_bonus(PlayerType *player_ptr, int hand, int hand_entry)
{
    HIT_PROB show_tohit = player_ptr->dis_to_h[hand];
    int show_todam = player_ptr->dis_to_d[hand];
    auto *o_ptr = &player_ptr->inventory_list[INVEN_MAIN_HAND + hand];

    if (o_ptr->is_known()) {
        show_tohit += o_ptr->to_h;
    }
    if (o_ptr->is_known()) {
        show_todam += o_ptr->to_d;
    }

    show_tohit += player_ptr->skill_thn / BTH_PLUS_ADJ;

    std::string buf = format("(%+d,%+d)", (int)show_tohit, (int)show_todam);
    if (!has_melee_weapon(player_ptr, INVEN_MAIN_HAND) && !has_melee_weapon(player_ptr, INVEN_SUB_HAND)) {
        display_player_one_line(ENTRY_BARE_HAND, buf, TERM_L_BLUE);
    } else if (has_two_handed_weapons(player_ptr)) {
        display_player_one_line(ENTRY_TWO_HANDS, buf, TERM_L_BLUE);
    } else {
        display_player_one_line(hand_entry, buf, TERM_L_BLUE);
    }
}

/*!
 * @brief 右手に比べて左手の表示ルーチンが複雑なので分離
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void display_sub_hand(PlayerType *player_ptr)
{
    if (can_attack_with_sub_hand(player_ptr)) {
        display_player_melee_bonus(player_ptr, 1, left_hander ? ENTRY_RIGHT_HAND2 : ENTRY_LEFT_HAND2);
        return;
    }

    PlayerClass pc(player_ptr);
    if (!pc.equals(PlayerClassType::MONK) || ((empty_hands(player_ptr, true) & EMPTY_HAND_MAIN) == 0)) {
        return;
    }

    if (pc.monk_stance_is(MonkStanceType::NONE)) {
        display_player_one_line(ENTRY_POSTURE, _("構えなし", "none"), TERM_YELLOW);
        return;
    }

    uint stance_num = enum2i(pc.get_monk_stance()) - 1;

    if (stance_num < monk_stances.size()) {
        display_player_one_line(ENTRY_POSTURE, format(_("%sの構え", "%s form"), monk_stances[stance_num].desc), TERM_YELLOW);
    }
}

/*!
 * @brief 弓による命中率とダメージの補正を表示する
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void display_bow_hit_damage(PlayerType *player_ptr)
{
    const auto &item = player_ptr->inventory_list[INVEN_BOW];
    auto show_tohit = player_ptr->dis_to_h_b;
    auto show_todam = 0;
    if (item.is_known()) {
        show_tohit += item.to_h;
        show_todam += item.to_d;
    }

    const auto tval = item.bi_key.tval();
    const auto median_skill_exp = PlayerSkill::weapon_exp_at(PlayerSkillRank::MASTER) / 2;
    const auto &weapon_exps = player_ptr->weapon_exp[tval];
    constexpr auto bow_magnification = 200;
    constexpr auto xbow_magnification = 400;
    if (tval == ItemKindType::NONE) {
        show_tohit += (weapon_exps[0] - median_skill_exp) / bow_magnification;
    } else {
        const auto sval = item.bi_key.sval().value();
        const auto weapon_exp = weapon_exps[sval];
        if (item.is_cross_bow()) {
            show_tohit += weapon_exp / xbow_magnification;
        } else {
            show_tohit += (weapon_exp - median_skill_exp) / bow_magnification;
        }
    }

    show_tohit += player_ptr->skill_thb / BTH_PLUS_ADJ;
    display_player_one_line(ENTRY_SHOOT_HIT_DAM, format("(%+d,%+d)", show_tohit, show_todam), TERM_L_BLUE);
}

/*!
 * @brief 射撃武器倍率を表示する
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void display_shoot_magnification(PlayerType *player_ptr)
{
    int tmul = 0;
    if (player_ptr->inventory_list[INVEN_BOW].is_valid()) {
        tmul = player_ptr->inventory_list[INVEN_BOW].get_arrow_magnification();
        if (player_ptr->xtra_might) {
            tmul++;
        }

        tmul = tmul * (100 + (int)(adj_str_td[player_ptr->stat_index[A_STR]]) - 128);
    }

    display_player_one_line(ENTRY_SHOOT_POWER, format("x%d.%02d", tmul / 100, tmul % 100), TERM_L_BLUE);
}

/*!
 * @brief プレイヤーの速度から表示色を決める
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param base_speed プレイヤーの速度
 */
static TERM_COLOR decide_speed_color(PlayerType *player_ptr, const int base_speed)
{
    TERM_COLOR attr;
    if (base_speed > 0) {
        if (!player_ptr->riding) {
            attr = TERM_L_GREEN;
        } else {
            attr = TERM_GREEN;
        }
    } else if (base_speed == 0) {
        if (!player_ptr->riding) {
            attr = TERM_L_BLUE;
        } else {
            attr = TERM_GREEN;
        }
    } else {
        if (!player_ptr->riding) {
            attr = TERM_L_UMBER;
        } else {
            attr = TERM_RED;
        }
    }

    return attr;
}

/*!
 * @brief 何らかの効果による一時的な速度変化を計算する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return プレイヤーの速度
 */
static int calc_temporary_speed(PlayerType *player_ptr)
{
    int tmp_speed = 0;
    if (!player_ptr->riding) {
        if (is_fast(player_ptr)) {
            tmp_speed += 10;
        }

        if (player_ptr->effects()->deceleration()->is_slow()) {
            tmp_speed -= 10;
        }

        if (player_ptr->lightspeed) {
            tmp_speed = 99;
        }
    } else {
        const auto &m_ref = player_ptr->current_floor_ptr->m_list[player_ptr->riding];
        if (m_ref.is_accelerated()) {
            tmp_speed += 10;
        }

        if (m_ref.is_decelerated()) {
            tmp_speed -= 10;
        }
    }

    return tmp_speed;
}

/*!
 * @brief プレイヤーの最終的な速度を表示する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param attr 表示色
 * @param base_speed プレイヤーの素の速度
 * @param tmp_speed アイテム等で一時的に変化した速度量
 */
static void display_player_speed(PlayerType *player_ptr, TERM_COLOR attr, int base_speed, int tmp_speed)
{
    std::string buf;
    if (tmp_speed) {
        if (!player_ptr->riding) {
            if (player_ptr->lightspeed) {
                buf = _("光速化 (+99)", "Lightspeed (+99)");
            } else {
                buf = format("(%+d%+d)", base_speed - tmp_speed, tmp_speed);
            }
        } else {
            buf = format(_("乗馬中 (%+d%+d)", "Riding (%+d%+d)"), base_speed - tmp_speed, tmp_speed);
        }

        if (tmp_speed > 0) {
            attr = TERM_YELLOW;
        } else {
            attr = TERM_VIOLET;
        }
    } else {
        if (!player_ptr->riding) {
            buf = format("(%+d)", base_speed);
        } else {
            buf = format(_("乗馬中 (%+d)", "Riding (%+d)"), base_speed);
        }
    }

    display_player_one_line(ENTRY_SPEED, buf, attr);
    display_player_one_line(ENTRY_LEVEL, format("%d", player_ptr->lev), TERM_L_GREEN);
}

/*!
 * @brief プレイヤーの現在経験値・最大経験値・次のレベルまでに必要な経験値を表示する
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void display_player_exp(PlayerType *player_ptr)
{
    PlayerRace pr(player_ptr);
    int e = pr.equals(PlayerRaceType::ANDROID) ? ENTRY_EXP_ANDR : ENTRY_CUR_EXP;
    if (player_ptr->exp >= player_ptr->max_exp) {
        display_player_one_line(e, format("%d", player_ptr->exp), TERM_L_GREEN);
    } else {
        display_player_one_line(e, format("%d", player_ptr->exp), TERM_YELLOW);
    }

    if (!pr.equals(PlayerRaceType::ANDROID)) {
        display_player_one_line(ENTRY_MAX_EXP, format("%d", player_ptr->max_exp), TERM_L_GREEN);
    }

    e = pr.equals(PlayerRaceType::ANDROID) ? ENTRY_EXP_TO_ADV_ANDR : ENTRY_EXP_TO_ADV;

    if (player_ptr->lev >= PY_MAX_LEVEL) {
        display_player_one_line(e, "*****", TERM_L_GREEN);
    } else if (pr.equals(PlayerRaceType::ANDROID)) {
        display_player_one_line(e, format("%d", player_exp_a[player_ptr->lev - 1] * player_ptr->expfact / 100), TERM_L_GREEN);
    } else {
        display_player_one_line(e, format("%d", player_exp[player_ptr->lev - 1] * player_ptr->expfact / 100), TERM_L_GREEN);
    }
}

/*!
 * @brief ゲーム内の経過時間を表示する
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void display_playtime_in_game(PlayerType *player_ptr)
{
    int day, hour, min;
    extract_day_hour_min(player_ptr, &day, &hour, &min);

    std::string buf;
    if (day < MAX_DAYS) {
        buf = format(_("%d日目 %2d:%02d", "Day %d %2d:%02d"), day, hour, min);
    } else {
        buf = format(_("*****日目 %2d:%02d", "Day ***** %2d:%02d"), hour, min);
    }

    display_player_one_line(ENTRY_DAY, buf, TERM_L_GREEN);

    if (player_ptr->chp >= player_ptr->mhp) {
        display_player_one_line(ENTRY_HP, format("%4d/%4d", player_ptr->chp, player_ptr->mhp), TERM_L_GREEN);
    } else if (player_ptr->chp > (player_ptr->mhp * hitpoint_warn) / 10) {
        display_player_one_line(ENTRY_HP, format("%4d/%4d", player_ptr->chp, player_ptr->mhp), TERM_YELLOW);
    } else {
        display_player_one_line(ENTRY_HP, format("%4d/%4d", player_ptr->chp, player_ptr->mhp), TERM_RED);
    }

    if (player_ptr->csp >= player_ptr->msp) {
        display_player_one_line(ENTRY_SP, format("%4d/%4d", player_ptr->csp, player_ptr->msp), TERM_L_GREEN);
    } else if (player_ptr->csp > (player_ptr->msp * mana_warn) / 10) {
        display_player_one_line(ENTRY_SP, format("%4d/%4d", player_ptr->csp, player_ptr->msp), TERM_YELLOW);
    } else {
        display_player_one_line(ENTRY_SP, format("%4d/%4d", player_ptr->csp, player_ptr->msp), TERM_RED);
    }
}

/*!
 * @brief 現実世界におけるプレイ時間を表示する
 * @param なし
 * @param なし
 */
static void display_real_playtime(void)
{
    uint32_t play_hour = w_ptr->play_time / (60 * 60);
    uint32_t play_min = (w_ptr->play_time / 60) % 60;
    uint32_t play_sec = w_ptr->play_time % 60;
    display_player_one_line(ENTRY_PLAY_TIME, format("%.2u:%.2u:%.2u", play_hour, play_min, play_sec), TERM_L_GREEN);
}

/*!
 * @brief プレイヤーステータス表示の中央部分を表示するサブルーチン
 * @param player_ptr プレイヤーへの参照ポインタ
 * Prints the following information on the screen.
 */
void display_player_middle(PlayerType *player_ptr)
{
    if (can_attack_with_main_hand(player_ptr)) {
        display_player_melee_bonus(player_ptr, 0, left_hander ? ENTRY_LEFT_HAND1 : ENTRY_RIGHT_HAND1);
    }

    display_sub_hand(player_ptr);
    display_bow_hit_damage(player_ptr);
    display_shoot_magnification(player_ptr);
    display_player_one_line(ENTRY_BASE_AC, format("[%d,%+d]", player_ptr->dis_ac, player_ptr->dis_to_a), TERM_L_BLUE);

    int base_speed = player_ptr->pspeed - STANDARD_SPEED;
    if (player_ptr->action == ACTION_SEARCH) {
        base_speed += 10;
    }

    TERM_COLOR attr = decide_speed_color(player_ptr, base_speed);
    int tmp_speed = calc_temporary_speed(player_ptr);
    display_player_speed(player_ptr, attr, base_speed, tmp_speed);
    display_player_exp(player_ptr);
    display_player_one_line(ENTRY_GOLD, format("%d", player_ptr->au), TERM_L_GREEN);
    display_playtime_in_game(player_ptr);
    display_real_playtime();
}
