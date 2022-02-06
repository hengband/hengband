#pragma once

#include "system/angband.h"

class MonsterAttackPlayer;
class PlayerType;
void process_blind_attack(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr);
void process_terrify_attack(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr);
void process_paralyze_attack(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr);
void process_lose_all_attack(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr);
void process_stun_attack(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr);
void process_monster_attack_time(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr);
