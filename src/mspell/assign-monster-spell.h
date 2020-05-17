#pragma once

HIT_POINT monspell_to_player(player_type* target_ptr, monster_spell_type ms_type, POSITION y, POSITION x, MONSTER_IDX m_idx);
HIT_POINT monspell_to_monster(player_type* target_ptr, monster_spell_type ms_type, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, bool is_special_spell);
