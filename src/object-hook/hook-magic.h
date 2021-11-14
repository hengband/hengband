#pragma once

struct object_type;
class PlayerType;
bool item_tester_hook_use(PlayerType *player_ptr, const object_type *o_ptr);
bool item_tester_learn_spell(PlayerType *player_ptr, const object_type *o_ptr);
bool item_tester_high_level_book(const object_type *o_ptr);
