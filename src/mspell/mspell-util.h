#pragma once

#include "system/angband.h"
#include <fmt/base.h>
#include <string_view>

/* Spell Type flag */
#define MONSTER_TO_PLAYER 0x01
#define MONSTER_TO_MONSTER 0x02

/* monster spell number */
#define RF4_SPELL_START 32 * 3
#define RF5_SPELL_START 32 * 4
#define RF6_SPELL_START 32 * 5

class FloorType;
class PlayerType;

/*!
 * @brief モンスターが呪文を唱えるときのメッセージの書式
 * @details 書式の引数は次のとおり。
 * - {0}: 唱えたモンスターの名前 (先頭を大文字にする)
 * - {1}: 対象のモンスターの名前
 * - {2}: 唱えたモンスターの名前 (先頭を大文字にしない)
 * - {3}: 唱えたモンスターの所有格 (英語版の "his" など)
 */
using MSpellMessageFormat = fmt::format_string<std::string_view, std::string_view, std::string_view, std::string_view>;

struct mspell_cast_msg {
    mspell_cast_msg(MSpellMessageFormat to_player_true, MSpellMessageFormat to_mons_true, MSpellMessageFormat to_player_false, MSpellMessageFormat to_mons_false);
    mspell_cast_msg();
    MSpellMessageFormat to_player_true; /*!< msg_flagがTRUEかつプレイヤー対象*/
    MSpellMessageFormat to_mons_true; /*!< msg_flagがTRUEかつモンスター対象*/
    MSpellMessageFormat to_player_false; /*!< msg_flagがFALSEかつプレイヤー対象*/
    MSpellMessageFormat to_mons_false; /*!< msg_flagがFALSEかつモンスター対象*/
};

struct mspell_cast_msg_blind {
    mspell_cast_msg_blind(MSpellMessageFormat blind, MSpellMessageFormat to_player, MSpellMessageFormat to_mons);
    mspell_cast_msg_blind();
    MSpellMessageFormat blind; /*!< 盲目時*/
    MSpellMessageFormat to_player; /*!< 対プレイヤーかつ非盲目時*/
    MSpellMessageFormat to_mons; /*!< 対モンスター*/
};

struct mspell_cast_msg_simple {
    mspell_cast_msg_simple(MSpellMessageFormat to_player, MSpellMessageFormat to_mons);
    mspell_cast_msg_simple();
    MSpellMessageFormat to_player; /*!< プレイヤー対象*/
    MSpellMessageFormat to_mons; /*!< モンスター対象*/
};

bool see_monster(PlayerType *player_ptr, MONSTER_IDX m_idx);
bool monster_near_player(const FloorType &floor, MONSTER_IDX m_idx, MONSTER_IDX t_idx);
bool monspell_message_base(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, const mspell_cast_msg &msgs, bool msg_flag_aux, int target_type);
bool monspell_message(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, const mspell_cast_msg_blind &msgs, int target_type);
void simple_monspell_message(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, const mspell_cast_msg_simple &msgs, int target_type);
