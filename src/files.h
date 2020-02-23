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

extern s16b tokenize(char *buf, s16b num, char **tokens, BIT_FLAGS mode);
extern void display_player(player_type *creature_ptr, int mode);
extern errr make_character_dump(player_type *creature_ptr, FILE *fff);
extern errr file_character(player_type *creature_ptr, concptr name);
extern errr process_pref_file_command(player_type *creature_ptr, char *buf);
extern concptr process_pref_file_expr(player_type *creature_ptr, char **sp, char *fp);
extern errr process_pref_file(player_type *creature_ptr, concptr name);
extern errr process_autopick_file(player_type *creature_ptr, concptr name);
extern errr process_histpref_file(player_type *creature_ptr, concptr name);
extern void display_player_equippy(player_type *player_ptr, TERM_LEN y, TERM_LEN x, BIT_FLAGS16 mode);
extern bool show_file(player_type *player_ptr, bool show_version, concptr name, concptr what, int line, BIT_FLAGS mode);
extern void do_cmd_help(player_type *creature_ptr);
extern void process_player_name(player_type *creature_ptr, bool sf);
extern void get_name(player_type *creature_ptr);
extern void do_cmd_save_game(player_type *creature_ptr, int is_autosave);
extern void do_cmd_save_and_exit(player_type *player_ptr);
extern void exit_game_panic(player_type *creature_ptr);
extern errr get_rnd_line(concptr file_name, int entry, char *output);
extern void print_tomb(player_type *dead_ptr);
extern void show_info(player_type *creature_ptr);

#ifdef JP
extern errr get_rnd_line_jonly(concptr file_name, int entry, char *output, int count);
#endif
extern errr counts_write(player_type *creature_ptr, int where, u32b count);
extern u32b counts_read(player_type *creature_ptr, int where);
