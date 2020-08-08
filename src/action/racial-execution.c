#include "action/racial-execution.h"
#include "action/action-limited.h"
#include "core/asking-player.h"
#include "game-option/disturbance-options.h"
#include "racial/racial-switcher.h"
#include "racial/racial-util.h"
#include "term/screen-processor.h"
#include "view/display-messages.h"

/*!
 * @brief レイシャル・パワー発動処理
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param command 発動するレイシャルのID
 * @return 処理を実際に実行した場合はTRUE、キャンセルした場合FALSEを返す。
 */
bool exe_racial_power(player_type *creature_ptr, const s32b command)
{
    if (command <= -3)
        return switch_class_racial_execution(creature_ptr, command);

    if (creature_ptr->mimic_form)
        return switch_mimic_racial_execution(creature_ptr);

    return switch_race_racial_execution(creature_ptr, command);
}

/*!
 * @brief レイシャル・パワーの発動成功率を計算する / Returns the chance to activate a racial power/mutation
 * @param rpi_ptr 発動したいレイシャル・パワー情報の構造体参照ポインタ
 * @return 成功率(%)を返す
 */
PERCENTAGE racial_chance(player_type *creature_ptr, rpi_type *rpi_ptr)
{
    if ((creature_ptr->lev < rpi_ptr->min_level) || creature_ptr->confused)
        return 0;

    PERCENTAGE difficulty = rpi_ptr->fail;
    if (difficulty == 0)
        return 100;

    if (creature_ptr->stun) {
        difficulty += (PERCENTAGE)creature_ptr->stun;
    } else if (creature_ptr->lev > rpi_ptr->min_level) {
        PERCENTAGE lev_adj = (PERCENTAGE)((creature_ptr->lev - rpi_ptr->min_level) / 3);
        if (lev_adj > 10)
            lev_adj = 10;

        difficulty -= lev_adj;
    }

    if (difficulty < 5)
        difficulty = 5;

    difficulty = difficulty / 2;
    const BASE_STATUS stat = creature_ptr->stat_cur[rpi_ptr->stat];
    int sum = 0;
    for (int i = 1; i <= stat; i++) {
        int val = i - difficulty;
        if (val > 0)
            sum += (val <= difficulty) ? val : difficulty;
    }

    if (difficulty == 0)
        return 100;
    else
        return ((sum * 100) / difficulty) / stat;
}

/*!
 * @brief レイシャル・パワーの発動の判定処理
 * @param rpi_ptr 発動したいレイシャル・パワー情報の構造体参照ポインタ
 * @return
 * 発動成功ならば1、発動失敗ならば-1、キャンセルならば0を返す。
 * return value indicates that we have succesfully used the power 1: Succeeded, 0: Cancelled, -1: Failed
 */
int check_racial_level(player_type *creature_ptr, rpi_type *rpi_ptr)
{
    PLAYER_LEVEL min_level = rpi_ptr->min_level;
    int use_stat = rpi_ptr->stat;
    int difficulty = rpi_ptr->fail;
    int use_hp = 0;
    rpi_ptr->racial_cost = rpi_ptr->cost;
    if (creature_ptr->csp < rpi_ptr->racial_cost)
        use_hp = rpi_ptr->racial_cost - creature_ptr->csp;

    if (creature_ptr->lev < min_level) {
        msg_format(_("この能力を使用するにはレベル %d に達していなければなりません。", "You need to attain level %d to use this power."), min_level);
        free_turn(creature_ptr);
        return FALSE;
    }

    if (cmd_limit_confused(creature_ptr)) {
        free_turn(creature_ptr);
        return FALSE;
    } else if (creature_ptr->chp < use_hp) {
        if (!get_check(_("本当に今の衰弱した状態でこの能力を使いますか？", "Really use the power in your weakened state? "))) {
            free_turn(creature_ptr);
            return FALSE;
        }
    }

    if (difficulty) {
        if (creature_ptr->stun) {
            difficulty += creature_ptr->stun;
        } else if (creature_ptr->lev > min_level) {
            int lev_adj = ((creature_ptr->lev - min_level) / 3);
            if (lev_adj > 10)
                lev_adj = 10;
            difficulty -= lev_adj;
        }

        if (difficulty < 5)
            difficulty = 5;
    }

    take_turn(creature_ptr, 100);
    if (randint1(creature_ptr->stat_cur[use_stat]) >= ((difficulty / 2) + randint1(difficulty / 2)))
        return 1;

    if (flush_failure)
        flush();

    msg_print(_("充分に集中できなかった。", "You've failed to concentrate hard enough."));
    return -1;
}
