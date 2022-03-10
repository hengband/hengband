#pragma once

enum class BlowEffectType {
    NONE = 0,
    FEAR = 1,
    SLEEP = 2,
    HEAL = 3,
};

struct mam_type;
class PlayerType;
void describe_melee_method(PlayerType *player_ptr, mam_type *mam_ptr);
void decide_monster_attack_effect(PlayerType *player_ptr, mam_type *mam_ptr);
void describe_monster_missed_monster(PlayerType *player_ptr, mam_type *mam_ptr);
