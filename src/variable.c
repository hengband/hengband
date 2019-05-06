/*!
 * @file variable.c
 * @brief グローバル変数定義 / Angband variables
 * @date 2014/10/05
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke<br>
 * <br>
 * This software may be copied and distributed for educational, research,<br>
 * and not for profit purposes provided that this copyright and statement<br>
 * are included in all such copies.  Other copyrights may also apply.<br>
 */

#include "angband.h"
#include "geometry.h"

/*!
 * コピーライト情報 /
 * Hack -- Link a copyright message into the executable
 */
const concptr copyright[5] =
{
	"Copyright (c) 1989 James E. Wilson, Robert A. Keoneke",
	"",
	"This software may be copied and distributed for educational, research,",
	"and not for profit purposes provided that this copyright and statement",
	"are included in all such copies."
};

int max_macrotrigger = 0; /*!< 現在登録中のマクロ(トリガー)の数 */
concptr macro_template = NULL; /*!< Angband設定ファイルのT: タグ情報から読み込んだ長いTコードを処理するために利用する文字列ポインタ */
concptr macro_modifier_chr; /*!< &x# で指定されるマクロトリガーに関する情報を記録する文字列ポインタ */
concptr macro_modifier_name[MAX_MACRO_MOD]; /*!< マクロ上で取り扱う特殊キーを文字列上で表現するためのフォーマットを記録した文字列ポインタ配列 */
concptr macro_trigger_name[MAX_MACRO_TRIG]; /*!< マクロのトリガーコード */
concptr macro_trigger_keycode[2][MAX_MACRO_TRIG];  /*!< マクロの内容 */

int level_up = 0; /*!< レベルアップの際に遅延してcalc_mana()関数上で上昇量を表示するかどうかの判定フラグ */

/*
 * Run-time arguments
 */
bool arg_fiddle;			/* Command arg -- Request fiddle mode */
bool arg_wizard;			/* Command arg -- Request wizard mode */
bool arg_sound;				/* Command arg -- Request special sounds */
bool arg_music;				/* Command arg -- Request special musics */
byte arg_graphics;			/* Command arg -- Request graphics mode */
bool arg_monochrome;		/* Command arg -- Request monochrome mode */
bool arg_force_original;	/* Command arg -- Request original keyset */
bool arg_force_roguelike;	/* Command arg -- Request roguelike keyset */
bool arg_bigtile = FALSE;	/* Command arg -- Request big tile mode */

/*
 * Various things
 */
bool character_generated;	/* The character exists */
bool character_dungeon;		/* The character has a dungeon */
bool character_loaded;		/* The character was loaded from a savefile */
bool character_saved;		/* The character was just saved to a savefile */

bool character_icky;		/* The game is in an icky full screen mode */
bool character_xtra;		/* The game is in an icky startup mode */

bool creating_savefile;		/* New savefile is currently created */

s16b command_cmd;		/* Current "Angband Command" */

COMMAND_ARG command_arg;	/*!< 各種コマンドの汎用的な引数として扱う / Gives argument of current command */
COMMAND_NUM command_rep;	/*!< 各種コマンドの汎用的なリピート数として扱う / Gives repetition of current command */
DIRECTION command_dir;		/*!< 各種コマンドの汎用的な方向値処理として扱う/ Gives direction of current command */

s16b command_see;		/* See "object1.c" */
s16b command_wrk;		/* See "object1.c" */

TERM_LEN command_gap = 999;         /* See "object1.c" */

s16b command_new;		/* Command chaining from inven/equip view */

bool msg_flag;			/* Used in msg_print() for "buffering" */

s16b running;			/* Current counter for running, if any */

GAME_TURN old_battle;

s16b signal_count;		/* Hack -- Count interupts */

bool inkey_base;		/* See the "inkey()" function */
bool inkey_xtra;		/* See the "inkey()" function */
bool inkey_scan;		/* See the "inkey()" function */
bool inkey_flag;		/* See the "inkey()" function */
bool get_com_no_macros = FALSE;	/* Expand macros in "get_com" or not */

OBJECT_SUBTYPE_VALUE coin_type;	/* Hack -- force coin type */

bool opening_chest;		/* Hack -- prevent chest generation */

