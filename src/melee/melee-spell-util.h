#pragma once

#include "system/angband.h"
#include "system/monster-type-definition.h"

typedef struct melee_spell_type {
    MONSTER_IDX m_idx;
    POSITION y;
    POSITION x;
    MONSTER_IDX target_idx;
    int thrown_spell;
    HIT_POINT dam;
    byte spell[96];
    byte num;
    GAME_TEXT m_name[160];
#ifdef JP
#else
    char m_poss[160];
#endif

    monster_type *m_ptr;
    monster_type *t_ptr;
    monster_race *r_ptr;
    bool see_m;
    bool maneable;
    bool pet;
    bool in_no_magic_dungeon;
    bool can_remember;
    BIT_FLAGS f4;
    BIT_FLAGS f5;
    BIT_FLAGS f6;
} melee_spell_type;

melee_spell_type *initialize_melee_spell_type(player_type *target_ptr, melee_spell_type *ms_ptr, MONSTER_IDX m_idx);
