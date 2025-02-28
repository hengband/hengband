#pragma once

#include "monster-race/race-ability-flags.h"
#include "system/angband.h"
#include "util/flag-group.h"
#include "util/point-2d.h"
#include <string>
#include <vector>

enum mspell_lite_type {
    DO_SPELL_NONE = 0,
    DO_SPELL_BR_LITE = 1,
    DO_SPELL_BR_DISI = 2,
    DO_SPELL_BA_LITE = 3,
};

// Monster Spell Attack.
class MonsterEntity;
class MonraceDefinition;
class PlayerType;
struct msa_type {
    msa_type(PlayerType *player_ptr, MONSTER_IDX m_idx);

    POSITION x_br_lite = 0;
    POSITION y_br_lite = 0;
    bool in_no_magic_dungeon = false;
    bool success = false;
    std::vector<MonsterAbilityType> mspells{};
    std::string m_name = "";
    bool can_remember = false;
    int dam = 0;
    DEPTH rlev = 0;

    MONSTER_IDX m_idx;
    MonsterEntity *m_ptr;
    POSITION x;
    POSITION y;
    mspell_lite_type do_spell;
    MonsterAbilityType thrown_spell;

    MonraceDefinition *r_ptr;
    bool no_inate;
    EnumClassFlagGroup<MonsterAbilityType> ability_flags;

    Pos2D get_position() const;
    void set_position(const Pos2D &pos);
    Pos2D get_position_lite() const;
    void set_position_lite(const Pos2D &pos);
};
