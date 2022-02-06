#pragma once

#include "system/angband.h"

class MonsterAttackPlayer;
class PlayerType;
void process_eat_gold(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr);
bool check_eat_item(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr);
void process_eat_item(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr);
void process_eat_food(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr);
void process_eat_lite(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr);

bool process_un_power(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr);
bool check_drain_hp(PlayerType *player_ptr, const int32_t d);
void process_drain_life(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr, const bool resist_drain);
void process_drain_mana(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr);
void process_monster_attack_hungry(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr);
