#pragma once

typedef struct melee_spell_type melee_spell_type;
struct player_type;
bool check_melee_spell_set(player_type *target_ptr, melee_spell_type *ms_ptr);
