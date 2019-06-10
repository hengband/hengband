#pragma once

/*
 * Hack -- Prepare to use the "Secure" routines
 */
#if defined(SET_UID) && defined(SECURE)
extern int PlayerUID;
# define getuid() PlayerUID
# define geteuid() PlayerUID
#endif

#define KEYMAP_MODE_ORIG	0 /*!< オリジナルキー配置 / Mode for original keyset commands */
#define KEYMAP_MODE_ROGUE	1 /*!< ローグライクキー配置 / Mode for roguelike keyset commands */
#define KEYMAP_MODES		2 /*!< キー配置の数 / Number of keymap modes */

#define SCREEN_BUF_MAX_SIZE (4 * 65536) /*!< Max size of screen dump buffer */

/* Cheat Info Type */
#define CHEAT_OBJECT 0
#define CHEAT_MONSTER 1
#define CHEAT_DUNGEON 2
#define CHEAT_MISC 3

/*
 * Max numbers of macro trigger names
 */
#define MAX_MACRO_MOD 12
#define MAX_MACRO_TRIG 200 /*!< 登録を許すマクロ（トリガー）の最大数 */

/*
 * Object flags
 *
 * Old variables for object flags such as flags1, flags2, and flags3
 * are obsolated.  Now single array flgs[TR_FLAG_SIZE] contains all
 * object flags.  And each flag is refered by single index number
 * instead of a bit mask.
 *
 * Therefore it's very easy to add a lot of new flags; no one need to
 * worry about in which variable a new flag should be put, nor to
 * modify a huge number of files all over the source directory at once
 * to add new flag variables such as flags4, a_ability_flags1, etc...
 *
 * All management of flags is now treated using a set of macros
 * instead of bit operations.
 * Note: These macros are using division, modulo, and bit shift
 * operations, and it seems that these operations are rather slower
 * than original bit operation.  But since index numbers are almost
 * always given as constant, such slow operations are performed in the
 * compile time.  So there is no problem on the speed.
 *
 * Exceptions of new flag management is a set of flags to control
 * object generation and the curse flags.  These are not yet rewritten
 * in new index form; maybe these have no merit of rewriting.
 */

#define have_flag(ARRAY, INDEX) !!((ARRAY)[(INDEX)/32] & (1L << ((INDEX)%32)))
#define add_flag(ARRAY, INDEX) ((ARRAY)[(INDEX)/32] |= (1L << ((INDEX)%32)))
#define remove_flag(ARRAY, INDEX) ((ARRAY)[(INDEX)/32] &= ~(1L << ((INDEX)%32)))
#define is_pval_flag(INDEX) ((TR_STR <= (INDEX) && (INDEX) <= TR_MAGIC_MASTERY) || (TR_STEALTH <= (INDEX) && (INDEX) <= TR_BLOWS))
#define have_pval_flags(ARRAY) !!((ARRAY)[0] & (0x00003f7f))

 /*
   Language selection macro
 */
#ifdef JP
#define _(JAPANESE,ENGLISH) (JAPANESE)
#else
#define _(JAPANESE,ENGLISH) (ENGLISH)
#endif


 /*
 * Sort-array element
 */
typedef struct tag_type tag_type;

struct tag_type
{
	int tag;
	int index;
};

/*
 * An entry for the object/monster allocation functions
 *
 * Pass 1 is determined from allocation information
 * Pass 2 is determined from allocation restriction
 * Pass 3 is determined from allocation calculation
 */

typedef struct alloc_entry alloc_entry;

struct alloc_entry
{
	KIND_OBJECT_IDX index;		/* The actual index */

	DEPTH level;		/* Base dungeon level */
	PROB prob1;		/* Probability, pass 1 */
	PROB prob2;		/* Probability, pass 2 */
	PROB prob3;		/* Probability, pass 3 */

	u16b total;		/* Unused for now */
};

extern u32b message__next;
extern u32b message__last;
extern u32b message__head;
extern u32b message__tail;
extern u32b *message__ptr;
extern char *message__buf;

extern bool msg_flag;

extern s16b macro__num;
extern concptr *macro__pat;
extern concptr *macro__act;
extern bool *macro__cmd;
extern char *macro__buf;

extern bool get_com_no_macros;

