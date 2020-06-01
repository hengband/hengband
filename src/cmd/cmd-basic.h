#pragma once

#include "system/angband.h"

bool cmd_limit_cast(player_type *creature_ptr);
bool cmd_limit_arena(player_type *creature_ptr);
bool cmd_limit_time_walk(player_type *creature_ptr);
bool cmd_limit_blind(player_type *creature_ptr);
bool cmd_limit_confused(player_type *creature_ptr);
bool cmd_limit_image(player_type *creature_ptr);
bool cmd_limit_stun(player_type *creature_ptr);
void forget_travel_flow(floor_type *floor_ptr);
void do_cmd_go_up(player_type *creature_ptr);
void do_cmd_go_down(player_type *creature_ptr);
void do_cmd_search(player_type *creature_ptr);
void do_cmd_open(player_type *creature_ptr);
void do_cmd_close(player_type *creature_ptr);
void do_cmd_tunnel(player_type *creature_ptr);
void do_cmd_disarm(player_type *creature_ptr);
void do_cmd_bash(player_type *creature_ptr);
void do_cmd_alter(player_type *creature_ptr);
void do_cmd_spike(player_type *creature_ptr);
void do_cmd_walk(player_type *creature_ptr, bool pickup);
void do_cmd_stay(player_type *creature_ptr, bool pickup);
void do_cmd_run(player_type *creature_ptr);
void do_cmd_rest(player_type *creature_ptr);
void do_cmd_fire(player_type *creature_ptr, SPELL_IDX snipe_type);
void exe_fire(player_type *shooter_ptr, INVENTORY_IDX item, object_type *j_ptr, SPELL_IDX snipe_type);
void do_cmd_suicide(player_type *creature_ptr);
bool do_cmd_throw(player_type *creature_ptr, int mult, bool boomerang, OBJECT_IDX shuriken);
bool easy_open_door(player_type *creature_ptr, POSITION y, POSITION x);
bool exe_disarm(player_type *creature_ptr, POSITION y, POSITION x, DIRECTION dir);

void verify_equip_slot(player_type *owner_ptr, INVENTORY_IDX item);
