#pragma once

extern void do_cmd_pref(player_type *creature_ptr);
extern void do_cmd_reload_autopick(player_type *creature_ptr);
extern void do_cmd_colors(player_type *creature_ptr);
extern void do_cmd_note(void);
extern void do_cmd_version(void);
extern void do_cmd_feeling(player_type *creature_ptr);
extern void do_cmd_knowledge(player_type *creature_ptr);
extern void do_cmd_checkquest(player_type *creature_ptr);
extern void do_cmd_time(player_type *creature_ptr);

/*
 * Forward declare
 */
extern void do_cmd_spoilers(player_type *creature_ptr);

/* wizard1.c */
extern void spoil_random_artifact(player_type *creature_ptr, concptr fname);
