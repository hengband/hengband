#pragma once

#include "system/angband.h"
#include <string>

enum class AttributeType;
struct grid_type;
class MonsterEntity;
class MonsterRaceInfo;
class PlayerType;
class EffectMonster {
public:
    EffectMonster(PlayerType *player_ptr, MONSTER_IDX who, POSITION r, POSITION y, POSITION x, int dam, AttributeType attribute, BIT_FLAGS flag, bool see_s_msg);

    char killer[MAX_MONSTER_NAME]{};
    bool obvious = false;
    bool skipped = false;
    bool get_angry = false;
    bool do_polymorph = false;
    int do_dist = 0;
    int do_conf = 0;
    int do_stun = 0;
    int do_sleep = 0;
    int do_fear = 0;
    int do_time = 0;
    bool heal_leper = false;
    GAME_TEXT m_name[MAX_NLEN]{};
    char m_poss[10]{};
    short photo = 0;
    std::string note = "";

    MONSTER_IDX who;
    POSITION r;
    POSITION y;
    POSITION x;
    int dam;
    AttributeType attribute;
    BIT_FLAGS flag;
    bool see_s_msg;

    grid_type *g_ptr;
    MonsterEntity *m_ptr;
    MonsterEntity *m_caster_ptr;
    MonsterRaceInfo *r_ptr;
    bool seen;
    bool seen_msg;
    bool slept;
    bool known;
    std::string note_dies;
    int caster_lev;
};
