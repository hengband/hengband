﻿#pragma once

#include "system/angband.h"
#include "mind/mind-types.h"

struct mind_power;
struct mind_type;
class player_type;
class MindPowerGetter {
public:
    MindPowerGetter(player_type *player_ptr);
    virtual ~MindPowerGetter() = default;
    bool get_mind_power(SPELL_IDX *sn, bool only_browse);

private:
    player_type *player_ptr;
    SPELL_IDX index = 0;
    int num = 0;
    TERM_LEN y = 1;
    TERM_LEN x = 10;
    bool ask = true;
    char choice = 0;
    concptr mind_description = "";
    const mind_type *spell = nullptr;
    bool flag = false;
    bool redraw = false;
    mind_kind_type use_mind;
    int menu_line;
    const mind_power *mind_ptr = nullptr;
    PERCENTAGE chance = 0;
    int mana_cost = 0;

    void select_mind_description();
    bool select_spell_index(SPELL_IDX *sn);
    bool decide_mind_choice(char *out_val, const bool only_browse);
    bool interpret_mind_key_input(const bool only_browse);
    bool display_minds_chance(const bool only_browse);
    void display_each_mind_chance();
    void calculate_mind_chance(bool *has_weapon); // 配列.
    void calculate_ki_chance(bool *has_weapon); // 配列.
    void add_ki_chance();
    void make_choice_lower();
};
