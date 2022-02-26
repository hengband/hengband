#pragma once

enum be_type {
    BLOW_EFFECT_TYPE_NONE = 0,
    BLOW_EFFECT_TYPE_FEAR = 1,
    BLOW_EFFECT_TYPE_SLEEP = 2,
    BLOW_EFFECT_TYPE_HEAL = 3,
};

struct mam_type;
class PlayerType;
void describe_melee_method(PlayerType *player_ptr, mam_type *mam_ptr);
void decide_monster_attack_effect(PlayerType *player_ptr, mam_type *mam_ptr);
void describe_monster_missed_monster(PlayerType *player_ptr, mam_type *mam_ptr);
