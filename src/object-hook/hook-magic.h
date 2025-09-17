#pragma once

class ItemEntity;
class PlayerType;
bool item_tester_hook_use(PlayerType *player_ptr, const ItemEntity &item);
bool item_tester_learn_spell(PlayerType *player_ptr, const ItemEntity &item);
