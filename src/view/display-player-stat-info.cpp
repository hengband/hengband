/*!
 * @brief プレイヤーの耐性と能力値を表示する
 * @date 2020/02/27
 * @author Hourier
 * @details
 * ここにこれ以上関数を引っ越してくるのは禁止。何ならここから更に分割していく
 */

#include "view/display-player-stat-info.h"
#include "inventory/inventory-slot-types.h"
#include "mutation/mutation-flag-types.h"
#include "object-enchant/tr-types.h"
#include "object/object-flags.h"
#include "player-base/player-race.h"
#include "player-info/class-info.h"
#include "player-info/mimic-info-table.h"
#include "player/permanent-resistances.h"
#include "player/player-personality.h"
#include "player/player-status-table.h"
#include "player/player-status.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief プレイヤーのパラメータ基礎値 (腕力等)を18以下になるようにして返す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param stat_num 能力値番号
 * @return 基礎値
 * @details 最大が18になるのはD&D由来
 */
static int calc_basic_stat(PlayerType *player_ptr, int stat_num)
{
    int e_adj = 0;
    if ((player_ptr->stat_max[stat_num] > 18) && (player_ptr->stat_top[stat_num] > 18)) {
        e_adj = (player_ptr->stat_top[stat_num] - player_ptr->stat_max[stat_num]) / 10;
    }

    if ((player_ptr->stat_max[stat_num] <= 18) && (player_ptr->stat_top[stat_num] <= 18)) {
        e_adj = player_ptr->stat_top[stat_num] - player_ptr->stat_max[stat_num];
    }

    if ((player_ptr->stat_max[stat_num] <= 18) && (player_ptr->stat_top[stat_num] > 18)) {
        e_adj = (player_ptr->stat_top[stat_num] - 18) / 10 - player_ptr->stat_max[stat_num] + 18;
    }

    if ((player_ptr->stat_max[stat_num] > 18) && (player_ptr->stat_top[stat_num] <= 18)) {
        e_adj = player_ptr->stat_top[stat_num] - (player_ptr->stat_max[stat_num] - 19) / 10 - 19;
    }

    return e_adj;
}

/*!
 * @brief 特殊な種族の時、腕力等の基礎パラメータを変動させる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param stat_num 能力値番号
 * @return 補正後の基礎パラメータ
 */
static int compensate_special_race(PlayerType *player_ptr, int stat_num)
{
    if (!PlayerRace(player_ptr).equals(PlayerRaceType::ENT)) {
        return 0;
    }

    int r_adj = 0;
    switch (stat_num) {
    case A_STR:
    case A_CON:
        if (player_ptr->lev > 25) {
            r_adj++;
        }
        if (player_ptr->lev > 40) {
            r_adj++;
        }
        if (player_ptr->lev > 45) {
            r_adj++;
        }
        break;
    case A_DEX:
        if (player_ptr->lev > 25) {
            r_adj--;
        }
        if (player_ptr->lev > 40) {
            r_adj--;
        }
        if (player_ptr->lev > 45) {
            r_adj--;
        }
        break;
    }

    return r_adj;
}

/*!
 * @brief 能力値名を(もし一時的減少なら'x'を付けて)表示する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param stat_num 能力値番号
 * @param row 行数
 * @param stat_col 列数
 */
static void display_basic_stat_name(PlayerType *player_ptr, int stat_num, int row, int stat_col)
{
    if (player_ptr->stat_cur[stat_num] < player_ptr->stat_max[stat_num]) {
        c_put_str(TERM_WHITE, stat_names_reduced[stat_num], row + stat_num + 1, stat_col + 1);
    } else {
        c_put_str(TERM_WHITE, stat_names[stat_num], row + stat_num + 1, stat_col + 1);
    }
}

/*!
 * @brief 能力値を、基本・種族補正・職業補正・性格補正・装備補正・合計・現在 (一時的減少のみ) の順で表示する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param stat_num 能力値番号
 * @param r_adj 補正後の基礎パラメータ
 * @param e_adj 種族補正値
 * @param row 行数
 * @param stat_col 列数
 * @param buf 能力値の数値
 */