extern bool inkey_base;
extern bool inkey_xtra;
extern bool inkey_scan;
extern bool inkey_flag;

extern bool use_menu;

extern pos_list tmp_pos;

/*
 * Automatically generated "variable" declarations
 */
extern int max_macrotrigger;
extern concptr macro_template;
extern concptr macro_modifier_chr;
extern concptr macro_modifier_name[MAX_MACRO_MOD];
extern concptr macro_trigger_name[MAX_MACRO_TRIG];
extern concptr macro_trigger_keycode[2][MAX_MACRO_TRIG];

extern COMMAND_CODE command_cmd;
extern COMMAND_ARG command_arg;
extern s16b command_rep;
extern DIRECTION command_dir;
extern s16b command_see;
extern TERM_LEN command_gap;
extern s16b command_wrk;
extern s16b command_new;

extern concptr keymap_act[KEYMAP_MODES][256];

/*** Music constants ***/

#define MUSIC_BASIC_DEFAULT    0
#define MUSIC_BASIC_GAMEOVER   1
#define MUSIC_BASIC_EXIT       2
#define MUSIC_BASIC_TOWN       3
#define MUSIC_BASIC_FIELD1     4
#define MUSIC_BASIC_FIELD2     5
#define MUSIC_BASIC_FIELD3     6
#define MUSIC_BASIC_DUN_LOW    7
#define MUSIC_BASIC_DUN_MED    8
#define MUSIC_BASIC_DUN_HIGH   9
#define MUSIC_BASIC_DUN_FEEL1 10
#define MUSIC_BASIC_DUN_FEEL2 11
#define MUSIC_BASIC_WINNER    12
#define MUSIC_BASIC_BUILD     13
#define MUSIC_BASIC_WILD      14
#define MUSIC_BASIC_QUEST     15
#define MUSIC_BASIC_ARENA     16
#define MUSIC_BASIC_BATTLE    17
#define MUSIC_BASIC_QUEST_CLEAR 18
#define MUSIC_BASIC_FINAL_QUEST_CLEAR 19
#define MUSIC_BASIC_AMBUSH    20
#define MUSIC_BASIC_MAX       21 /*!< BGM定義の最大数 */

/*** Sound constants ***/

/*
 * Mega-Hack -- some primitive sound support (see "main-win.c")
 *
 * Some "sound" constants for "Term_xtra(TERM_XTRA_SOUND, val)"
 */
