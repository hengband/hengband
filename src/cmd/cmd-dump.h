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

#define RECORD_NAMED_PET_NAME        0
#define RECORD_NAMED_PET_UNNAME      1
#define RECORD_NAMED_PET_DISMISS     2
#define RECORD_NAMED_PET_DEATH       3
#define RECORD_NAMED_PET_MOVED       4
#define RECORD_NAMED_PET_LOST_SIGHT  5
#define RECORD_NAMED_PET_DESTROY     6
#define RECORD_NAMED_PET_EARTHQUAKE  7
#define RECORD_NAMED_PET_GENOCIDE    8
#define RECORD_NAMED_PET_WIZ_ZAP     9
#define RECORD_NAMED_PET_TELE_LEVEL  10
#define RECORD_NAMED_PET_BLAST       11
#define RECORD_NAMED_PET_HEAL_LEPER  12
#define RECORD_NAMED_PET_COMPACT     13
#define RECORD_NAMED_PET_LOSE_PARENT 14


/* cmd4.c */
#ifndef JP
extern concptr get_ordinal_number_suffix(int num);
#endif
extern errr exe_write_diary(player_type *creature_ptr, int type, int num, concptr note);
extern void do_cmd_nikki(player_type *creature_ptr);
extern void do_cmd_redraw(player_type *creature_ptr);
extern void do_cmd_player_status(player_type *creature_ptr);
extern void do_cmd_message_one(void);
extern void do_cmd_messages(int num_now);
extern void do_cmd_pref(void);
extern void do_cmd_reload_autopick(void);
extern void do_cmd_macros(player_type *creature_ptr);
extern void do_cmd_visuals(player_type *creature_ptr);
extern void do_cmd_colors(player_type *creature_ptr);
extern void do_cmd_note(void);
extern void do_cmd_version(void);
extern void do_cmd_feeling(player_type *creature_ptr);
extern void do_cmd_load_screen(void);
extern void do_cmd_save_screen_html_aux(char *filename, int message);
extern void do_cmd_save_screen(player_type *creature_ptr);
extern void do_cmd_knowledge_quests_completed(FILE *fff, QUEST_IDX quest_num[]);
extern void do_cmd_knowledge_quests_failed(FILE *fff, QUEST_IDX quest_num[]);
extern void do_cmd_knowledge(player_type *creature_ptr);
extern void do_cmd_checkquest(player_type *creature_ptr);
extern void do_cmd_time(void);

/*
 * Forward declare
 */
extern void do_cmd_spoilers(void);

/* wizard1.c */
extern void spoil_random_artifact(player_type *creature_ptr, concptr fname);

extern bool write_level;
