#pragma once

#include "system/angband.h"

/* Spell Type flag */
#define MONSTER_TO_PLAYER 0x01
#define MONSTER_TO_MONSTER 0x02

/* monster spell number */
#define RF4_SPELL_START 32 * 3
#define RF5_SPELL_START 32 * 4
#define RF6_SPELL_START 32 * 5

class FloorType;
class PlayerType;

struct mspell_cast_msg {
    template <typename T, typename U, typename V, typename W>
    mspell_cast_msg(T &&to_player_true, U &&to_mons_true, V &&to_player_false, W &&to_mons_false)
        : to_player_true(std::forward<T>(to_player_true))
        , to_mons_true(std::forward<U>(to_mons_true))
        , to_player_false(std::forward<V>(to_player_false))
        , to_mons_false(std::forward<W>(to_mons_false))
    {
    }
    mspell_cast_msg() = default;
    std::string to_player_true; /*!< msg_flagがTRUEかつプレイヤー対象*/
    std::string to_mons_true; /*!< msg_flagがTRUEかつモンスター対象*/
    std::string to_player_false; /*!< msg_flagがFALSEかつプレイヤー対象*/
    std::string to_mons_false; /*!< msg_flagがFALSEかつモンスター対象*/
};

struct mspell_cast_msg_blind {
    mspell_cast_msg_blind(concptr blind, concptr to_player, concptr to_mons);
    mspell_cast_msg_blind() = default;
    concptr blind; /*!< 盲目時*/
    concptr to_player; /*!< 対プレイヤーかつ非盲目時*/
    concptr to_mons; /*!< 対モンスター*/
};

struct mspell_cast_msg_simple {
    mspell_cast_msg_simple(concptr to_player, concptr to_mons);
    mspell_cast_msg_simple() = default;
    concptr to_player; /*!< プレイヤー対象*/
    concptr to_mons; /*!< モンスター対象*/
};

bool see_monster(PlayerType *player_ptr, MONSTER_IDX m_idx);
bool monster_near_player(FloorType *floor_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx);
bool monspell_message_base(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, const mspell_cast_msg &msgs, bool msg_flag_aux, int target_type);
bool monspell_message(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, const mspell_cast_msg_blind &msgs, int target_type);
void simple_monspell_message(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, const mspell_cast_msg_simple &msgs, int target_type);
