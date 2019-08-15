#pragma once
#include "h-type.h"

/*** Option Definitions ***/

#define OPT_PAGE_INPUT          1
#define OPT_PAGE_MAPSCREEN      2
#define OPT_PAGE_TEXT           3
#define OPT_PAGE_GAMEPLAY       4
#define OPT_PAGE_DISTURBANCE    5
#define OPT_PAGE_BIRTH          6
#define OPT_PAGE_AUTODESTROY    7
#define OPT_PAGE_PLAYRECORD    10

#define OPT_PAGE_JAPANESE_ONLY 99

/*!
 * チートオプションの最大数 / Number of cheating options
 */
#define CHEAT_MAX 10

/*** Input Options ***/

extern bool rogue_like_commands;	/* Rogue-like commands */
extern bool always_pickup;	/* Pick things up by default */
extern bool carry_query_flag;	/* Prompt before picking things up */
extern bool quick_messages;	/* Activate quick messages */
extern bool auto_more;	/* Automatically clear '-more-' prompts */
extern bool command_menu;	/* Enable command selection menu */
extern bool other_query_flag;	/* Prompt for floor item selection */
extern bool use_old_target;	/* Use old target by default */
extern bool always_repeat;	/* Repeat obvious commands */
extern bool confirm_destroy;	/* Prompt for destruction of known worthless items */
extern bool confirm_wear;	/* Confirm to wear/wield known cursed items */
extern bool confirm_quest;	/* Prompt before exiting a quest level */
extern bool target_pet;	/* Allow targetting pets */
extern bool easy_open;	/* Automatically open doors */
extern bool easy_disarm;	/* Automatically disarm traps */
extern bool easy_floor;	/* Display floor stacks in a list */
extern bool use_command;	/* Allow unified use command */
extern bool over_exert;	/* Allow casting spells when short of mana */
extern bool numpad_as_cursorkey;	/* Use numpad keys as cursor key in editor mode */


									/*** Map Screen Options ***/

extern bool center_player;	/* Center map while walking (*slow*) */
extern bool center_running;	/* Centering even while running */
extern bool view_yellow_lite;	/* Use special colors for torch-lit grids */
extern bool view_bright_lite;	/* Use special colors for 'viewable' grids */
extern bool view_granite_lite;	/* Use special colors for wall grids (slow) */
extern bool view_special_lite;	/* Use special colors for floor grids (slow) */
extern bool view_perma_grids;	/* Map remembers all perma-lit grids */
extern bool view_torch_grids;	/* Map remembers all torch-lit grids */
extern bool view_unsafe_grids;	/* Map marked by detect traps */
extern bool view_reduce_view;	/* Reduce view-radius in town */
extern bool fresh_before;	/* Flush output while continuous command */
extern bool fresh_after;	/* Flush output after monster's move */
extern bool fresh_message;	/* Flush output after every message */
extern bool hilite_player;	/* Hilite the player with the cursor */
extern bool display_path;	/* Display actual path before shooting */


							/*** Text Display Options ***/

extern bool plain_descriptions;	/* Plain object descriptions */
extern bool plain_pickup;	/* Plain pickup messages(japanese only) */
extern bool always_show_list;	/* Always show list when choosing items */
extern bool depth_in_feet;	/* Show dungeon level in feet */
extern bool show_labels;	/* Show labels in object listings */
extern bool show_weights;	/* Show weights in object listings */
extern bool show_item_graph;	/* Show items graphics */
extern bool equippy_chars;	/* Display 'equippy' chars */
extern bool display_mutations;	/* Display mutations in 'C'haracter Display */
extern bool compress_savefile;	/* Compress messages in savefiles */
extern bool abbrev_extra;	/* Describe obj's extra resistances by abbreviation */
extern bool abbrev_all;	/* Describe obj's all resistances by abbreviation */
extern bool exp_need;	/* Show the experience needed for next level */
extern bool ignore_unview;	/* Ignore whenever any monster does */
extern bool show_ammo_detail;	/* Show Description of ammo damage */
extern bool show_ammo_no_crit;	/* Show No-crit damage of ammo */
extern bool show_ammo_crit_ratio;	/* Show critical ratio of ammo */
extern bool show_actual_value;	/* Show actual value of skill */

								/*** Game-Play Options ***/

extern bool stack_force_notes;	/* Merge inscriptions when stacking */
extern bool stack_force_costs;	/* Merge discounts when stacking */
extern bool expand_list;	/* Expand the power of the list commands */
extern bool small_levels;	/* Allow unusually small dungeon levels */
extern bool always_small_levels;	/* Always create unusually small dungeon levels */
extern bool empty_levels;	/* Allow empty 'arena' levels */
extern bool bound_walls_perm;	/* Boundary walls become 'permanent wall' */
extern bool last_words;	/* Leave last words when your character dies */

#ifdef WORLD_SCORE
extern bool send_score;	/* Send score dump to the world score server */
#endif

extern bool allow_debug_opts;	/* Allow use of debug/cheat options */


								/*** Disturbance Options ***/

