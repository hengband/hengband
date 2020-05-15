#pragma once

extern bool select_ring_slot;

extern bool is_ring_slot(int i);
extern concptr describe_use(player_type *owner_ptr, int i);

extern void display_inventory(player_type *creature_ptr, OBJECT_TYPE_VALUE tval);
extern void display_equipment(player_type *creature_ptr, OBJECT_TYPE_VALUE tval);
extern COMMAND_CODE show_inventory(player_type *owner_ptr, int target_item, BIT_FLAGS mode, OBJECT_TYPE_VALUE tval);
extern COMMAND_CODE show_equipment(player_type *owner_ptr, int target_item, BIT_FLAGS mode, OBJECT_TYPE_VALUE tval);
extern void toggle_inventory_equipment(player_type *owner_ptr);

extern object_type *choose_object(player_type *owner_ptr, OBJECT_IDX *idx, concptr q, concptr s, BIT_FLAGS option, OBJECT_TYPE_VALUE tval);
extern bool can_get_item(player_type *owner_ptr, OBJECT_TYPE_VALUE tval);
extern bool get_item(player_type *owner_ptr, OBJECT_IDX *cp, concptr pmt, concptr str, BIT_FLAGS mode, OBJECT_TYPE_VALUE tval);
extern ITEM_NUMBER scan_floor(player_type *owner_ptr, OBJECT_IDX *items, POSITION y, POSITION x, BIT_FLAGS mode, OBJECT_TYPE_VALUE item_tester_tval);
extern COMMAND_CODE show_floor(player_type *owner_ptr, int target_item, POSITION y, POSITION x, TERM_LEN *min_width, OBJECT_TYPE_VALUE item_tester_tval);
extern bool get_item_floor(player_type *creature_ptr, COMMAND_CODE *cp, concptr pmt, concptr str, BIT_FLAGS mode, OBJECT_TYPE_VALUE tval);
extern void py_pickup_floor(player_type *creature_ptr, bool pickup);
