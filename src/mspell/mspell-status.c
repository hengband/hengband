#include "system/angband.h"
#include "mspell/mspell-status.h"
#include "mspell/monster-spell.h"
#include "mspell/mspell-util.h"
#include "player/player-move.h"
#include "monster/monster-status.h"

/*!
 * @brief 状態異常呪文のメッセージ処理関数。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param msg1 対プレイヤーなら盲目時メッセージ。対モンスターなら通常時メッセージ。
 * @param msg2 対プレイヤーなら非盲目時メッセージ。対モンスターなら耐性有メッセージ。
 * @param msg3 対プレイヤーなら耐性有メッセージ。対モンスターなら抵抗時メッセージ。
 * @param msg4 対プレイヤーなら抵抗時メッセージ。対モンスターなら成功時メッセージ。
 * @param resist 耐性の有無を判別するフラグ
 * @param saving_throw 抵抗に成功したか判別するフラグ
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 */
void spell_badstatus_message(player_type* target_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, concptr msg1, concptr msg2, concptr msg3, concptr msg4, bool resist, bool saving_throw, int TARGET_TYPE)
{
    floor_type* floor_ptr = target_ptr->current_floor_ptr;
    bool see_either = see_monster(floor_ptr, m_idx) || see_monster(floor_ptr, t_idx);
    bool see_t = see_monster(floor_ptr, t_idx);
    bool known = monster_near_player(floor_ptr, m_idx, t_idx);
    GAME_TEXT m_name[MAX_NLEN], t_name[MAX_NLEN];
    monster_name(target_ptr, m_idx, m_name);
    monster_name(target_ptr, t_idx, t_name);

    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        disturb(target_ptr, TRUE, TRUE);
        if (target_ptr->blind)
            msg_format(msg1, m_name);
        else
            msg_format(msg2, m_name);

        if (resist) {
            msg_print(msg3);
        } else if (saving_throw) {
            msg_print(msg4);
        }

        return;
    }

    if (TARGET_TYPE != MONSTER_TO_MONSTER)
        return;

    if (known) {
        if (see_either) {
            msg_format(msg1, m_name, t_name);
        } else {
            floor_ptr->monster_noise = TRUE;
        }
    }

    if (resist) {
        if (see_t)
            msg_format(msg2, t_name);
    } else if (saving_throw) {
        if (see_t)
            msg_format(msg3, t_name);
    } else {
        if (see_t)
            msg_format(msg4, t_name);
    }

    set_monster_csleep(target_ptr, t_idx, 0);
}
