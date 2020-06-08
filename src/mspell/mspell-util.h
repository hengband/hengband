#pragma once

#include "system/angband.h"

bool see_monster(floor_type* floor_ptr, MONSTER_IDX m_idx);
bool monster_near_player(floor_type* floor_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx);
void monspell_message_base(player_type* target_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, concptr msg1, concptr msg2, concptr msg3, concptr msg4, bool msg_flag_aux, int TARGET_TYPE);
void monspell_message(player_type* target_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, concptr msg1, concptr msg2, concptr msg3, int TARGET_TYPE);
void simple_monspell_message(player_type* target_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, concptr msg1, concptr msg2, int TARGET_TYPE);