#define SOUND_HIT        1
#define SOUND_MISS       2
#define SOUND_FLEE       3
#define SOUND_DROP       4
#define SOUND_KILL       5
#define SOUND_LEVEL      6
#define SOUND_DEATH      7
#define SOUND_STUDY      8
#define SOUND_TELEPORT   9
#define SOUND_SHOOT     10
#define SOUND_QUAFF     11
#define SOUND_ZAP       12
#define SOUND_WALK      13
#define SOUND_TPOTHER   14
#define SOUND_HITWALL   15
#define SOUND_EAT       16
#define SOUND_STORE1    17
#define SOUND_STORE2    18
#define SOUND_STORE3    19
#define SOUND_STORE4    20
#define SOUND_DIG       21
#define SOUND_OPENDOOR  22
#define SOUND_SHUTDOOR  23
#define SOUND_TPLEVEL   24
#define SOUND_SCROLL	25
#define SOUND_BUY	    26
#define SOUND_SELL	    27
#define SOUND_WARN	    28
#define SOUND_ROCKET    29 /*!< Somebody's shooting rockets */
#define SOUND_N_KILL    30 /*!< The player kills a non-living/undead monster */
#define SOUND_U_KILL    31 /*!< The player kills a unique */
#define SOUND_QUEST     32 /*!< The player has just completed a quest */
#define SOUND_HEAL      33 /*!< The player was healed a little bit */
#define SOUND_X_HEAL    34 /*!< The player was healed full health */
#define SOUND_BITE      35 /*!< A monster bites you */
#define SOUND_CLAW      36 /*!< A monster claws you */
#define SOUND_M_SPELL   37 /*!< A monster casts a miscellaneous spell */
#define SOUND_SUMMON    38 /*!< A monster casts a summoning spell  */
#define SOUND_BREATH    39 /*!< A monster breathes */
#define SOUND_BALL      40 /*!< A monster casts a ball / bolt spell */
#define SOUND_M_HEAL    41 /*!< A monster heals itself somehow */
#define SOUND_ATK_SPELL 42 /*!< A monster casts a misc. offensive spell */
#define SOUND_EVIL      43 /*!< Something nasty has just happened! */
#define SOUND_TOUCH     44 /*!< A monster touches you */
#define SOUND_STING     45 /*!< A monster stings you */
#define SOUND_CRUSH     46 /*!< A monster crushes / envelopes you */
#define SOUND_SLIME     47 /*!< A monster drools/spits/etc on you */
#define SOUND_WAIL      48 /*!< A monster wails */
#define SOUND_WINNER    49 /*!< Just won the game! */
#define SOUND_FIRE      50 /*!< An item was burned  */
#define SOUND_ACID      51 /*!< An item was destroyed by acid */
#define SOUND_ELEC      52 /*!< An item was destroyed by electricity */
#define SOUND_COLD      53 /*!< An item was shattered */
#define SOUND_ILLEGAL   54 /*!< Illegal command attempted */
#define SOUND_FAIL      55 /*!< Fail to get a spell off / activate an item */
#define SOUND_WAKEUP    56 /*!< A monster wakes up */
#define SOUND_INVULN    57 /*!< Invulnerability! */
#define SOUND_FALL      58 /*!< Falling through a trapdoor... */
#define SOUND_PAIN      59 /*!< A monster is in pain! */
#define SOUND_DESTITEM  60 /*!< An item was destroyed by misc. means */
#define SOUND_MOAN      61 /*!< A monster makes a moan/beg/insult attack */
#define SOUND_SHOW      62 /*!< A monster makes a "show" attack */
#define SOUND_UNUSED    63 /*!< (no sound for gaze attacks) */
#define SOUND_EXPLODE   64 /*!< Something (or somebody) explodes */
#define SOUND_GLASS     65 /*!< A glass feature was crashed */
#define SOUND_REFLECT   66 /*!< A bolt was reflected */

/*
 * Mega-Hack -- maximum known sounds
 */
#define SOUND_MAX 67 /*!< 効果音定義の最大数 */

/*!
 * @brief 銘情報の最大数 / Maximum number of "quarks" (see "io.c")
 * @note
 * Default: assume at most 512 different inscriptions are used<br>
 * Was 512... 256 quarks added for random artifacts<br>
 */
#define QUARK_MAX       768

/*
 * OPTION: Maximum number of messages to remember (see "io.c")
 * Default: assume maximal memorization of 2048 total messages
 */
#define MESSAGE_MAX  81920

/*
 * OPTION: Maximum space for the message text buffer (see "io.c")
 * Default: assume that each of the 2048 messages is repeated an
 * average of three times, and has an average length of 48
 */
#define MESSAGE_BUF 655360

/*
 * Hack -- The main "screen"
 */
#define term_screen     (angband_term[0])

/*
 * Hack -- conditional (or "bizarre") externs
 */

#ifdef SET_UID
extern void user_name(char *buf, int id);
#endif

#if 0
#ifndef HAS_STRICMP
extern int stricmp(concptr a, concptr b);
#endif
#endif

#ifndef HAVE_USLEEP
extern int usleep(huge usecs);
#endif

#if defined(MAC_MPW) && defined(CARBON)
extern void convert_pathname(char *path);
#endif

#if defined(MACH_O_CARBON)
extern void fsetfileinfo(concptr path, u32b fcreator, u32b ftype);
#endif

#if defined(MAC_MPW) || defined(MACH_O_CARBON)
/* Globals needed */
extern  u32b _ftype;
extern  u32b _fcreator;
#endif

extern const char hexsym[16];

