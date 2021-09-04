#pragma once

#include "system/angband.h"
#include "spell/spells-util.h"

struct object_type;;
struct player_type;
struct monster_type;
bool heal_monster(player_type *caster_ptr, DIRECTION dir, HIT_POINT dam);
bool speed_monster(player_type *caster_ptr, DIRECTION dir, int power);
bool slow_monster(player_type *caster_ptr, DIRECTION dir, int power);
bool sleep_monster(player_type *caster_ptr, DIRECTION dir, int power);
bool stasis_monster(player_type *caster_ptr, DIRECTION dir);    /* Like sleep, affects undead as well */
bool stasis_evil(player_type *caster_ptr, DIRECTION dir);    /* Like sleep, affects undead as well */
bool confuse_monster(player_type *caster_ptr, DIRECTION dir, PLAYER_LEVEL plev);
bool stun_monster(player_type *caster_ptr, DIRECTION dir, PLAYER_LEVEL plev);
bool fear_monster(player_type *caster_ptr, DIRECTION dir, PLAYER_LEVEL plev);
bool poly_monster(player_type *caster_ptr, DIRECTION dir, int power);
bool clone_monster(player_type *caster_ptr, DIRECTION dir);
bool time_walk(player_type *creature_ptr);
void roll_hitdice(player_type *creature_ptr, spell_operation options);
bool life_stream(player_type *creature_ptr, bool message, bool virtue_change);
bool heroism(player_type *creature_ptr, int base);
bool berserk(player_type *creature_ptr, int base);
bool cure_light_wounds(player_type *creature_ptr, DICE_NUMBER dice, DICE_SID sides);
bool cure_serious_wounds(player_type *creature_ptr, DICE_NUMBER dice, DICE_SID sides);
bool cure_critical_wounds(player_type *creature_ptr, HIT_POINT pow);
bool true_healing(player_type *creature_ptr, HIT_POINT pow);
bool restore_mana(player_type *creature_ptr, bool magic_eater);
bool restore_all_status(player_type *creature_ptr);

bool fishing(player_type *creature_ptr);
bool cosmic_cast_off(player_type *creature_ptr, object_type **o_ptr_ptr);
void apply_nexus(monster_type *m_ptr, player_type *target_ptr);
void status_shuffle(player_type *creature_ptr);
