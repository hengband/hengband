#pragma once

#include "effect/attribute-types.h"
#include "system/angband.h"

enum class MonsterRaceId : int16_t;

struct floor_type;
class PlayerType;
typedef union spell_functions {
    struct debug_spell_type1 {
        bool (*spell_function)(PlayerType *, floor_type *);
    } spell1;

    struct debug_spell_type2 {
        bool (*spell_function)(PlayerType *);
    } spell2;

    struct debug_spell_type3 {
        bool (*spell_function)(PlayerType *, int);
    } spell3;

    struct debug_spell_type4 { // 実質 ty curse
        bool (*spell_function)(PlayerType *, bool, int *);
    } spell4;

    struct debug_spell_type5 {
        void (*spell_function)(PlayerType *);
    } spell5;

} spell_functions;

struct debug_spell_command {
    int type;
    concptr command_name;
    spell_functions command_function;
};

bool wiz_debug_spell(PlayerType *player_ptr);
void wiz_dimension_door(PlayerType *player_ptr);
void wiz_summon_horde(PlayerType *player_ptr);
void wiz_teleport_back(PlayerType *player_ptr);
void wiz_learn_blue_magic_all(PlayerType *player_ptr);
void wiz_fillup_all_smith_essences(PlayerType *player_ptr);
void wiz_summon_random_enemy(PlayerType *player_ptr, int num);
void wiz_summon_specific_enemy(PlayerType *player_ptr, MonsterRaceId r_idx);
void wiz_summon_pet(PlayerType *player_ptr, MonsterRaceId r_idx);
void wiz_kill_target(PlayerType *player_ptr, int dam = 1000000, AttributeType effect_idx = AttributeType::DEBUG, const bool self = false);
