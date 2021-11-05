﻿#pragma once

#include "system/angband.h"

struct monster_race;
struct monster_type;
class PlayerType;
bool project_all_los(PlayerType *player_ptr, EFFECT_ID typ, HIT_POINT dam);
bool speed_monsters(PlayerType *player_ptr);
bool slow_monsters(PlayerType *player_ptr, int power);
bool sleep_monsters(PlayerType *player_ptr, int power);
void aggravate_monsters(PlayerType *player_ptr, MONSTER_IDX who);
bool banish_evil(PlayerType *player_ptr, int dist);
bool turn_undead(PlayerType *player_ptr);
bool dispel_evil(PlayerType *player_ptr, HIT_POINT dam);
bool dispel_good(PlayerType *player_ptr, HIT_POINT dam);
bool dispel_undead(PlayerType *player_ptr, HIT_POINT dam);
bool dispel_monsters(PlayerType *player_ptr, HIT_POINT dam);
bool dispel_living(PlayerType *player_ptr, HIT_POINT dam);
bool dispel_demons(PlayerType *player_ptr, HIT_POINT dam);
bool crusade(PlayerType *player_ptr);
bool confuse_monsters(PlayerType *player_ptr, HIT_POINT dam);
bool charm_monsters(PlayerType *player_ptr, HIT_POINT dam);
bool charm_animals(PlayerType *player_ptr, HIT_POINT dam);
bool stun_monsters(PlayerType *player_ptr, HIT_POINT dam);
bool stasis_monsters(PlayerType *player_ptr, HIT_POINT dam);
bool mindblast_monsters(PlayerType *player_ptr, HIT_POINT dam);
bool banish_monsters(PlayerType *player_ptr, int dist);
bool turn_evil(PlayerType *player_ptr, HIT_POINT dam);
bool turn_monsters(PlayerType *player_ptr, HIT_POINT dam);
bool deathray_monsters(PlayerType *player_ptr);
void probed_monster_info(char *buf, PlayerType *player_ptr, monster_type *m_ptr, monster_race *r_ptr);
bool probing(PlayerType *player_ptr);
