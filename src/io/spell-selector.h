#pragma once

#include "system/angband.h"

extern const uint32_t fake_spell_flags[4];

class PlayerType;
class SpellSelector {
public:
    SpellSelector(PlayerType *player_ptr);

    bool spell_okay(int spell, bool learned, bool study_pray, const short tmp_use_realm);
    bool get_spell(concptr tmp_prompt, OBJECT_SUBTYPE_VALUE sval, bool learned, const short tmp_use_realm, int tmp_sn = -2);
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
    int spell_num = 0;
    char jverb_buf[128]{};
    concptr spell_category = nullptr;
    concptr prompt = nullptr;
    bool flag = false;
    int selected_spell = 0;

    bool on_key_down();
    bool decide_redraw();
    process_result select_spell_number();
    bool ask_capital();
    bool can_use(const bool learned);
    bool loop_key_input(char *out_val, const bool learned);
    bool need_learning(OBJECT_SUBTYPE_VALUE sval);
};
