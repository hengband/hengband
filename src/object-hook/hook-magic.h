#pragma once

typedef struct object_type object_type;
typedef struct player_type player_type;
bool object_is_activatable(const object_type *o_ptr);
bool item_tester_hook_use(player_type *player_ptr, const object_type *o_ptr);
bool object_is_rechargeable(const object_type *o_ptr);
bool item_tester_learn_spell(player_type *player_ptr, const object_type *o_ptr);
bool item_tester_high_level_book(const object_type *o_ptr);