extern errr path_parse(char *buf, int max, concptr file);
extern errr path_build(char *buf, int max, concptr path, concptr file);
extern FILE *my_fopen(concptr file, concptr mode);
extern FILE *my_fopen_temp(char *buf, int max);
extern errr my_fgets(FILE *fff, char *buf, huge n);
extern errr my_fputs(FILE *fff, concptr buf, huge n);
extern errr my_fclose(FILE *fff);
extern errr fd_kill(concptr file);
extern errr fd_move(concptr file, concptr what);
extern errr fd_copy(concptr file, concptr what);
extern int fd_make(concptr file, BIT_FLAGS mode);
extern int fd_open(concptr file, int flags);
extern errr fd_lock(int fd, int what);
extern errr fd_seek(int fd, huge n);
extern errr fd_chop(int fd, huge n);
extern errr fd_read(int fd, char *buf, huge n);
extern errr fd_write(int fd, concptr buf, huge n);
extern errr fd_close(int fd);
extern void flush(void);
extern void bell(void);
extern errr play_music(int type, int num);
extern void select_floor_music(void);
extern void sound(int num);
extern void move_cursor(int row, int col);
extern void text_to_ascii(char *buf, concptr str);
extern void ascii_to_text(char *buf, concptr str);
extern errr macro_add(concptr pat, concptr act);
extern sint macro_find_exact(concptr pat);
extern char inkey(void);
extern concptr quark_str(STR_OFFSET num);
extern void quark_init(void);
extern u16b quark_add(concptr str);
extern s32b message_num(void);
extern concptr message_str(int age);
extern void message_add(concptr msg);
extern void msg_erase(void);
extern void msg_print(concptr msg);
extern void msg_print_wizard(int cheat_type, concptr msg);
#ifndef SWIG
extern void msg_format(concptr fmt, ...);
extern void msg_format_wizard(int cheat_type, concptr fmt, ...);
#endif /* SWIG */
extern void screen_save(void);
extern void screen_load(void);
extern void c_put_str(TERM_COLOR attr, concptr str, TERM_LEN row, TERM_LEN col);
extern void put_str(concptr str, TERM_LEN row, TERM_LEN col);
extern void c_prt(TERM_COLOR attr, concptr str, TERM_LEN row, TERM_LEN col);
extern void prt(concptr str, TERM_LEN row, TERM_LEN col);
extern void c_roff(TERM_COLOR attr, concptr str);
extern void roff(concptr str);
extern void clear_from(int row);
extern bool askfor_aux(char *buf, int len, bool numpad_cursor);
extern bool askfor(char *buf, int len);
extern bool get_string(concptr prompt, char *buf, int len);

/*
 * Bit flags for control of get_check_strict()
 */
#define CHECK_OKAY_CANCEL 0x01
#define CHECK_NO_ESCAPE   0x02
#define CHECK_NO_HISTORY  0x04
#define CHECK_DEFAULT_Y   0x08
extern bool get_check(concptr prompt);
extern bool get_check_strict(concptr prompt, BIT_FLAGS mode);

extern bool get_com(concptr prompt, char *command, bool z_escape);
extern QUANTITY get_quantity(concptr prompt, QUANTITY max);
extern void pause_line(int row);
extern void request_command(int shopping);
extern bool is_a_vowel(int ch);
extern int get_keymap_dir(char ch);
extern errr type_string(concptr str, uint len);
extern void roff_to_buf(concptr str, int wlen, char *tbuf, size_t bufsize);

#ifdef SORT_R_INFO
extern void tag_sort(tag_type elements[], int number);
#endif /* SORT_R_INFO */

#ifdef SUPPORT_GAMMA
extern byte gamma_table[256];
extern void build_gamma_table(int gamma);
#endif /* SUPPORT_GAMMA */

extern size_t my_strcpy(char *buf, concptr src, size_t bufsize);
extern size_t my_strcat(char *buf, concptr src, size_t bufsize);
extern char *my_strstr(concptr haystack, concptr needle);
extern char *my_strchr(concptr ptr, char ch);
extern void str_tolower(char *str);

/*
 * Special key code used for inkey_special()
 */
#define SKEY_MOD_MASK     0x0f00
#define SKEY_MOD_SHIFT    0x0100
#define SKEY_MOD_CONTROL  0x0200

#define SKEY_MASK         0xf000
#define SKEY_DOWN   	  0xf001
#define SKEY_LEFT   	  0xf002
#define SKEY_RIGHT  	  0xf003
#define SKEY_UP     	  0xf004
#define SKEY_PGUP   	  0xf005
#define SKEY_PGDOWN 	  0xf006
#define SKEY_TOP    	  0xf007
#define SKEY_BOTTOM 	  0xf008
extern int inkey_special(bool numpad_cursor);

/* util.c */
extern void repeat_push(COMMAND_CODE what);
extern bool repeat_pull(COMMAND_CODE *what);
extern void repeat_check(void);
