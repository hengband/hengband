#pragma once

extern void do_cmd_diary(player_type *creature_ptr);
extern void do_cmd_redraw(player_type *creature_ptr);
extern void do_cmd_player_status(player_type *creature_ptr);
extern void do_cmd_message_one(void);
extern void do_cmd_messages(int num_now);
extern void do_cmd_pref(player_type *creature_ptr);
extern void do_cmd_reload_autopick(player_type *creature_ptr);
extern void do_cmd_visuals(player_type *creature_ptr);
extern void do_cmd_colors(player_type *creature_ptr);
extern void do_cmd_note(void);
extern void do_cmd_version(void);
extern void do_cmd_feeling(player_type *creature_ptr);
extern void do_cmd_load_screen(void);
extern void do_cmd_save_screen_html_aux(char *filename, int message);
extern void do_cmd_save_screen(player_type *creature_ptr);
extern void do_cmd_knowledge_quests_completed(player_type *creature_ptr, FILE *fff, QUEST_IDX quest_num[]);
extern void do_cmd_knowledge_quests_failed(player_type *creature_ptr, FILE *fff, QUEST_IDX quest_num[]);
extern void do_cmd_knowledge(player_type *creature_ptr);
extern void do_cmd_checkquest(player_type *creature_ptr);
extern void do_cmd_time(player_type *creature_ptr);

/*
 * Forward declare
 */
extern void do_cmd_spoilers(player_type *creature_ptr);

/* wizard1.c */
extern void spoil_random_artifact(player_type *creature_ptr, concptr fname);
