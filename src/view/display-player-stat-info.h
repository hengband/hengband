#pragma once

#include "view/display-symbol.h"

class ItemEntity;
class PlayerType;
DisplaySymbol get_equipment_stat_symbol(const ItemEntity &item, int stat);
DisplaySymbol get_intrinsic_stat_symbol(PlayerType *player_ptr, int stat);
void display_player_stat_info(PlayerType *player_ptr);
