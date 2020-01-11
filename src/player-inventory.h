#pragma once

extern bool select_ring_slot;

extern bool is_ring_slot(int i);
extern bool get_item_okay(OBJECT_IDX i);
extern concptr describe_use(int i);

extern void display_inven(player_type *creature_ptr, OBJECT_TYPE_VALUE tval);
extern void display_equip(player_type *creature_ptr, OBJECT_TYPE_VALUE tval);
extern COMMAND_CODE show_inven(player_type *owner_ptr, int target_item, BIT_FLAGS mode, OBJECT_TYPE_VALUE tval);
extern COMMAND_CODE show_equip(player_type *owner_ptr, int target_item, BIT_FLAGS mode, OBJECT_TYPE_VALUE tval);
extern void toggle_inven_equip(player_type *owner_ptr);

extern object_type *choose_object(player_type *owner_ptr, OBJECT_IDX *idx, concptr q, concptr s, BIT_FLAGS option, OBJECT_TYPE_VALUE tval);
