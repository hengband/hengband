#pragma once

#include "system/angband.h"

extern const uint32_t fake_spell_flags[4];

class PlayerType;
bool spell_okay(PlayerType *player_ptr, int spell, bool learned, bool study_pray, int use_realm);
int get_spell(PlayerType *player_ptr, SPELL_IDX *sn, concptr prompt, OBJECT_SUBTYPE_VALUE sval, bool learned, int16_t use_realm);
