#include "mspell/mspell-util.h"
#include "core/disturbance.h"
#include "floor/geometry.h"
#include "grid/grid.h"
#include "monster/monster-info.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "timed-effect/player-blindness.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"

mspell_cast_msg::mspell_cast_msg(concptr to_player_true, concptr to_mons_true, concptr to_player_false, concptr to_mons_false)
    : to_player_true(to_player_true)
    , to_mons_true(to_mons_true)
    , to_player_false(to_player_false)
    , to_mons_false(to_mons_false)
{
}

mspell_cast_msg_blind::mspell_cast_msg_blind(concptr blind, concptr to_player, concptr to_mons)
    : blind(blind)
    , to_player(to_player)
    , to_mons(to_mons)
{
}

mspell_cast_msg_simple::mspell_cast_msg_simple(concptr to_player, concptr to_mons)
    : to_player(to_player)
    , to_mons(to_mons)
{
}

/*!
 * @brief プレイヤーがモンスターを見ることができるかの判定 /
 * @param floor_ptr 現在フロアへの参照ポインタ
 * @param m_idx モンスターID
 * @return プレイヤーがモンスターを見ることができるならTRUE、そうでなければFALSEを返す。
 */
bool see_monster(PlayerType *player_ptr, MONSTER_IDX m_idx)
{
    monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    return is_seen(player_ptr, m_ptr);
}

/*!
 * @brief モンスター2体がプレイヤーの近くに居るかの判定 /
 * @param floor_ptr 現在フロアへの参照ポインタ
 * @param m_idx モンスターID一体目
 * @param t_idx モンスターID二体目
 * @return モンスター2体のどちらかがプレイヤーの近くに居ればTRUE、どちらも遠ければFALSEを返す。
 */
bool monster_near_player(FloorType *floor_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx)
{
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    monster_type *t_ptr = &floor_ptr->m_list[t_idx];
    return (m_ptr->cdis <= MAX_PLAYER_SIGHT) || (t_ptr->cdis <= MAX_PLAYER_SIGHT);
}

/*!
 * @brief モンスターが呪文行使する際のメッセージを処理する汎用関数 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param msgs メッセージの構造体
 * @param msg_flag_aux メッセージを分岐するためのフラグ
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return メッセージを表示した場合trueを返す。
 */
bool monspell_message_base(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, const mspell_cast_msg &msgs, bool msg_flag_aux, int target_type)
{
    bool notice = false;
    FloorType *floor_ptr = player_ptr->current_floor_ptr;
    bool known = monster_near_player(floor_ptr, m_idx, t_idx);
    bool see_either = see_monster(player_ptr, m_idx) || see_monster(player_ptr, t_idx);
    bool mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    bool mon_to_player = (target_type == MONSTER_TO_PLAYER);
    GAME_TEXT m_name[MAX_NLEN], t_name[MAX_NLEN];
    monster_name(player_ptr, m_idx, m_name);
    monster_name(player_ptr, t_idx, t_name);

    if (mon_to_player || (mon_to_mon && known && see_either)) {
        disturb(player_ptr, true, true);
    }

    if (msg_flag_aux) {
        if (mon_to_player) {
            msg_format(msgs.to_player_true, m_name);
            notice = true;
        } else if (mon_to_mon && known && see_either) {
            msg_format(msgs.to_mons_true, m_name);
            notice = true;
        }
    } else {
        if (mon_to_player) {
            msg_format(msgs.to_player_false, m_name);
            notice = true;
        } else if (mon_to_mon && known && see_either) {
            msg_format(msgs.to_mons_false, m_name, t_name);
            notice = true;
        }
    }

    if (mon_to_mon && known && !see_either) {
        floor_ptr->monster_noise = true;
    }

    return notice;
}

/*!
 * @brief モンスターが呪文行使する際のメッセージを処理する汎用関数。盲目時と通常時のメッセージを切り替える。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param msgs メッセージの構造体
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return メッセージを表示した場合trueを返す。
 */
bool monspell_message(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, const mspell_cast_msg_blind &msgs, int target_type)
{
    mspell_cast_msg mcm(msgs.blind, msgs.blind, msgs.to_player, msgs.to_mons);
    const auto is_blind = player_ptr->effects()->blindness()->is_blind();
    return monspell_message_base(player_ptr, m_idx, t_idx, mcm, is_blind, target_type);
}

/*!
 * @brief モンスターが呪文行使する際のメッセージを処理する汎用関数。対モンスターと対プレイヤーのメッセージを切り替える。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param msgs メッセージの構造体
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 */
void simple_monspell_message(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, const mspell_cast_msg_simple &msgs, int target_type)
{
    mspell_cast_msg mcm(msgs.to_player, msgs.to_mons, msgs.to_player, msgs.to_mons);
    const auto is_blind = player_ptr->effects()->blindness()->is_blind();
    monspell_message_base(player_ptr, m_idx, t_idx, mcm, is_blind, target_type);
}
