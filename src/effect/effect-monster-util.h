#pragma once

#include "effect/attribute-types.h"
#include "system/angband.h"

struct grid_type;
;
struct monster_type;
struct monster_race;
struct effect_monster_type {
    grid_type *g_ptr;
    monster_type *m_ptr;
    monster_type *m_caster_ptr;
    monster_race *r_ptr;
    char killer[MAX_MONSTER_NAME];
    bool seen;
    bool seen_msg;
    bool slept;
    bool obvious;
    bool known;
    bool skipped;
    bool get_angry;
    bool do_polymorph;
    int do_dist;
    int do_conf;
    int do_stun;
    int do_sleep;
    int do_fear;
    int do_time;
    bool heal_leper;
    GAME_TEXT m_name[MAX_NLEN];
    char m_poss[10];
    PARAMETER_VALUE photo;
    concptr note;
    concptr note_dies;
    DEPTH caster_lev;

    MONSTER_IDX who;
    POSITION r;
    POSITION y;
    POSITION x;
    int dam;
    AttributeType attribute;
    BIT_FLAGS flag;
    bool see_s_msg;
};

class PlayerType;
effect_monster_type *initialize_effect_monster(PlayerType *player_ptr, effect_monster_type *em_ptr, MONSTER_IDX who, POSITION r, POSITION y, POSITION x,
    int dam, AttributeType attribute, BIT_FLAGS flag, bool see_s_msg);