static void display_basic_stat_value(PlayerType *player_ptr, int stat_num, int r_adj, int e_adj, int row, int stat_col, char *buf)
{
    (void)sprintf(buf, "%3d", r_adj);
    c_put_str(TERM_L_BLUE, buf, row + stat_num + 1, stat_col + 13);

    (void)sprintf(buf, "%3d", (int)cp_ptr->c_adj[stat_num]);
    c_put_str(TERM_L_BLUE, buf, row + stat_num + 1, stat_col + 16);

    (void)sprintf(buf, "%3d", (int)ap_ptr->a_adj[stat_num]);
    c_put_str(TERM_L_BLUE, buf, row + stat_num + 1, stat_col + 19);

    (void)sprintf(buf, "%3d", (int)e_adj);
    c_put_str(TERM_L_BLUE, buf, row + stat_num + 1, stat_col + 22);

    cnv_stat(player_ptr->stat_top[stat_num], buf);
    c_put_str(TERM_L_GREEN, buf, row + stat_num + 1, stat_col + 26);

    if (player_ptr->stat_use[stat_num] < player_ptr->stat_top[stat_num]) {
        cnv_stat(player_ptr->stat_use[stat_num], buf);
        c_put_str(TERM_YELLOW, buf, row + stat_num + 1, stat_col + 33);
    }
}

/*!
 * @brief 能力値を補正しつつ表示する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param row 行数
 * @param stat_col 列数
 */
static void process_stats(PlayerType *player_ptr, int row, int stat_col)
{
    char buf[80];
    for (int i = 0; i < A_MAX; i++) {
        int r_adj = player_ptr->mimic_form != MimicKindType::NONE ? mimic_info.at(player_ptr->mimic_form).r_adj[i] : rp_ptr->r_adj[i];
        int e_adj = calc_basic_stat(player_ptr, i);
        r_adj += compensate_special_race(player_ptr, i);
        e_adj -= r_adj;
        e_adj -= cp_ptr->c_adj[i];
        e_adj -= ap_ptr->a_adj[i];

        display_basic_stat_name(player_ptr, i, row, stat_col);
        cnv_stat(player_ptr->stat_max[i], buf);
        if (player_ptr->stat_max[i] == player_ptr->stat_max_max[i]) {
            c_put_str(TERM_WHITE, "!", row + i + 1, _(stat_col + 6, stat_col + 4));
        }

        c_put_str(TERM_BLUE, buf, row + i + 1, stat_col + 13 - strlen(buf));

        display_basic_stat_value(player_ptr, i, r_adj, e_adj, row, stat_col, buf);
    }
}

/*!
 * @brief pval付きの装備に依るステータス補正を表示する
 * @param c 補正後の表示記号
 * @param a 表示色
 * @param o_ptr 装備品への参照ポインタ
 * @param stat 能力値番号
 * @param flags 装備品に立っているフラグ
 */
static void compensate_stat_by_weapon(char *c, TERM_COLOR *a, ItemEntity *o_ptr, tr_type tr_flag, const TrFlags &flags)
{
    *c = '*';

    if (o_ptr->pval > 0) {
        *a = TERM_L_GREEN;
        if (o_ptr->pval < 10) {
            *c = '0' + o_ptr->pval;
        }
    }

    if (flags.has(tr_flag)) {
        *a = TERM_GREEN;
    }

    if (o_ptr->pval < 0) {
        *a = TERM_RED;
        if (o_ptr->pval > -10) {
            *c = '0' - o_ptr->pval;
        }
    }
}

/*!
 * @brief 装備品を走査してpval付きのものをそれと分かるように表示する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param flags 装備品に立っているフラグ
 * @param row 行数
 * @param col 列数
 */
static void display_equipments_compensation(PlayerType *player_ptr, int row, int *col)
{
    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        ItemEntity *o_ptr;
        o_ptr = &player_ptr->inventory_list[i];
        auto flags = object_flags_known(o_ptr);
        for (int stat = 0; stat < A_MAX; stat++) {
            TERM_COLOR a = TERM_SLATE;
            char c = '.';
            if (flags.has(TR_STATUS_LIST[stat])) {
                compensate_stat_by_weapon(&c, &a, o_ptr, TR_SUST_STATUS_LIST[stat], flags);
            } else if (flags.has(TR_SUST_STATUS_LIST[stat])) {
                a = TERM_GREEN;
                c = 's';
            }

            term_putch(*col, row + stat + 1, a, c);
        }

        (*col)++;
    }
}

/*!
 * @brief 各能力値の補正
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param stat 能力値番号
 */
