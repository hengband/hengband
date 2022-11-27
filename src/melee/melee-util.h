#pragma once

#include "effect/attribute-types.h"
#include "monster-attack/monster-attack-effect.h"
#include "monster-attack/monster-attack-table.h"
#include "system/angband.h"

enum class BlowEffectType;

/* monster-attack-monster type*/
class MonsterEntity;
struct mam_type {
    BlowEffectType attribute;
    MONSTER_IDX m_idx;
    MONSTER_IDX t_idx;
    MonsterEntity *m_ptr;
    MonsterEntity *t_ptr;
    GAME_TEXT m_name[MAX_NLEN];
    GAME_TEXT t_name[MAX_NLEN];
    int damage;
    bool see_m;
    bool see_t;
    bool see_either;
    POSITION y_saver;
    POSITION x_saver;
    RaceBlowMethodType method;
    bool explode;
    bool touched;
    concptr act;
    AttributeType pt;
    RaceBlowEffectType effect;
    ARMOUR_CLASS ac;
    DEPTH rlev;
    bool blinked;
    bool do_silly_attack;
    int power;
    bool obvious;
    int d_dice;
    int d_side;
    bool known;
    bool fear;
    bool dead;
};

class PlayerType;
mam_type *initialize_mam_type(PlayerType *player_ptr, mam_type *mam_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx);
