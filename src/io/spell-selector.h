﻿#pragma once

#include "system/angband.h"

extern const uint32_t fake_spell_flags[4];

class PlayerType;
class SpellSelector {
public:
    SpellSelector(PlayerType *player_ptr);

    bool spell_okay(int spell, bool learned, bool study_pray, int use_realm);
    bool get_spell(concptr prompt, OBJECT_SUBTYPE_VALUE sval, bool learned, int16_t use_realm, int tmp_sn = -2);
    int get_selected_spell();

private:
    PlayerType *player_ptr;
    int sn = 0;
};
