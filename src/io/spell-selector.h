#pragma once

#include "system/angband.h"

extern const uint32_t fake_spell_flags[4];

class PlayerType;
class SpellSelector {
public:
    SpellSelector(PlayerType *player_ptr);

    bool spell_okay(int spell, bool learned, bool study_pray, const short tmp_use_realm);
    bool get_spell(concptr prompt, OBJECT_SUBTYPE_VALUE sval, bool learned, const short tmp_use_realm, int tmp_sn = -2);
    int get_selected_spell();

private:
    PlayerType *player_ptr;
    int sn = 0;
    char choice = '\0';
    int menu_line = 0;
    bool ask = true;
    int num = 0;
    bool redraw = false;
    int spells[64]{};
    short use_realm = 0;
    
    bool on_key_down(int *spell_num);
    bool decide_redraw();
};
