#pragma once

#include "system/angband.h"

/* Spell Type flag */
#define MONSTER_TO_PLAYER 0x01
#define MONSTER_TO_MONSTER 0x02

/* monster spell number */
#define RF4_SPELL_START 32 * 3
#define RF5_SPELL_START 32 * 4
#define RF6_SPELL_START 32 * 5

struct floor_type;
struct player_type;
bool see_monster(player_type *player_ptr, MONSTER_IDX m_idx);
bool monster_near_player(floor_type* floor_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx);
bool monspell_message_base(player_type* target_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, concptr msg1, concptr msg2, concptr msg3, concptr msg4, bool msg_flag_aux, int TARGET_TYPE);
bool monspell_message(player_type* target_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, concptr msg1, concptr msg2, concptr msg3, int TARGET_TYPE);
void simple_monspell_message(player_type* target_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, concptr msg1, concptr msg2, int TARGET_TYPE);
