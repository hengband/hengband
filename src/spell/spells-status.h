#pragma once

#include "spell/spells-util.h"
#include "system/angband.h"

class ItemEntity;
class PlayerType;
class MonsterEntity;
bool heal_monster(PlayerType *player_ptr, DIRECTION dir, int dam);
bool speed_monster(PlayerType *player_ptr, DIRECTION dir, int power);
bool slow_monster(PlayerType *player_ptr, DIRECTION dir, int power);
bool sleep_monster(PlayerType *player_ptr, DIRECTION dir, int power);
bool stasis_monster(PlayerType *player_ptr, DIRECTION dir); /* Like sleep, affects undead as well */
bool stasis_evil(PlayerType *player_ptr, DIRECTION dir); /* Like sleep, affects undead as well */
bool confuse_monster(PlayerType *player_ptr, DIRECTION dir, PLAYER_LEVEL plev);
bool stun_monster(PlayerType *player_ptr, DIRECTION dir, PLAYER_LEVEL plev);
bool fear_monster(PlayerType *player_ptr, DIRECTION dir, PLAYER_LEVEL plev);
bool poly_monster(PlayerType *player_ptr, DIRECTION dir, int power);
bool clone_monster(PlayerType *player_ptr, DIRECTION dir);
bool time_walk(PlayerType *player_ptr);
void roll_hitdice(PlayerType *player_ptr, spell_operation options);
bool life_stream(PlayerType *player_ptr, bool message, bool virtue_change);
bool heroism(PlayerType *player_ptr, int base);
bool berserk(PlayerType *player_ptr, int base);
bool cure_light_wounds(PlayerType *player_ptr, DICE_NUMBER dice, DICE_SID sides);
bool cure_serious_wounds(PlayerType *player_ptr, DICE_NUMBER dice, DICE_SID sides);
bool cure_critical_wounds(PlayerType *player_ptr, int pow);
bool true_healing(PlayerType *player_ptr, int pow);
bool restore_mana(PlayerType *player_ptr, bool magic_eater);
bool restore_all_status(PlayerType *player_ptr);

bool fishing(PlayerType *player_ptr);
bool cosmic_cast_off(PlayerType *player_ptr, ItemEntity **o_ptr_ptr);
void apply_nexus(MonsterEntity *m_ptr, PlayerType *player_ptr);
void status_shuffle(PlayerType *player_ptr);
