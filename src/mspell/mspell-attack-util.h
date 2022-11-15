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
class MonsterEntity;
class MonsterRaceInfo;
struct msa_type {
    MONSTER_IDX m_idx;
    MonsterEntity *m_ptr;
    MonsterRaceInfo *r_ptr;
    bool no_inate;
    EnumClassFlagGroup<MonsterAbilityType> ability_flags;
    POSITION x;
    POSITION y;
    POSITION x_br_lite;
    POSITION y_br_lite;
    mspell_lite_type do_spell;
    bool in_no_magic_dungeon;
    bool success;
    std::vector<MonsterAbilityType> mspells;
    MonsterAbilityType thrown_spell;
    GAME_TEXT m_name[MAX_NLEN];
    bool can_remember;
    int dam;
    DEPTH rlev;
};

class PlayerType;
msa_type *initialize_msa_type(PlayerType *player_ptr, msa_type *msa_ptr, MONSTER_IDX m_idx);
