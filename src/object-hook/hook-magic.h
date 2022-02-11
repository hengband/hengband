#pragma once

class ObjectType;
class PlayerType;
bool item_tester_hook_use(PlayerType *player_ptr, const ObjectType *o_ptr);
bool item_tester_learn_spell(PlayerType *player_ptr, const ObjectType *o_ptr);
bool item_tester_high_level_book(const ObjectType *o_ptr);
