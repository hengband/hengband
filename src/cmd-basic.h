#pragma once

/* cmd2.c */
extern bool cmd_limit_cast(player_type *creature_ptr);
extern bool cmd_limit_arena(player_type *creature_ptr);
extern bool cmd_limit_time_walk(player_type *creature_ptr);
extern bool cmd_limit_blind(player_type *creature_ptr);
extern bool cmd_limit_confused(player_type *creature_ptr);
extern bool cmd_limit_image(player_type *creature_ptr);
extern bool cmd_limit_stun(player_type *creature_ptr);
extern void forget_travel_flow(void);
extern void do_cmd_go_up(player_type *creature_ptr);
extern void do_cmd_go_down(player_type *creature_ptr);
extern void do_cmd_search(player_type *creature_ptr);
extern void do_cmd_open(player_type *creature_ptr);
extern void do_cmd_close(player_type *creature_ptr);
extern void do_cmd_tunnel(player_type *creature_ptr);
extern void do_cmd_disarm(player_type *creature_ptr);
extern void do_cmd_bash(player_type *creature_ptr);
extern void do_cmd_alter(void);
extern void do_cmd_spike(player_type *creature_ptr);
extern void do_cmd_walk(player_type *creature_ptr, bool pickup);
extern void do_cmd_stay(bool pickup);
extern void do_cmd_run(player_type *creature_ptr);
extern void do_cmd_rest(player_type *creature_ptr);
extern void do_cmd_fire(player_type *creature_ptr, SPELL_IDX snipe_type);
extern void exe_fire(INVENTORY_IDX item, object_type *j_ptr, SPELL_IDX snipe_type);
extern bool do_cmd_throw(int mult, bool boomerang, OBJECT_IDX shuriken);
extern bool easy_open_door(player_type *creature_ptr, POSITION y, POSITION x);
extern bool do_cmd_disarm_aux(POSITION y, POSITION x, DIRECTION dir);

extern void kamaenaoshi(INVENTORY_IDX item);
