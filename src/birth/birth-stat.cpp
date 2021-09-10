#include "birth/birth-stat.h"
#include "birth/auto-roller.h"
#include "core/player-redraw-types.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "player-info/race-types.h"
#include "player/player-personality-types.h"
#include "player/player-personality.h"
#include "player/player-skill.h"
#include "spell/spells-status.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/player-type-definition.h"

/*! オートロール能力値の乱数分布 / emulate 5 + 1d3 + 1d4 + 1d5 by randint0(60) */
BASE_STATUS rand3_4_5[60] = {
    8, 9, 9, 9, 10, 10, 10, 10, 10, 10, /*00-09*/
    11, 11, 11, 11, 11, 11, 11, 11, 11, 12, /*10-19*/
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, /*20-29*/
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, /*30-49*/
    13, 14, 14, 14, 14, 14, 14, 14, 14, 14, /*40-49*/
    15, 15, 15, 15, 15, 15, 16, 16, 16, 17 /*50-59*/
};

/*!
 * @brief プレイヤーの能力値表現に基づいて加減算を行う。
 * @param value 現在の能力値
 * @param amount 加減算する値
 * @return 加減算の結果
 */
int adjust_stat(int value, int amount)
{
    if (amount < 0) {
        for (int i = 0; i < (0 - amount); i++) {
            if (value >= 18 + 10) {
                value -= 10;
            } else if (value > 18) {
                value = 18;
            } else if (value > 3) {
                value--;
            }
        }
    } else if (amount > 0) {
        for (int i = 0; i < amount; i++) {
            if (value < 18) {
                value++;
            } else {
                value += 10;
            }
        }
    }

    return value;
}

/*!
 * @brief プレイヤーの能力値を一通りロールする。 / Roll for a characters stats
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @details
 * calc_bonuses()による、独立ステータスからの副次ステータス算出も行っている。
 * For efficiency, we include a chunk of "calc_bonuses()".\n
 */
void get_stats(player_type *creature_ptr)
{
    while (true) {
        int sum = 0;
        for (int i = 0; i < 2; i++) {
            int32_t tmp = randint0(60 * 60 * 60);
            BASE_STATUS val;

            for (int j = 0; j < 3; j++) {
                int stat = i * 3 + j;

                /* Extract 5 + 1d3 + 1d4 + 1d5 */
                val = rand3_4_5[tmp % 60];

                sum += val;
                creature_ptr->stat_cur[stat] = creature_ptr->stat_max[stat] = val;

                tmp /= 60;
            }
        }

        if ((sum > 42 + 5 * 6) && (sum < 57 + 5 * 6))
            break;
    }
}

/*!
 * @brief 経験値修正の合計値を計算
 */
uint16_t get_expfact(player_type *creature_ptr)
{
    uint16_t expfact = rp_ptr->r_exp;

    if (creature_ptr->prace != player_race_type::ANDROID)
        expfact += cp_ptr->c_exp;
    if (((creature_ptr->pclass == CLASS_MONK) || (creature_ptr->pclass == CLASS_FORCETRAINER) || (creature_ptr->pclass == CLASS_NINJA))
        && ((creature_ptr->prace == player_race_type::KLACKON) || (creature_ptr->prace == player_race_type::SPRITE)))
        expfact -= 15;

    return expfact;
}

/*!
 * @brief その他「オートローラ中は算出の対象にしない」副次ステータスを処理する / Roll for some info that the auto-roller ignores
 */
void get_extra(player_type *creature_ptr, bool roll_hitdie)
{
    creature_ptr->expfact = get_expfact(creature_ptr);

    /* Reset record of race/realm changes */
    creature_ptr->start_race = creature_ptr->prace;
    creature_ptr->old_race1 = 0L;
    creature_ptr->old_race2 = 0L;
    creature_ptr->old_realm = 0;

    for (int i = 0; i < 64; i++) {
        if (creature_ptr->pclass == CLASS_SORCERER)
            creature_ptr->spell_exp[i] = SPELL_EXP_MASTER;
        else if (creature_ptr->pclass == CLASS_RED_MAGE)
            creature_ptr->spell_exp[i] = SPELL_EXP_SKILLED;
        else
            creature_ptr->spell_exp[i] = SPELL_EXP_UNSKILLED;
    }

    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 64; j++)
            creature_ptr->weapon_exp[i][j] = s_info[creature_ptr->pclass].w_start[i][j];

    if ((creature_ptr->pseikaku == PERSONALITY_SEXY) && (creature_ptr->weapon_exp[TV_HAFTED - TV_WEAPON_BEGIN][SV_WHIP] < WEAPON_EXP_BEGINNER)) {
        creature_ptr->weapon_exp[TV_HAFTED - TV_WEAPON_BEGIN][SV_WHIP] = WEAPON_EXP_BEGINNER;
    }

    for (int i = 0; i < MAX_SKILLS; i++)
        creature_ptr->skill_exp[i] = s_info[creature_ptr->pclass].s_start[i];

    if (creature_ptr->pclass == CLASS_SORCERER)
        creature_ptr->hitdie = rp_ptr->r_mhp / 2 + cp_ptr->c_mhp + ap_ptr->a_mhp;
    else
        creature_ptr->hitdie = rp_ptr->r_mhp + cp_ptr->c_mhp + ap_ptr->a_mhp;

    if (roll_hitdie)
        roll_hitdice(creature_ptr, SPOP_NO_UPDATE);

    creature_ptr->mhp = creature_ptr->player_hp[0];
}

/*!
 * @brief プレイヤーの限界ステータスを決める。
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @details 新生の薬やステータスシャッフルでもこの関数が呼ばれる
 */
void get_max_stats(player_type *creature_ptr)
{
    int dice[6];
    while (true) {
        int j = 0;
        for (int i = 0; i < A_MAX; i++) {
            dice[i] = randint1(7);
            j += dice[i];
        }

        if (j == 24)
            break;
    }

    for (int i = 0; i < A_MAX; i++) {
        BASE_STATUS max_max = 18 + 60 + dice[i] * 10;
        creature_ptr->stat_max_max[i] = max_max;
        if (creature_ptr->stat_max[i] > max_max)
            creature_ptr->stat_max[i] = max_max;
        if (creature_ptr->stat_cur[i] > max_max)
            creature_ptr->stat_cur[i] = max_max;
    }

    creature_ptr->knowledge &= ~(KNOW_STAT);
    creature_ptr->redraw |= (PR_STATS);
}
