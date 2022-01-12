#pragma once

#include "system/angband.h"
#include "effect/attribute-types.h"

#define SPELL_MAX 5

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
        bool (*spell_function)(PlayerType *, HIT_POINT);
    } spell3;

    struct debug_spell_type4 { // 実質 ty curse
        bool (*spell_function)(PlayerType *, bool, int*);
    } spell4;

    struct debug_spell_type5 {
        void (*spell_function)(PlayerType *);
    } spell5;

} spell_functions;

typedef struct debug_spell_command {
    int type;
    concptr command_name;
    spell_functions command_function;
} debug_spell_command;

extern debug_spell_command debug_spell_commands_list[SPELL_MAX];

bool wiz_debug_spell(PlayerType *player_ptr);
void wiz_dimension_door(PlayerType *player_ptr);
void wiz_summon_horde(PlayerType *player_ptr);
void wiz_teleport_back(PlayerType *player_ptr);
void wiz_learn_blue_magic_all(PlayerType *player_ptr);
void wiz_fillup_all_smith_essences(PlayerType *player_ptr);
void wiz_summon_random_enemy(PlayerType *player_ptr, int num);
void wiz_summon_specific_enemy(PlayerType *player_ptr, MONRACE_IDX r_idx);
void wiz_summon_pet(PlayerType *player_ptr, MONRACE_IDX r_idx);
void wiz_kill_enemy(PlayerType *player_ptr, HIT_POINT dam = 1000000, AttributeType effect_idx = AttributeType::DEBUG);
void wiz_kill_me(PlayerType *player_ptr, HIT_POINT dam, AttributeType effect_idx);
