#pragma once

class MonsterAttackPlayer;
class PlayerType;
void calc_blow_disease(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr);
void calc_blow_lose_strength(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr);
void calc_blow_lose_intelligence(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr);
void calc_blow_lose_wisdom(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr);
void calc_blow_lose_dexterity(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr);
void calc_blow_lose_constitution(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr);
void calc_blow_lose_charisma(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr);
void calc_blow_lose_all(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr);
