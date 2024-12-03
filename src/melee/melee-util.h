#pragma once

#include "system/angband.h"
#include "util/dice.h"

enum class AttributeType;
enum class BlowEffectType;
enum class RaceBlowEffectType;
enum class RaceBlowMethodType;

/* monster-attack-monster type*/
class MonsterEntity;
struct mam_type {
    BlowEffectType attribute{};
    MONSTER_IDX m_idx = 0;
    MONSTER_IDX t_idx = 0;
    MonsterEntity *m_ptr = nullptr;
    MonsterEntity *t_ptr = nullptr;
    GAME_TEXT m_name[MAX_NLEN]{};
    GAME_TEXT t_name[MAX_NLEN]{};
    int damage = 0;
    bool see_m = false;
    bool see_t = false;
    bool see_either = false;
    POSITION y_saver = 0;
    POSITION x_saver = 0;
    RaceBlowMethodType method{};
    bool explode = false;
    bool touched = false;
    concptr act = "";
    AttributeType pt{};
    RaceBlowEffectType effect{};
    ARMOUR_CLASS ac = 0;
    DEPTH rlev = 0;
    bool blinked = false;
    bool do_silly_attack = false;
    int power = 0;
    bool obvious = false;
    Dice damage_dice{};
    bool known = false;
    bool fear = false;
    bool dead = false;
};

class PlayerType;
mam_type *initialize_mam_type(PlayerType *player_ptr, mam_type *mam_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx);
