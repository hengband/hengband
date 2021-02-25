#pragma once

#include "system/angband.h"

typedef enum mspell_lite_type {
    DO_SPELL_NONE = 0,
    DO_SPELL_BR_LITE = 1,
    DO_SPELL_BR_DISI = 2,
    DO_SPELL_BA_LITE = 3,
} mspell_lite_type;

// Monster Spell Attack.
typedef struct monster_type monster_type;
typedef struct monster_race monster_race;
typedef struct msa_type {
    MONSTER_IDX m_idx;
    monster_type *m_ptr;
    monster_race *r_ptr;
    bool no_inate;
    BIT_FLAGS f4;
    BIT_FLAGS f5;
    BIT_FLAGS f6;
    POSITION x;
    POSITION y;
    POSITION x_br_lite;
    POSITION y_br_lite;
    mspell_lite_type do_spell;
    bool in_no_magic_dungeon;
    bool success;
    byte mspells[96];
    byte num;
    SPELL_IDX thrown_spell;
    GAME_TEXT m_name[MAX_NLEN];
    bool can_remember;
    int dam;
    DEPTH rlev;
} msa_type;

msa_type *initialize_msa_type(player_type *target_ptr, msa_type *msa_ptr, MONSTER_IDX m_idx);
