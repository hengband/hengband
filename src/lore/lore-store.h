#pragma once

#include "system/angband.h"

struct player_type;
int lore_do_probe(player_type *player_ptr, MONRACE_IDX r_idx);
void lore_treasure(player_type *player_ptr, MONSTER_IDX m_idx, ITEM_NUMBER num_item, ITEM_NUMBER num_gold);