bool shimmer_monsters;	/* Hack -- optimize multi-hued monsters */
bool shimmer_objects;	/* Hack -- optimize multi-hued objects */

bool repair_monsters;	/* Hack -- optimize detect monsters */
bool repair_objects;	/* Hack -- optimize detect objects */

MONSTER_IDX hack_m_idx = 0;	/* Hack -- see "process_monsters()" */
MONSTER_IDX hack_m_idx_ii = 0;

bool multi_rew = FALSE;

int total_friends = 0;
int leaving_quest = 0;
bool reinit_wilderness = FALSE;

/*
 * Dungeon size info
 */

POSITION panel_row_min, panel_row_max;
POSITION panel_col_min, panel_col_max;
POSITION panel_col_prt, panel_row_prt;


/*
 * Targetting variables
 */
MONSTER_IDX target_who;
POSITION target_col;
POSITION target_row;


/*
 * User info
 */
int player_uid;
int player_euid;
int player_egid;

/*
 * Stripped version of "player_name"
 */
char player_base[32];


/*
 * Buffer to hold the current savefile name
 * 'savefile' holds full path name. 'savefile_base' holds only base name.
 */
char savefile[1024];
char savefile_base[40];


pos_list tmp_pos;


/*
 * Number of active macros.
 */
s16b macro__num;

/*
 * Array of macro patterns [MACRO_MAX]
 */
concptr *macro__pat;

/*
 * Array of macro actions [MACRO_MAX]
 */
concptr *macro__act;

/*
 * Array of macro types [MACRO_MAX]
 */
bool *macro__cmd;

/*
 * Current macro action [1024]
 */
char *macro__buf;


/*
 * The number of quarks
 */
STR_OFFSET quark__num;

/*
 * The pointers to the quarks [QUARK_MAX]
 */
concptr *quark__str;

BIT_FLAGS option_flag[8]; //!< The array of normal options
BIT_FLAGS option_mask[8]; //!< The array of normal options
BIT_FLAGS window_flag[8]; //!< The array of window options
BIT_FLAGS window_mask[8]; //!< The array of window options

/*
 * The array of window pointers
 */
term *angband_term[8];

/*
 * Standard window names
 */
const char angband_term_name[8][16] =
{
	"Hengband",
	"Term-1",
	"Term-2",
	"Term-3",
	"Term-4",
	"Term-5",
	"Term-6",
	"Term-7"
};


/*
 * Global table of color definitions
 */
byte angband_color_table[256][4] =
{
	{0x00, 0x00, 0x00, 0x00},	/* TERM_DARK */
	{0x00, 0xFF, 0xFF, 0xFF},	/* TERM_WHITE */
	{0x00, 0x80, 0x80, 0x80},	/* TERM_SLATE */
	{0x00, 0xFF, 0x80, 0x00},	/* TERM_ORANGE */
	{0x00, 0xC0, 0x00, 0x00},	/* TERM_RED */
	{0x00, 0x00, 0x80, 0x40},	/* TERM_GREEN */
	{0x00, 0x00, 0x00, 0xFF},	/* TERM_BLUE */
	{0x00, 0x80, 0x40, 0x00},	/* TERM_UMBER */
	{0x00, 0x40, 0x40, 0x40},	/* TERM_L_DARK */
	{0x00, 0xC0, 0xC0, 0xC0},	/* TERM_L_WHITE */
	{0x00, 0xFF, 0x00, 0xFF},	/* TERM_VIOLET */
	{0x00, 0xFF, 0xFF, 0x00},	/* TERM_YELLOW */
	{0x00, 0xFF, 0x00, 0x00},	/* TERM_L_RED */
	{0x00, 0x00, 0xFF, 0x00},	/* TERM_L_GREEN */
	{0x00, 0x00, 0xFF, 0xFF},	/* TERM_L_BLUE */
	{0x00, 0xC0, 0x80, 0x40}	/* TERM_L_UMBER */
};


/*
 * Standard sound names
 */
