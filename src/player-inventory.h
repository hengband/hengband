#pragma once

extern bool select_ring_slot;

extern bool is_ring_slot(int i);
extern bool get_item_okay(OBJECT_IDX i);
extern concptr describe_use(int i);

extern void display_inven(OBJECT_TYPE_VALUE tval);
extern void display_equip(OBJECT_TYPE_VALUE tval);
extern COMMAND_CODE show_inven(int target_item, BIT_FLAGS mode, OBJECT_TYPE_VALUE tval);
extern COMMAND_CODE show_equip(int target_item, BIT_FLAGS mode, OBJECT_TYPE_VALUE tval);
