/*!
 * @file externs.h
 * @brief Angband(変愚蛮怒)基本関数、グローバル変数ヘッダファイル / 
 * extern declarations (variables and functions)
 * @date 2014/08/08
 * @author
 * Copyright (c) 1997 Ben Harrison
 * @details
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.
 * Note that some files have their own header files
 * (z-virt.h, z-util.h, z-form.h, term.h, random.h)
 */
#include "geometry.h"
#include "grid.h"
#include "floor-save.h"
#include "monster.h"
#include "monsterrace.h"
#include "object.h"
#include "player-status.h"

/*
 * Automatically generated "variable" declarations
 */
extern int max_macrotrigger;
extern concptr macro_template;
extern concptr macro_modifier_chr;
extern concptr macro_modifier_name[MAX_MACRO_MOD];
extern concptr macro_trigger_name[MAX_MACRO_TRIG];
extern concptr macro_trigger_keycode[2][MAX_MACRO_TRIG];

extern int level_up;


/* tables.c */

extern const char hexsym[16];
extern const char listsym[];
extern const concptr color_char;
extern const byte adj_mag_study[];
extern const byte adj_mag_mana[];
extern const byte adj_mag_fail[];
extern const byte adj_mag_stat[];
extern const byte adj_chr_gold[];
extern const byte adj_int_dev[];
extern const byte adj_wis_sav[];
extern const byte adj_dex_dis[];
extern const byte adj_int_dis[];
extern const byte adj_dex_ta[];
extern const byte adj_str_td[];
extern const byte adj_dex_th[];
extern const byte adj_str_th[];
extern const byte adj_str_wgt[];
extern const byte adj_str_hold[];
extern const byte adj_str_dig[];
extern const byte adj_dex_safe[];
extern const byte adj_con_fix[];
extern const byte adj_con_mhp[];
extern const byte adj_chr_chm[];
extern const byte extract_energy[200];


extern const concptr spell_names[VALID_REALM][32];
extern const concptr color_names[16];
extern const concptr stat_names[6];
extern const concptr stat_names_reduced[6];
extern const concptr window_flag_desc[32];

extern const concptr game_inscriptions[];

extern const concptr ident_info[];
extern const byte feature_action_flags[FF_FLAG_MAX];

/* variable.c */


extern bool character_generated;
extern bool character_dungeon;
extern bool character_loaded;
extern bool character_saved;
extern bool character_icky;
extern bool character_xtra;
extern bool creating_savefile;
extern COMMAND_CODE command_cmd;
extern COMMAND_ARG command_arg;
extern s16b command_rep;
extern DIRECTION command_dir;
extern s16b command_see;
extern TERM_LEN command_gap;
extern s16b command_wrk;
extern s16b command_new;
extern bool msg_flag;
extern s16b running;

extern GAME_TURN old_battle;

extern s16b signal_count;
extern bool inkey_base;
extern bool inkey_xtra;
extern bool inkey_scan;
extern bool inkey_flag;
extern bool get_com_no_macros;
extern OBJECT_SUBTYPE_VALUE coin_type;

extern bool repair_monsters;
extern bool repair_objects;
extern MONSTER_IDX hack_m_idx;
extern MONSTER_IDX hack_m_idx_ii;
extern int total_friends;
extern int leaving_quest;
extern bool reinit_wilderness;
extern bool multi_rew;
extern bool reset_concent;
extern bool is_fired;



/*
 * Software options (set via the '=' command).  See "tables.c"
 */


extern GAME_TURN record_turn;
extern POSITION panel_row_min, panel_row_max;
extern POSITION panel_col_min, panel_col_max;
extern POSITION panel_col_prt, panel_row_prt;
extern MONSTER_IDX target_who;
extern POSITION target_col;
extern POSITION target_row;
extern int player_uid;
extern int player_euid;
extern int player_egid;
extern char player_base[32];
extern char savefile[1024];
extern char savefile_base[40];
extern pos_list tmp_pos;
extern STR_OFFSET quark__num;
extern concptr *quark__str;

extern term *angband_term[8];
extern const char angband_term_name[8][16];
extern byte angband_color_table[256][4];
extern const concptr angband_sound_name[SOUND_MAX];
extern const concptr angband_music_basic_name[MUSIC_BASIC_MAX];
extern FLOOR_IDX max_floor_id;
extern u32b saved_floor_file_sign;
extern concptr keymap_act[KEYMAP_MODES][256];
extern player_type *p_ptr;
extern char *f_name;
extern char *f_tag;

extern OBJECT_TYPE_VALUE item_tester_tval;
extern bool (*item_tester_hook)(object_type *o_ptr);
extern monsterrace_hook_type get_mon_num_hook;
extern monsterrace_hook_type get_mon_num2_hook;
extern bool (*get_obj_num_hook)(KIND_OBJECT_IDX k_idx);


extern VAULT_IDX max_v_idx;

extern EGO_IDX max_e_idx;
extern int init_flags;
extern int highscore_fd;
extern bool can_save;

extern MONSTER_IDX pet_t_m_idx;
extern MONSTER_IDX riding_t_m_idx;
extern MONRACE_IDX today_mon;
extern bool write_level;
extern u32b start_time;
extern bool sukekaku;
extern bool new_mane;
extern bool generate_encounter;
extern concptr screen_dump;

extern DEPTH *max_dlv;
extern COMMAND_CODE now_message;
extern bool use_menu;

/* xtra1.c */
extern void handle_stuff(void);
extern void update_output(void);
extern void print_monster_list(TERM_LEN x, TERM_LEN y, TERM_LEN max_lines);
extern void update_playtime(void);

#ifdef MACINTOSH
/* main-mac.c */
/* extern void main(void); */
#endif

#if defined(MAC_MPW) || defined(MACH_O_CARBON)
/* Globals needed */
extern  u32b _ftype;
extern  u32b _fcreator;
#endif

#if defined(MAC_MPW) && defined(CARBON)
extern void convert_pathname(char *path);
#endif

#if defined(MACH_O_CARBON)
extern void fsetfileinfo(concptr path, u32b fcreator, u32b ftype);
#endif

#ifdef WINDOWS
/* main-win.c */
/* extern int FAR PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, ...); */
#endif

/* wizard1.c */
extern void spoil_random_artifact(concptr fname);


#ifdef JP
/* japanese.c */
extern void sindarin_to_kana(char *kana, concptr sindarin);
extern void jverb( concptr in , char *out , int flag);
extern void sjis2euc(char *str);
extern void euc2sjis(char *str);
extern byte codeconv(char *str);
extern bool iskanji2(concptr s, int x);
extern void guess_convert_to_system_encoding(char* strbuf, int buflen);
#endif

/* inet.c */
extern int soc_write(int sd, char *buf, size_t sz);
extern int soc_read(int sd, char *buf, size_t sz);
extern void set_proxy(char *default_url, int default_port);
extern int connect_server(int timeout, concptr host, int port);
extern int disconnect_server(int sd);
extern concptr soc_err(void);

extern void prepare_movie_hooks(void);
extern void prepare_browse_movie_aux(concptr filename);
extern void prepare_browse_movie(concptr filename);
extern void browse_movie(void);
extern bool browsing_movie;

extern void kamaenaoshi(INVENTORY_IDX item);