const concptr angband_sound_name[SOUND_MAX] =
{
	"dummy",
	"hit",
	"miss",
	"flee",
	"drop",
	"kill",
	"level",
	"death",
	"study",
	"teleport",
	"shoot",
	"quaff",
	"zap",
	"walk",
	"tpother",
	"hitwall",
	"eat",
	"store1",
	"store2",
	"store3",
	"store4",
	"dig",
	"opendoor",
	"shutdoor",
	"tplevel",
	"scroll",
	"buy",
	"sell",
	"warn",
	"rocket",
	"n_kill",
	"u_kill",
	"quest",
	"heal",
	"x_heal",
	"bite",
	"claw",
	"m_spell",
	"summon",
	"breath",
	"ball",
	"m_heal",
	"atkspell",
	"evil",
	"touch",
	"sting",
	"crush",
	"slime",
	"wail",
	"winner",
	"fire",
	"acid",
	"elec",
	"cold",
	"illegal",
	"fail",
	"wakeup",
	"invuln",
	"fall",
	"pain",
	"destitem",
	"moan",
	"show",
	"unused",
	"explode",
	"glass",
	"reflect",
};

/*
 * Standard music names
 */
const concptr angband_music_basic_name[MUSIC_BASIC_MAX] =
{
	"default",
	"gameover",
	"exit",
	"town",
	"field1",
	"field2",
	"field3",
	"dun_low",
	"dun_med",
	"dun_high",
	"feel1",
	"feel2",
	"winner",
	"build",
	"wild",
	"quest",
	"arena",
	"battle",
	"quest_clear",
	"final_quest_clear",
	"ambush",
};


/*
 * Number of floor_id used from birth
 */
FLOOR_IDX max_floor_id;

/*
 * Sign for current process used in temporal files.
 * Actually it is the start time of current process.
 */
u32b saved_floor_file_sign;


/*
 * Keymaps for each "mode" associated with each keypress.
 */
concptr keymap_act[KEYMAP_MODES][256];



/*** Player information ***/

/*
 * Static player info record
 */
player_type p_body;

/*
 * Pointer to the player info
 */
player_type *p_ptr = &p_body;


/*
 * Here is a "pseudo-hook" used during calls to "get_item()" and
 * "show_inven()" and "show_equip()", and the choice window routines.
 */
OBJECT_TYPE_VALUE item_tester_tval;


/*
 * Here is a "hook" used during calls to "get_item()" and
 * "show_inven()" and "show_equip()", and the choice window routines.
 */
bool (*item_tester_hook)(object_type*);




/*
 * Hack -- function hooks to restrict "get_mon_num_prep()" function
 */
monsterrace_hook_type get_mon_num_hook;
monsterrace_hook_type get_mon_num2_hook;


/*
 * Hack -- function hook to restrict "get_obj_num_prep()" function
 */
bool (*get_obj_num_hook)(KIND_OBJECT_IDX k_idx);

/*
 * Maximum number of vaults in v_info.txt
 */
VAULT_IDX max_v_idx;

/*
 * Maximum number of terrain features in f_info.txt
 */
FEAT_IDX max_f_idx;

/*
 * Maximum number of ego-items in e_info.txt
 */
EGO_IDX max_e_idx;

/*
 * Maximum number of dungeon in e_info.txt
 */
DUNGEON_IDX max_d_idx;

/*
 * Flags for initialization
 */
int init_flags;


/*
 * The "highscore" file descriptor, if available.
 */
int highscore_fd = -1;

bool can_save = FALSE;        /* Game can be saved */


int cap_mon;
int cap_mspeed;
HIT_POINT cap_hp;
HIT_POINT cap_maxhp;
STR_OFFSET cap_nickname;

MONRACE_IDX battle_mon[4];
int sel_monster;
int battle_odds;
PRICE kakekin;
u32b mon_odds[4];

MONSTER_IDX pet_t_m_idx;
MONSTER_IDX riding_t_m_idx;

MONSTER_IDX today_mon;

bool write_level;

u32b start_time;

bool sukekaku;
bool new_mane;

bool mon_fight;

bool generate_encounter;

concptr screen_dump = NULL;


/*
 * Which dungeon ?
 */
DEPTH *max_dlv;

COMMAND_CODE now_message;
bool use_menu;

#ifdef CHUUKEI
bool chuukei_server;
bool chuukei_client;
char *server_name;
int server_port;
#endif

/* for movie */
bool browsing_movie;


/* for snipers */
bool reset_concent = FALSE;   /* Concentration reset flag */
bool is_fired = FALSE;

