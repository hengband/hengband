#pragma once

#include "system/angband.h"
#include "spell/spell-types.h"

enum class ElementRealm {
	FIRE = 1,
	ICE = 2,
	SKY = 3,
	SEA = 4,
	DARKNESS = 5,
	CHAOS = 6,
	EARTH = 7,
	DEATH = 8,
	MAX
};

struct player_type;
struct effect_monster_type;
struct rc_type;

concptr get_element_title(int realm_idx);
spells_type get_element_type(int realm_idx, int n);
concptr get_element_name(int realm_idx, int n);
void do_cmd_element(player_type *caster_ptr);
void do_cmd_element_browse(player_type *caster_ptr);
process_result effect_monster_elemental_genocide(player_type *caster_ptr, effect_monster_type *em_ptr);
bool has_element_resist(player_type *creature_ptr, ElementRealm realm, PLAYER_LEVEL lev);
byte select_element_realm(player_type *creature_ptr);
void switch_element_racial(player_type *creature_ptr, rc_type *rc_ptr);
bool switch_element_execution(player_type *creature_ptr);
