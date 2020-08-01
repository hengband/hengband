#pragma once

#include "system/angband.h"

#define SPELL_MAX 3

typedef union spell_functions {
    struct debug_spell_type1 {
        bool (*spell_function)(player_type *, floor_type *);
    } spell1;

    struct debug_spell_type2 {
        bool (*spell_function)(player_type *);
    } spell2;

    struct debug_spell_type3 {
        bool (*spell_function)(player_type *, HIT_POINT);
    } spell3;

} spell_functions;

typedef struct debug_spell_command {
    int type;
    char *command_name;
    spell_functions command_function;
} debug_spell_command;

extern debug_spell_command debug_spell_commands_list[SPELL_MAX];

bool wiz_debug_spell(player_type *creature_ptr);
void wiz_dimension_door(player_type *caster_ptr);
void wiz_summon_horde(player_type *caster_ptr);
void wiz_teleport_back(player_type *caster_ptr);
void wiz_learn_blue_magic_all(player_type *caster_ptr);
void wiz_summon_random_enemy(player_type *caster_ptr, int num);
void wiz_summon_specific_enemy(player_type *summoner_ptr, MONRACE_IDX r_idx);
void wiz_summon_pet(player_type *summoner_ptr, MONRACE_IDX r_idx);
