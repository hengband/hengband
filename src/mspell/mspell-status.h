#pragma once

#include "system/angband.h"

struct MonsterSpellResult;

struct player_type;
void spell_badstatus_message_to_player(player_type *player_ptr, MONSTER_IDX m_idx, concptr msg1, concptr msg2, concptr msg3, concptr msg4, bool resist,
    bool saving_throw);
void spell_badstatus_message_to_mons(player_type *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, concptr msg1, concptr msg2, concptr msg3, concptr msg4, bool resist,
    bool saving_throw);
MonsterSpellResult spell_RF5_DRAIN_MANA(player_type *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
MonsterSpellResult spell_RF5_MIND_BLAST(player_type *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
MonsterSpellResult spell_RF5_BRAIN_SMASH(player_type *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
MonsterSpellResult spell_RF5_SCARE(MONSTER_IDX m_idx, player_type *player_ptr, MONSTER_IDX t_idx, int TARGET_TYPE);
MonsterSpellResult spell_RF5_BLIND(MONSTER_IDX m_idx, player_type *player_ptr, MONSTER_IDX t_idx, int TARGET_TYPE);
MonsterSpellResult spell_RF5_CONF(MONSTER_IDX m_idx, player_type *player_ptr, MONSTER_IDX t_idx, int TARGET_TYPE);
MonsterSpellResult spell_RF5_HOLD(MONSTER_IDX m_idx, player_type *player_ptr, MONSTER_IDX t_idx, int TARGET_TYPE);
MonsterSpellResult spell_RF6_HASTE(player_type *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
MonsterSpellResult spell_RF5_SLOW(MONSTER_IDX m_idx, player_type *player_ptr, MONSTER_IDX t_idx, int TARGET_TYPE);
MonsterSpellResult spell_RF6_HEAL(player_type *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
MonsterSpellResult spell_RF6_INVULNER(player_type *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE);
MonsterSpellResult spell_RF6_FORGET(player_type *player_ptr, MONSTER_IDX m_idx);
