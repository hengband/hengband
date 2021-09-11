#pragma once

#include "system/angband.h"

#include "monster-race/race-ability-flags.h"
#include "util/flag-group.h"

#include <vector>

enum mspell_lite_type {
    DO_SPELL_NONE = 0,
    DO_SPELL_BR_LITE = 1,
    DO_SPELL_BR_DISI = 2,
    DO_SPELL_BA_LITE = 3,
};

// Monster Spell Attack.
struct monster_type;
struct monster_race;
typedef struct msa_type {
    MONSTER_IDX m_idx;
    monster_type *m_ptr;
    monster_race *r_ptr;
    bool no_inate;
    EnumClassFlagGroup<RF_ABILITY> ability_flags;
    POSITION x;
    POSITION y;
    POSITION x_br_lite;
    POSITION y_br_lite;
    mspell_lite_type do_spell;
    bool in_no_magic_dungeon;
    bool success;
    std::vector<RF_ABILITY> mspells;
    RF_ABILITY thrown_spell;
    GAME_TEXT m_name[MAX_NLEN];
    bool can_remember;
    int dam;
    DEPTH rlev;
} msa_type;

struct player_type;
msa_type *initialize_msa_type(player_type *player_ptr, msa_type *msa_ptr, MONSTER_IDX m_idx);
