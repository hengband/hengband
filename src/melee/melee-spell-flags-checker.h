#pragma once

typedef struct melee_spell_type melee_spell_type;
class PlayerType;
bool check_melee_spell_set(PlayerType *player_ptr, melee_spell_type *ms_ptr);