extern bool find_ignore_stairs;	/* Run past stairs */
extern bool find_ignore_doors;	/* Run through open doors */
extern bool find_cut;	/* Run past known corners */
extern bool check_abort;	/* Check for user abort while continuous command */
extern bool flush_failure;	/* Flush input on various failures */
extern bool flush_disturb;	/* Flush input whenever disturbed */
extern bool disturb_move;	/* Disturb whenever any monster moves */
extern bool disturb_high;	/* Disturb whenever high-level monster moves */
extern bool disturb_near;	/* Disturb whenever viewable monster moves */
extern bool disturb_pets;	/* Disturb when visible pets move */
extern bool disturb_panel;	/* Disturb whenever map panel changes */
extern bool disturb_state;	/* Disturb whenever player state changes */
extern bool disturb_minor;	/* Disturb whenever boring things happen */
extern bool ring_bell;	/* Audible bell (on errors, etc) */
extern bool disturb_trap_detect;	/* Disturb when leaving trap detected area */
extern bool alert_trap_detect;	/* Alert when leaving trap detected area */


								/*** Birth Options ***/

extern bool manual_haggle;	/* Manually haggle in stores */
extern bool easy_band;	/* Easy Mode (*) */
extern bool smart_learn;	/* Monsters learn from their mistakes (*) */
extern bool smart_cheat;	/* Monsters exploit players weaknesses (*) */
extern bool vanilla_town;	/* Use 'vanilla' town without quests and wilderness */
extern bool lite_town;	/* Use 'lite' town without a wilderness */
extern bool ironman_shops;	/* Stores are permanently closed (*) */
extern bool ironman_small_levels;	/* Always create unusually small dungeon levels (*) */
extern bool ironman_downward;	/* Disable recall and use of up stairs (*) */
extern bool ironman_empty_levels;	/* Always create empty 'arena' levels (*) */
extern bool ironman_rooms;	/* Always generate very unusual rooms (*) */
extern bool ironman_nightmare;	/* Nightmare mode(it isn't even remotely fair!)(*) */
extern bool left_hander;	/* Left-Hander */
extern bool preserve_mode;	/* Preserve artifacts (*) */
extern bool autoroller;	/* Allow use of autoroller for stats (*) */
extern bool autochara;	/* Autoroll for weight, height and social status */
extern bool powerup_home;	/* Increase capacity of your home (*) */


							/*** Easy Object Auto-Destroyer ***/

extern bool destroy_items;	/* Use easy auto-destroyer */
extern bool destroy_feeling;	/* Apply auto-destroy as sense feeling */
extern bool destroy_identify;	/* Apply auto-destroy as identify an item */
extern bool leave_worth;	/* Auto-destroyer leaves known worthy items */
extern bool leave_equip;	/* Auto-destroyer leaves weapons and armour */
extern bool leave_chest;	/* Auto-destroyer leaves closed chests */
extern bool leave_wanted;	/* Auto-destroyer leaves wanted corpses */
extern bool leave_corpse;	/* Auto-destroyer leaves corpses and skeletons */
extern bool leave_junk;	/* Auto-destroyer leaves junk */
extern bool leave_special;	/* Auto-destroyer leaves items your race/class needs */


							/*** Play-record Options ***/

extern bool record_fix_art;	/* Record fixed artifacts */
extern bool record_rand_art;	/* Record random artifacts */
extern bool record_destroy_uniq;	/* Record when destroy unique monster */
extern bool record_fix_quest;	/* Record fixed quests */
extern bool record_rand_quest;	/* Record random quests */
extern bool record_maxdepth;	/* Record movements to deepest level */
extern bool record_stair;	/* Record recall and stair movements */
extern bool record_buy;	/* Record purchased items */
extern bool record_sell;	/* Record sold items */
extern bool record_danger;	/* Record hitpoint warning */
extern bool record_arena;	/* Record arena victories */
extern bool record_ident;	/* Record first identified items */
extern bool record_named_pet;	/* Record informations of named pets */

extern char record_o_name[MAX_NLEN];
extern GAME_TURN record_turn;

extern bool cheat_peek;
extern bool cheat_hear;
extern bool cheat_room;
extern bool cheat_xtra;
extern bool cheat_know;
extern bool cheat_live;
extern bool cheat_save;
extern bool cheat_diary_output;
extern bool cheat_turn;
extern bool cheat_sight;

extern byte hitpoint_warn;
extern byte mana_warn;
extern byte delay_factor;
extern s16b autosave_freq;
extern bool autosave_t;
extern bool autosave_l;

extern bool use_sound;
extern bool use_music;
extern bool use_graphics;
extern bool use_bigtile;

extern bool arg_fiddle;
extern bool arg_wizard;
extern bool arg_music;
extern bool arg_sound;
extern byte arg_graphics;
extern bool arg_monochrome;
extern bool arg_force_original;
extern bool arg_force_roguelike;
extern bool arg_bigtile;

extern BIT_FLAGS option_flag[8];
extern BIT_FLAGS option_mask[8];
extern BIT_FLAGS window_flag[8];
extern BIT_FLAGS window_mask[8];
/*
 * Available "options"
 *
 *	- Address of actual option variable (or NULL)
 *
 *	- Normal Value (TRUE or FALSE)
 *
 *	- Option Page Number (or zero)
 *
 *	- Savefile Set (or zero)
 *	- Savefile Bit in that set
 *
 *	- Textual name (or NULL)
 *	- Textual description
 */

typedef struct option_type option_type;

struct option_type
{
	bool	*o_var;
	byte	o_norm;
	byte	o_page;
	byte	o_set;
	byte	o_bit;
	concptr	o_text;
	concptr	o_desc;
};

extern const option_type option_info[];
extern const option_type cheat_info[CHEAT_MAX];
extern const option_type autosave_info[2];

extern void extract_option_vars(void);

extern void do_cmd_options_aux(int page, concptr info);
extern void do_cmd_options(void);
