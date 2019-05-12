#pragma once

/* cmd4.c */
#ifndef JP
extern concptr get_ordinal_number_suffix(int num);
#endif
extern errr do_cmd_write_nikki(int type, int num, concptr note);
extern void do_cmd_nikki(void);
extern void do_cmd_redraw(void);
extern void do_cmd_change_name(void);
extern void do_cmd_message_one(void);
extern void do_cmd_messages(int num_now);
extern void do_cmd_options_aux(int page, concptr info);
extern void do_cmd_options(void);
extern void do_cmd_pref(void);
extern void do_cmd_reload_autopick(void);
extern void do_cmd_macros(void);
extern void do_cmd_visuals(void);
extern void do_cmd_colors(void);
extern void do_cmd_note(void);
extern void do_cmd_version(void);
extern void do_cmd_feeling(void);
extern void do_cmd_load_screen(void);
extern void do_cmd_save_screen_html_aux(char *filename, int message);
extern void do_cmd_save_screen(void);
extern void do_cmd_knowledge_quests_completed(FILE *fff, QUEST_IDX quest_num[]);
extern void do_cmd_knowledge_quests_failed(FILE *fff, QUEST_IDX quest_num[]);
extern void do_cmd_knowledge(void);
extern void plural_aux(char * Name);
extern void do_cmd_checkquest(void);
extern void do_cmd_time(void);
extern void do_cmd_suicide(void);

/*
 * Forward declare
 */
extern void do_cmd_spoilers(void);

extern bool write_level;
