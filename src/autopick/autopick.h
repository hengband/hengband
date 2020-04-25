#pragma once

#include "autopick/autopick-util.h"

#define DO_AUTOPICK       0x01
#define DO_AUTODESTROY    0x02
#define DO_DISPLAY        0x04
#define DONT_AUTOPICK     0x08
#define ITEM_DISPLAY      0x10
#define DO_QUERY_AUTOPICK 0x20

/*
 *  List for auto-picker/destroyer entries
 */
extern int max_autopick;
extern int max_max_autopick;
extern autopick_type *autopick_list;

extern void autopick_load_pref(player_type *player_ptr, bool disp_mes);
extern void process_autopick_file_command(char *buf);
extern concptr autopick_line_from_entry(autopick_type *entry);
extern int is_autopick(player_type *player_ptr, object_type *o_ptr);
extern void autopick_alter_item(player_type *player_ptr, INVENTORY_IDX item, bool destroy);
extern void autopick_delayed_alter(player_type *player_ptr);
extern void autopick_pickup_items(player_type *player_ptr, grid_type *g_ptr);
extern bool autopick_autoregister(player_type *player_ptr, object_type *o_ptr);
extern void do_cmd_edit_autopick(player_type *player_ptr);
