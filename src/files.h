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
extern errr file_character(player_type *creature_ptr, concptr name);
extern errr process_pref_file_command(player_type *creature_ptr, char *buf);
extern concptr process_pref_file_expr(player_type *creature_ptr, char **sp, char *fp);
extern errr process_pref_file(player_type *creature_ptr, concptr name);
extern errr process_autopick_file(player_type *creature_ptr, concptr name);
extern errr process_histpref_file(player_type *creature_ptr, concptr name);
extern bool show_file(player_type *player_ptr, bool show_version, concptr name, concptr what, int line, BIT_FLAGS mode);
extern void do_cmd_help(player_type *creature_ptr);
extern void process_player_name(player_type *creature_ptr, bool sf);
extern void get_name(player_type *creature_ptr);
extern void do_cmd_save_game(player_type *creature_ptr, int is_autosave);
extern void do_cmd_save_and_exit(player_type *player_ptr);
extern void exit_game_panic(player_type *creature_ptr);
extern errr get_rnd_line(concptr file_name, int entry, char *output);
void read_dead_file(char* buf, size_t buf_size);

#ifdef JP
extern errr get_rnd_line_jonly(concptr file_name, int entry, char *output, int count);
#endif
extern errr counts_write(player_type *creature_ptr, int where, u32b count);
extern u32b counts_read(player_type *creature_ptr, int where);
