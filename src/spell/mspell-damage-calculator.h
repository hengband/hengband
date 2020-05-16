#pragma once

HIT_POINT monspell_damage(player_type* target_ptr, monster_spell_type ms_type, MONSTER_IDX m_idx, int TYPE);
HIT_POINT monspell_race_damage(player_type* target_ptr, monster_spell_type ms_type, MONRACE_IDX r_idx, int TYPE);
HIT_POINT monspell_bluemage_damage(player_type* target_ptr, monster_spell_type ms_type, PLAYER_LEVEL plev, int TYPE);
