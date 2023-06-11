#pragma once

class ItemEntity;
class PlayerType;
bool item_tester_hook_eatable(PlayerType *player_ptr, const ItemEntity *o_ptr);
bool item_tester_hook_quaff(PlayerType *player_ptr, const ItemEntity *o_ptr);
bool can_player_destroy_object(ItemEntity *o_ptr);
