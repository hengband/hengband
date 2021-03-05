﻿#pragma once

#include "system/angband.h"
#include "player-attack/player-attack-util.h"

bool kawarimi(player_type *caster_ptr, bool success);
bool rush_attack(player_type *attacker_ptr, bool *mdeath);
void process_surprise_attack(player_type *attacker_ptr, player_attack_type *pa_ptr);
void print_surprise_attack(player_attack_type *pa_ptr);
void calc_surprise_attack_damage(player_type *attacker_ptr, player_attack_type *pa_ptr);
bool hayagake(player_type *creature_ptr);
bool set_superstealth(player_type *creature_ptr, bool set);

enum mind_ninja_type : int;
bool cast_ninja_spell(player_type *caster_ptr, mind_ninja_type spell);
