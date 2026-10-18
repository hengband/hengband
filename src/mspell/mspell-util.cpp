#include "mspell/mspell-util.h"
#include "core/disturbance.h"
#include "floor/geometry.h"
#include "grid/grid.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "system/floor/floor-info.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "timed-effect/timed-effects.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include <fmt/format.h>

// 書式は空文字列にしておき、使う前に書き換える
mspell_cast_msg::mspell_cast_msg()
    : mspell_cast_msg("", "", "", "")
{
}

mspell_cast_msg::mspell_cast_msg(MSpellMessageFormat to_player_true, MSpellMessageFormat to_mons_true, MSpellMessageFormat to_player_false, MSpellMessageFormat to_mons_false)
    : to_player_true(to_player_true)
    , to_mons_true(to_mons_true)
    , to_player_false(to_player_false)
    , to_mons_false(to_mons_false)
{
}

mspell_cast_msg_blind::mspell_cast_msg_blind()
    : mspell_cast_msg_blind("", "", "")
{
}

mspell_cast_msg_blind::mspell_cast_msg_blind(MSpellMessageFormat blind, MSpellMessageFormat to_player, MSpellMessageFormat to_mons)
    : blind(blind)
    , to_player(to_player)
    , to_mons(to_mons)
{
}

mspell_cast_msg_simple::mspell_cast_msg_simple()
    : mspell_cast_msg_simple("", "")
{
}

mspell_cast_msg_simple::mspell_cast_msg_simple(MSpellMessageFormat to_player, MSpellMessageFormat to_mons)
    : to_player(to_player)
    , to_mons(to_mons)
{
}

/*!
 * @brief モンスターが呪文を唱えるときのメッセージを出力する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param fmt メッセージの書式
 */
static void print_monspell_message(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, MSpellMessageFormat fmt)
{
    const auto &monster = player_ptr->current_floor_ptr->m_list[m_idx];
    const auto m_name = monster_name(player_ptr, m_idx);
    const auto m_name_upcased = str_upcase_first(m_name);
    const auto t_name = monster_name(player_ptr, t_idx);
    const auto t_name_upcased = str_upcase_first(t_name);
    const auto m_poss = monster_desc(player_ptr, monster, MD_PRON_VISIBLE | MD_POSSESSIVE);
    msg_print(fmt::vformat(fmt.get(), fmt::make_format_args(m_name_upcased, t_name, m_name, m_poss, t_name_upcased)));
}

/*!
 * @brief プレイヤーがモンスターを見ることができるかの判定 /
 * @param m_idx モンスターID
 * @return プレイヤーがモンスターを見ることができるならTRUE、そうでなければFALSEを返す。
 */
bool see_monster(PlayerType *player_ptr, MONSTER_IDX m_idx)
{
    const auto &monster = player_ptr->current_floor_ptr->m_list[m_idx];
    return is_seen(player_ptr, monster);
}

/*!
 * @brief モンスター2体がプレイヤーの近くに居るかの判定 /
 * @param floor フロアへの参照
 * @param m_idx モンスターID一体目
 * @param t_idx モンスターID二体目
 * @return モンスター2体のどちらかがプレイヤーの近くに居ればTRUE、どちらも遠ければFALSEを返す。
 */
bool monster_near_player(const FloorType &floor, MONSTER_IDX m_idx, MONSTER_IDX t_idx)
{
    const auto &monster = floor.m_list[m_idx];
    const auto &monster_target = floor.m_list[t_idx];
    return (monster.cdis <= MAX_PLAYER_SIGHT) || (monster_target.cdis <= MAX_PLAYER_SIGHT);
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
    auto &floor = *player_ptr->current_floor_ptr;
    const auto known = monster_near_player(floor, m_idx, t_idx);
    const auto see_either = see_monster(player_ptr, m_idx) || see_monster(player_ptr, t_idx);
    const auto mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    const auto mon_to_player = (target_type == MONSTER_TO_PLAYER);
    if (mon_to_mon && known && !see_either) {
        floor.monster_noise = true;
    }

    if (!mon_to_player && !(mon_to_mon && known && see_either)) {
        return false;
    }

    disturb(player_ptr, true, true);
    if (mon_to_player) {
        print_monspell_message(player_ptr, m_idx, t_idx, msg_flag_aux ? msgs.to_player_true : msgs.to_player_false);
    } else {
        print_monspell_message(player_ptr, m_idx, t_idx, msg_flag_aux ? msgs.to_mons_true : msgs.to_mons_false);
    }

    return true;
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
    const auto is_blind = player_ptr->effects()->blindness().is_active();
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
    const auto is_blind = player_ptr->effects()->blindness().is_active();
    monspell_message_base(player_ptr, m_idx, t_idx, mcm, is_blind, target_type);
}
