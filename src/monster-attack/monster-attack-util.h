#pragma once

#include "monster-attack/monster-attack-effect.h"
#include "monster-attack/monster-attack-types.h"
#include "system/angband.h"

struct monster_type;
struct object_type;
class MonsterAttackPlayer {
public:
#ifdef JP
    int abbreviate; // 2回目以降の省略表現フラグ.
#endif
    MONSTER_IDX m_idx;
    monster_type *m_ptr;
    concptr act;
    int do_cut;
    int do_stun;
    bool touched;
    RaceBlowMethodType method;
    bool explode;
    DEPTH rlev;
    GAME_TEXT m_name[MAX_NLEN];
    bool do_silly_attack;
    int d_dice;
    int d_side;
    object_type *o_ptr;
    bool obvious;
    HIT_POINT damage;
    RaceBlowEffectType effect;
    bool blinked;
    GAME_TEXT o_name[MAX_NLEN];
    HIT_POINT get_damage;
    GAME_TEXT ddesc[MAX_MONSTER_NAME];
    ARMOUR_CLASS ac;
    bool alive;
    bool fear;
};

class PlayerType;
MonsterAttackPlayer *initialize_MonsterAttackPlayer(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr, MONSTER_IDX m_idx);
