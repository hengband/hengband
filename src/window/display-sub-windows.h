#pragma once

#include "system/angband.h"
#include "object/tval-types.h"

typedef struct pos_list pos_list;

void fix_inventory(player_type *player_ptr, tval_type item_tester_tval);
void print_monster_list(floor_type *floor_ptr, pos_list *plist, TERM_LEN x, TERM_LEN y, TERM_LEN max_lines);
void fix_monster_list(player_type *player_ptr);
void fix_equip(player_type *player_ptr, tval_type item_tester_tval);
void fix_player(player_type *player_ptr);
void fix_message(void);
void fix_overhead(player_type *player_ptr);
void fix_dungeon(player_type *player_ptr);
void fix_monster(player_type *player_ptr);
void fix_object(player_type *player_ptr);
void fix_floor_item_list(player_type *player_ptr, const int y, const int x);
void toggle_inventory_equipment(player_type *owner_ptr);
