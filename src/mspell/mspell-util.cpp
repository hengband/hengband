#include "mspell/mspell-util.h"
#include "core/disturbance.h"
#include "floor/geometry.h"
#include "grid/grid.h"
#include "monster/monster-info.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
* @brief プレイヤーがモンスターを見ることができるかの判定 /
* @param floor_ptr 現在フロアへの参照ポインタ
* @param m_idx モンスターID
* @return プレイヤーがモンスターを見ることができるならTRUE、そうでなければFALSEを返す。
*/
bool see_monster(player_type* player_ptr, MONSTER_IDX m_idx)
{
    monster_type* m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    return is_seen(player_ptr, m_ptr);
}

/*!
 * @brief モンスター2体がプレイヤーの近くに居るかの判定 /
 * @param floor_ptr 現在フロアへの参照ポインタ
 * @param m_idx モンスターID一体目
 * @param t_idx モンスターID二体目
 * @return モンスター2体のどちらかがプレイヤーの近くに居ればTRUE、どちらも遠ければFALSEを返す。
 */
bool monster_near_player(floor_type* floor_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx)
{
    monster_type* m_ptr = &floor_ptr->m_list[m_idx];
    monster_type* t_ptr = &floor_ptr->m_list[t_idx];
    return (m_ptr->cdis <= MAX_SIGHT) || (t_ptr->cdis <= MAX_SIGHT);
}

/*!
 * @brief モンスターが呪文行使する際のメッセージを処理する汎用関数 /
* @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param msg1 msg_flagがTRUEで、プレイヤーを対象とする場合のメッセージ
 * @param msg2 msg_flagがTRUEで、モンスターを対象とする場合のメッセージ
 * @param msg3 msg_flagがFALSEで、プレイヤーを対象とする場合のメッセージ
 * @param msg4 msg_flagがFALSEで、モンスターを対象とする場合のメッセージ
 * @param msg_flag_aux メッセージを分岐するためのフラグ
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return メッセージを表示した場合trueを返す。
 */
bool monspell_message_base(player_type* player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, concptr msg1, concptr msg2, concptr msg3, concptr msg4, bool msg_flag_aux, int TARGET_TYPE)
{
    bool notice = false;
    floor_type* floor_ptr = player_ptr->current_floor_ptr;
    bool known = monster_near_player(floor_ptr, m_idx, t_idx);
    bool see_either = see_monster(player_ptr, m_idx) || see_monster(player_ptr, t_idx);
    bool mon_to_mon = (TARGET_TYPE == MONSTER_TO_MONSTER);
    bool mon_to_player = (TARGET_TYPE == MONSTER_TO_PLAYER);
    GAME_TEXT m_name[MAX_NLEN], t_name[MAX_NLEN];
    monster_name(player_ptr, m_idx, m_name);
    monster_name(player_ptr, t_idx, t_name);

    if (mon_to_player || (mon_to_mon && known && see_either))
        disturb(player_ptr, true, true);

    if (msg_flag_aux) {
        if (mon_to_player) {
            msg_format(msg1, m_name);
            notice = true;
        } else if (mon_to_mon && known && see_either) {
            msg_format(msg2, m_name);
            notice = true;
        }
    } else {
        if (mon_to_player) {
            msg_format(msg3, m_name);
            notice = true;
        } else if (mon_to_mon && known && see_either) {
            msg_format(msg4, m_name, t_name);
            notice = true;
        }
    }

    if (mon_to_mon && known && !see_either)
        floor_ptr->monster_noise = true;

    return notice;
}

/*!
* @brief モンスターが呪文行使する際のメッセージを処理する汎用関数。盲目時と通常時のメッセージを切り替える。 /
* @param player_ptr プレイヤーへの参照ポインタ
* @param m_idx 呪文を唱えるモンスターID
* @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
* @param msg1 プレイヤーが盲目状態のメッセージ
* @param msg2 プレイヤーが盲目でなく、プレイヤーを対象とする場合のメッセージ
* @param msg3 プレイヤーが盲目でなく、モンスター対象とする場合のメッセージ
* @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return メッセージを表示した場合trueを返す。
 */
bool monspell_message(player_type* player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, concptr msg1, concptr msg2, concptr msg3, int TARGET_TYPE)
{
    return monspell_message_base(player_ptr, m_idx, t_idx, msg1, msg1, msg2, msg3, player_ptr->blind > 0, TARGET_TYPE);
}

/*!
* @brief モンスターが呪文行使する際のメッセージを処理する汎用関数。対モンスターと対プレイヤーのメッセージを切り替える。 /
* @param player_ptr プレイヤーへの参照ポインタ
* @param m_idx 呪文を唱えるモンスターID
* @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
* @param msg1 プレイヤーを対象とする場合のメッセージ
* @param msg2 モンスター対象とする場合のメッセージ
* @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
*/
void simple_monspell_message(player_type* player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, concptr msg1, concptr msg2, int TARGET_TYPE)
{
    monspell_message_base(player_ptr, m_idx, t_idx, msg1, msg2, msg1, msg2, player_ptr->blind > 0, TARGET_TYPE);
}
