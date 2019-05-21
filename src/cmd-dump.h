#pragma once

#define NIKKI_HIGAWARI     0
#define NIKKI_BUNSHOU      1
#define NIKKI_ART          2
#define NIKKI_UNIQUE       3
#define NIKKI_FIX_QUEST_C  4
#define NIKKI_FIX_QUEST_F  5
#define NIKKI_RAND_QUEST_C 6
#define NIKKI_RAND_QUEST_F 7
#define NIKKI_MAXDEAPTH    8
#define NIKKI_TRUMP        9
#define NIKKI_STAIR       10
#define NIKKI_RECALL      11
#define NIKKI_TO_QUEST    12
#define NIKKI_TELE_LEV    13
#define NIKKI_BUY         14
#define NIKKI_SELL        15
#define NIKKI_ARENA       16
#define NIKKI_HANMEI      17
#define NIKKI_LEVELUP     18
#define NIKKI_GAMESTART   19
#define NIKKI_WIZ_TELE    20
#define NIKKI_NAMED_PET   21
#define NIKKI_PAT_TELE    22
#define NIKKI_ART_SCROLL  23

#define NIKKI_WIZARD_LOG  24



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

/* wizard1.c */
extern void spoil_random_artifact(concptr fname);

extern bool write_level;
