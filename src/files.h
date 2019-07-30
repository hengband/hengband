#pragma once

#define TOKENIZE_CHECKQUOTE 0x01  /* Special handling of single quotes */

extern char savefile[1024];
extern char savefile_base[40];

extern concptr ANGBAND_DIR;
extern concptr ANGBAND_DIR_APEX;
extern concptr ANGBAND_DIR_BONE;
extern concptr ANGBAND_DIR_DATA;
extern concptr ANGBAND_DIR_EDIT;
extern concptr ANGBAND_DIR_SCRIPT;
extern concptr ANGBAND_DIR_FILE;
extern concptr ANGBAND_DIR_HELP;
extern concptr ANGBAND_DIR_INFO;
extern concptr ANGBAND_DIR_PREF;
extern concptr ANGBAND_DIR_SAVE;
extern concptr ANGBAND_DIR_USER;
extern concptr ANGBAND_DIR_XTRA;

extern s16b signal_count;

/* files.c */
extern void safe_setuid_drop(void);
extern void safe_setuid_grab(void);
extern s16b tokenize(char *buf, s16b num, char **tokens, BIT_FLAGS mode);
extern void display_player(player_type *creature_ptr, int mode);
extern errr make_character_dump(FILE *fff);
extern errr file_character(concptr name);
extern errr process_pref_file_command(char *buf);
extern concptr process_pref_file_expr(char **sp, char *fp);
extern errr process_pref_file(concptr name);
extern errr process_autopick_file(concptr name);
extern errr process_histpref_file(concptr name);
extern void print_equippy(void);
extern void display_player_equippy(TERM_LEN y, TERM_LEN x, BIT_FLAGS16 mode);
extern errr check_time_init(void);
extern errr check_load_init(void);
extern errr check_time(void);
extern errr check_load(void);
extern bool show_file(bool show_version, concptr name, concptr what, int line, BIT_FLAGS mode);
extern void do_cmd_help(void);
extern void process_player_name(bool sf);
extern void get_name(void);
extern void do_cmd_save_game(int is_autosave);
extern void do_cmd_save_and_exit(void);
extern void exit_game_panic(void);
extern void signals_ignore_tstp(void);
extern void signals_handle_tstp(void);
extern void signals_init(void);
extern errr get_rnd_line(concptr file_name, int entry, char *output);
extern void print_tomb(void);
extern void show_info(void);

#ifdef JP
extern errr get_rnd_line_jonly(concptr file_name, int entry, char *output, int count);
#endif
extern errr counts_write(int where, u32b count);
extern u32b counts_read(int where);
