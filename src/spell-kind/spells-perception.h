#pragma once

#include "system/angband.h"

class ItemEntity;
class PlayerType;
void identify_pack(PlayerType *player_ptr);
bool identify_item(PlayerType *player_ptr, ItemEntity *o_ptr);
bool ident_spell(PlayerType *player_ptr, bool only_equip);
bool identify_fully(PlayerType *player_ptr, bool only_equip);
