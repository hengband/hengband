#pragma once

#include "effect/attribute-types.h"
#include "system/angband.h"

class MonsterRaceInfo;
class MonsterEntity;
class PlayerType;
bool project_all_los(PlayerType *player_ptr, AttributeType typ, int dam);
bool speed_monsters(PlayerType *player_ptr);
bool slow_monsters(PlayerType *player_ptr, int power);
bool sleep_monsters(PlayerType *player_ptr, int power);
void aggravate_monsters(PlayerType *player_ptr, MONSTER_IDX who);
bool banish_evil(PlayerType *player_ptr, int dist);
bool turn_undead(PlayerType *player_ptr);
bool dispel_evil(PlayerType *player_ptr, int dam);
bool dispel_good(PlayerType *player_ptr, int dam);
bool dispel_undead(PlayerType *player_ptr, int dam);
bool dispel_monsters(PlayerType *player_ptr, int dam);
bool dispel_living(PlayerType *player_ptr, int dam);
bool dispel_demons(PlayerType *player_ptr, int dam);
bool crusade(PlayerType *player_ptr);
bool confuse_monsters(PlayerType *player_ptr, int dam);
bool charm_monsters(PlayerType *player_ptr, int dam);
bool charm_animals(PlayerType *player_ptr, int dam);
bool stun_monsters(PlayerType *player_ptr, int dam);
bool stasis_monsters(PlayerType *player_ptr, int dam);
bool mindblast_monsters(PlayerType *player_ptr, int dam);
bool banish_monsters(PlayerType *player_ptr, int dist);
bool turn_evil(PlayerType *player_ptr, int dam);
bool turn_monsters(PlayerType *player_ptr, int dam);
bool deathray_monsters(PlayerType *player_ptr);
void probed_monster_info(char *buf, PlayerType *player_ptr, MonsterEntity *m_ptr, MonsterRaceInfo *r_ptr);
bool probing(PlayerType *player_ptr);
