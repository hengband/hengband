﻿#pragma once

#include "system/angband.h"
#include "spell/spells-util.h"

class player_type;
concptr exe_spell(player_type *player_ptr, int16_t realm, SPELL_IDX spell, spell_type mode);
