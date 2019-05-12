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
extern void do_cmd_go_up(void);
extern void do_cmd_go_down(void);
extern void do_cmd_search(void);
extern void do_cmd_open(void);
extern void do_cmd_close(void);
extern void do_cmd_tunnel(void);
extern void do_cmd_disarm(void);
extern void do_cmd_bash(void);
extern void do_cmd_alter(void);
extern void do_cmd_spike(void);
extern void do_cmd_walk(bool pickup);
extern void do_cmd_stay(bool pickup);
extern void do_cmd_run(void);
extern void do_cmd_rest(void);
extern void do_cmd_fire(SPELL_IDX snipe_type);
extern void exe_fire(INVENTORY_IDX item, object_type *j_ptr, SPELL_IDX snipe_type);
extern bool do_cmd_throw(int mult, bool boomerang, OBJECT_IDX shuriken);
#ifdef TRAVEL
extern void do_cmd_travel(void);
#endif
extern bool easy_open_door(POSITION y, POSITION x);
extern bool do_cmd_disarm_aux(POSITION y, POSITION x, DIRECTION dir);

extern void kamaenaoshi(INVENTORY_IDX item);
