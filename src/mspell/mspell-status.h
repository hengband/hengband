#pragma once

#include "system/angband.h"

struct MonsterSpellResult;

class PlayerType;

struct mspell_cast_msg_bad_status_to_player {
    mspell_cast_msg_bad_status_to_player(concptr blind, concptr not_blind, concptr resist, concptr saved_throw);
    mspell_cast_msg_bad_status_to_player() = default;
    concptr blind; /*盲目時*/
    concptr not_blind; /*非盲目時*/
    concptr resist; /*耐性有*/
    concptr saved_throw; /*抵抗時*/
};

struct mspell_cast_msg_bad_status_to_monster {
    mspell_cast_msg_bad_status_to_monster(concptr default_msg, concptr resist, concptr saved_throw, concptr success);
    mspell_cast_msg_bad_status_to_monster() = default;
    concptr default_msg; /*通常時*/
    concptr resist; /*耐性有*/
    concptr saved_throw; /*抵抗時*/
    concptr success; /*成功時*/
};

void spell_badstatus_message_to_player(PlayerType *player_ptr, MONSTER_IDX m_idx, const mspell_cast_msg_bad_status_to_player &msgs, bool resist,
    bool saved_throw);
void spell_badstatus_message_to_mons(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, const mspell_cast_msg_bad_status_to_monster &msgs, bool resist,
    bool saved_throw);
MonsterSpellResult spell_RF5_DRAIN_MANA(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF5_MIND_BLAST(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF5_BRAIN_SMASH(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF5_SCARE(MONSTER_IDX m_idx, PlayerType *player_ptr, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF5_BLIND(MONSTER_IDX m_idx, PlayerType *player_ptr, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF5_CONF(MONSTER_IDX m_idx, PlayerType *player_ptr, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF5_HOLD(MONSTER_IDX m_idx, PlayerType *player_ptr, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_HASTE(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF5_SLOW(MONSTER_IDX m_idx, PlayerType *player_ptr, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_HEAL(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_INVULNER(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_FORGET(PlayerType *player_ptr, MONSTER_IDX m_idx);
