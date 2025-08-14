#pragma once

#include "effect/attribute-types.h"
#include "system/angband.h"

enum class MonraceId : short;

class FloorType;
class PlayerType;
typedef union spell_functions {
    struct debug_spell_type1 {
        bool (*spell_function)(PlayerType *, FloorType *);
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

void wiz_debug_spell(PlayerType *player_ptr);
void wiz_dimension_door(PlayerType *player_ptr);
void wiz_summon_horde(PlayerType *player_ptr);
void wiz_teleport_back(PlayerType *player_ptr);
void wiz_learn_blue_magic_all(PlayerType *player_ptr);
void wiz_fillup_all_smith_essences(PlayerType *player_ptr);
void wiz_generate_random_monster(PlayerType *player_ptr, int num);
void wiz_summon_random_monster(PlayerType *player_ptr, int num);
void wiz_summon_specific_monster(PlayerType *player_ptr, MonraceId monrace_id);
void wiz_summon_pet(PlayerType *player_ptr, MonraceId monrace_id);
void wiz_summon_clone(PlayerType *player_ptr, MonraceId monrace_id);
void wiz_kill_target(PlayerType *player_ptr, int initial_dam = 1000000, AttributeType effect_idx = AttributeType::DEBUG, const bool self = false);