static int compensation_stat_by_mutation(PlayerType *player_ptr, int stat)
{
    int compensation = 0;
    if (stat == A_STR) {
        if (player_ptr->muta.has(PlayerMutationType::HYPER_STR)) {
            compensation += 4;
        }
        if (player_ptr->muta.has(PlayerMutationType::PUNY)) {
            compensation -= 4;
        }
        if (player_ptr->tsuyoshi) {
            compensation += 4;
        }
        return compensation;
    }

    if (stat == A_WIS || stat == A_INT) {
        if (player_ptr->muta.has(PlayerMutationType::HYPER_INT)) {
            compensation += 4;
        }
        if (player_ptr->muta.has(PlayerMutationType::MORONIC)) {
            compensation -= 4;
        }
        return compensation;
    }

    if (stat == A_DEX) {
        if (player_ptr->muta.has(PlayerMutationType::IRON_SKIN)) {
            compensation -= 1;
        }
        if (player_ptr->muta.has(PlayerMutationType::LIMBER)) {
            compensation += 3;
        }
        if (player_ptr->muta.has(PlayerMutationType::ARTHRITIS)) {
            compensation -= 3;
        }
        return compensation;
    }

    if (stat == A_CON) {
        if (player_ptr->muta.has(PlayerMutationType::RESILIENT)) {
            compensation += 4;
        }
        if (player_ptr->muta.has(PlayerMutationType::XTRA_FAT)) {
            compensation += 2;
        }
        if (player_ptr->muta.has(PlayerMutationType::ALBINO)) {
            compensation -= 4;
        }
        if (player_ptr->muta.has(PlayerMutationType::FLESH_ROT)) {
            compensation -= 2;
        }
        if (player_ptr->tsuyoshi) {
            compensation += 4;
        }
        return compensation;
    }

    if (stat == A_CHR) {
        if (player_ptr->muta.has(PlayerMutationType::SILLY_VOI)) {
            compensation -= 4;
        }
        if (player_ptr->muta.has(PlayerMutationType::BLANK_FAC)) {
            compensation -= 1;
        }
        if (player_ptr->muta.has(PlayerMutationType::FLESH_ROT)) {
            compensation -= 1;
        }
        if (player_ptr->muta.has(PlayerMutationType::SCALES)) {
            compensation -= 1;
        }
        if (player_ptr->muta.has(PlayerMutationType::WART_SKIN)) {
            compensation -= 2;
        }
        if (player_ptr->muta.has(PlayerMutationType::ILL_NORM)) {
            compensation = 0;
        }
        return compensation;
    }

    return 0;
}

/*!
 * @brief 突然変異 (と、つよしスペシャル)による能力値の補正有無で表示する記号を変える
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param stat 能力値番号
 * @param c 補正後の表示記号
 * @param a 表示色
 */
static void change_display_by_mutation(PlayerType *player_ptr, int stat, char *c, TERM_COLOR *a)
{
    int compensation = compensation_stat_by_mutation(player_ptr, stat);
    if (compensation == 0) {
        return;
    }

    *c = '*';
    if (compensation > 0) {
        *a = TERM_L_GREEN;
        if (compensation < 10) {
            *c = '0' + compensation;
        }
    }

    if (compensation < 0) {
        *a = TERM_RED;
        if (compensation > -10) {
            *c = '0' - compensation;
        }
    }
}

/*!
 * @brief 能力値を走査し、突然変異 (と、つよしスペシャル)で補正をかける必要があればかける
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param col 列数
 * @param row 行数
 */
static void display_mutation_compensation(PlayerType *player_ptr, int row, int col)
{
    TrFlags flags;
    player_flags(player_ptr, flags);

    for (int stat = 0; stat < A_MAX; stat++) {
        byte a = TERM_SLATE;
        char c = '.';
        change_display_by_mutation(player_ptr, stat, &c, &a);

        if (flags.has(TR_SUST_STATUS_LIST[stat])) {
            a = TERM_GREEN;
            c = 's';
        }

        term_putch(col, row + stat + 1, a, c);
    }
}

/*!
 * @brief プレイヤーの特性フラグ一覧表示2b /
 * Special display, part 2b
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * <pre>
 * How to print out the modifications and sustains.
 * Positive mods with no sustain will be light green.
 * Positive mods with a sustain will be dark green.
 * Sustains (with no modification) will be a dark green 's'.
 * Negative mods (from a curse) will be red.
 * Huge mods (>9), like from MICoMorgoth, will be a '*'
 * No mod, no sustain, will be a slate '.'
 * </pre>
 */
void display_player_stat_info(PlayerType *player_ptr)
{
    int stat_col = 22;
    int row = 3;
    c_put_str(TERM_WHITE, _("能力", "Stat"), row, stat_col + 1);
    c_put_str(TERM_BLUE, _("  基本", "  Base"), row, stat_col + 7);
    c_put_str(TERM_L_BLUE, _(" 種 職 性 装 ", "RacClaPerMod"), row, stat_col + 13);
    c_put_str(TERM_L_GREEN, _("合計", "Actual"), row, stat_col + 28);
    c_put_str(TERM_YELLOW, _("現在", "Current"), row, stat_col + 35);
    process_stats(player_ptr, row, stat_col);

    int col = stat_col + 41;
    c_put_str(TERM_WHITE, "abcdefghijkl@", row, col);
    c_put_str(TERM_L_GREEN, _("能力修正", "Modification"), row - 1, col);

    display_equipments_compensation(player_ptr, row, &col);
    display_mutation_compensation(player_ptr, row, col);
}
