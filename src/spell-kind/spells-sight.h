#pragma once

#include "system/angband.h"

struct monster_race;
struct monster_type;
struct player_type;
bool project_all_los(player_type *player_ptr, EFFECT_ID typ, HIT_POINT dam);
bool speed_monsters(player_type *player_ptr);
bool slow_monsters(player_type *player_ptr, int power);
bool sleep_monsters(player_type *player_ptr, int power);
void aggravate_monsters(player_type *player_ptr, MONSTER_IDX who);
bool banish_evil(player_type *player_ptr, int dist);
bool turn_undead(player_type *player_ptr);
bool dispel_evil(player_type *player_ptr, HIT_POINT dam);
bool dispel_good(player_type *player_ptr, HIT_POINT dam);
bool dispel_undead(player_type *player_ptr, HIT_POINT dam);
bool dispel_monsters(player_type *player_ptr, HIT_POINT dam);
bool dispel_living(player_type *player_ptr, HIT_POINT dam);
bool dispel_demons(player_type *player_ptr, HIT_POINT dam);
bool crusade(player_type *player_ptr);
bool confuse_monsters(player_type *player_ptr, HIT_POINT dam);
bool charm_monsters(player_type *player_ptr, HIT_POINT dam);
bool charm_animals(player_type *player_ptr, HIT_POINT dam);
bool stun_monsters(player_type *player_ptr, HIT_POINT dam);
bool stasis_monsters(player_type *player_ptr, HIT_POINT dam);
bool mindblast_monsters(player_type *player_ptr, HIT_POINT dam);
bool banish_monsters(player_type *player_ptr, int dist);
bool turn_evil(player_type *player_ptr, HIT_POINT dam);
bool turn_monsters(player_type *player_ptr, HIT_POINT dam);
bool deathray_monsters(player_type *player_ptr);
void probed_monster_info(char *buf, player_type *player_ptr, monster_type *m_ptr, monster_race *r_ptr);
bool probing(player_type *player_ptr);
