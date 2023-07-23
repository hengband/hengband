#pragma once

#include "system/angband.h"

enum class MonsterRaceId : int16_t;
class PlayerType;
int lore_do_probe(PlayerType *player_ptr, MonsterRaceId r_idx);
void lore_treasure(PlayerType *player_ptr, MONSTER_IDX m_idx, ITEM_NUMBER num_item, ITEM_NUMBER num_gold);
