
/*
 * Software options (set via the '=' command).  See "tables.c"
 */

/*** Input Options ***/

bool rogue_like_commands;	/* Rogue-like commands */
bool always_pickup;	/* Pick things up by default */
bool carry_query_flag;	/* Prompt before picking things up */
bool quick_messages;	/* Activate quick messages */
bool auto_more;	/* Automatically clear '-more-' prompts */
bool command_menu;	/* Enable command selection menu */
bool other_query_flag;	/* Prompt for floor item selection */
bool use_old_target;	/* Use old target by default */
bool always_repeat;	/* Repeat obvious commands */
bool confirm_destroy;	/* Prompt for destruction of known worthless items */
bool confirm_wear;	/* Confirm to wear/wield known cursed items */
bool confirm_quest;	/* Prompt before exiting a quest level */
bool target_pet;	/* Allow targetting pets */

#ifdef ALLOW_EASY_OPEN
bool easy_open;	/* Automatically open doors */
#endif

#ifdef ALLOW_EASY_DISARM
bool easy_disarm;	/* Automatically disarm traps */
#endif

#ifdef ALLOW_EASY_FLOOR
bool easy_floor;	/* Display floor stacks in a list */
#endif

bool use_command;	/* Allow unified use command */
bool over_exert;	/* Allow casting spells when short of mana */
bool numpad_as_cursorkey;	/* Use numpad keys as cursor key in editor mode */


							/*** Map Screen Options ***/

bool center_player;	/* Center map while walking (*slow*) */
bool center_running;	/* Centering even while running */
bool view_yellow_lite;	/* Use special colors for torch-lit grids */
bool view_bright_lite;	/* Use special colors for 'viewable' grids */
bool view_granite_lite;	/* Use special colors for wall grids (slow) */
bool view_special_lite;	/* Use special colors for floor grids (slow) */
bool view_perma_grids;	/* Map remembers all perma-lit grids */
bool view_torch_grids;	/* Map remembers all torch-lit grids */
bool view_unsafe_grids;	/* Map marked by detect traps */
bool view_reduce_view;	/* Reduce view-radius in town */
bool fresh_before;	/* Flush output while continuous command */
bool fresh_after;	/* Flush output after monster's move */
bool fresh_message;	/* Flush output after every message */
bool hilite_player;	/* Hilite the player with the cursor */
bool display_path;	/* Display actual path before shooting */


					/*** Text Display Options ***/

bool plain_descriptions;	/* Plain object descriptions */
bool plain_pickup;	/* Plain pickup messages(japanese only) */
bool always_show_list;	/* Always show list when choosing items */
bool depth_in_feet;	/* Show dungeon level in feet */
bool show_labels;	/* Show labels in object listings */
bool show_weights;	/* Show weights in object listings */
bool show_item_graph;	/* Show items graphics */
bool equippy_chars;	/* Display 'equippy' chars */
bool display_mutations;	/* Display mutations in 'C'haracter Display */
bool compress_savefile;	/* Compress messages in savefiles */
bool abbrev_extra;	/* Describe obj's extra resistances by abbreviation */
bool abbrev_all;	/* Describe obj's all resistances by abbreviation */
bool exp_need;	/* Show the experience needed for next level */
bool ignore_unview;	/* Ignore whenever any monster does */


					/*** Game-Play Options ***/

bool stack_force_notes;	/* Merge inscriptions when stacking */
bool stack_force_costs;	/* Merge discounts when stacking */
bool expand_list;	/* Expand the power of the list commands */
bool small_levels;	/* Allow unusually small dungeon levels */
bool always_small_levels;	/* Always create unusually small dungeon levels */
bool empty_levels;	/* Allow empty 'arena' levels */
bool bound_walls_perm;	/* Boundary walls become 'permanent wall' */
bool last_words;	/* Leave last words when your character dies */

#ifdef WORLD_SCORE
bool send_score;	/* Send score dump to the world score server */
#endif

bool allow_debug_opts;	/* Allow use of debug/cheat options */


						/*** Disturbance Options ***/

bool find_ignore_stairs;	/* Run past stairs */
bool find_ignore_doors;	/* Run through open doors */
bool find_cut;	/* Run past known corners */
bool check_abort;	/* Check for user abort while continuous command */
bool flush_failure;	/* Flush input on various failures */
bool flush_disturb;	/* Flush input whenever disturbed */
bool disturb_move;	/* Disturb whenever any monster moves */
bool disturb_high;	/* Disturb whenever high-level monster moves */
bool disturb_near;	/* Disturb whenever viewable monster moves */
bool disturb_pets;	/* Disturb when visible pets move */
bool disturb_panel;	/* Disturb whenever map panel changes */
bool disturb_state;	/* Disturb whenever player state changes */
bool disturb_minor;	/* Disturb whenever boring things happen */
bool ring_bell;	/* Audible bell (on errors, etc) */
bool disturb_trap_detect;	/* Disturb when leaving trap detected area */
bool alert_trap_detect;	/* Alert when leaving trap detected area */


						/*** Birth Options ***/

bool manual_haggle;	/* Manually haggle in stores */
bool easy_band;	/* Easy Mode (*) */
bool smart_learn;	/* Monsters learn from their mistakes (*) */
bool smart_cheat;	/* Monsters exploit players weaknesses (*) */
bool vanilla_town;	/* Use 'vanilla' town without quests and wilderness */
bool lite_town;	/* Use 'lite' town without a wilderness */
bool ironman_shops;	/* Stores are permanently closed (*) */
bool ironman_small_levels;	/* Always create unusually small dungeon levels (*) */
bool ironman_downward;	/* Disable recall and use of up stairs (*) */
bool ironman_empty_levels;	/* Always create empty 'arena' levels (*) */
bool ironman_rooms;	/* Always generate very unusual rooms (*) */
bool ironman_nightmare;	/* Nightmare mode(it isn't even remotely fair!)(*) */
bool left_hander;	/* Left-Hander */
bool preserve_mode;	/* Preserve artifacts (*) */
bool autoroller;	/* Allow use of autoroller for stats (*) */
bool autochara;	/* Autoroll for weight, height and social status */
bool powerup_home;	/* Increase capacity of your home (*) */
bool show_ammo_detail;	/* Show Description of ammo damage */
bool show_ammo_no_crit;	/* Show No-crit damage of ammo */
bool show_ammo_crit_ratio;	/* Show critical ratio of ammo */




							/*** Easy Object Auto-Destroyer ***/

bool destroy_items;	/* Use easy auto-destroyer */
bool destroy_feeling;	/* Apply auto-destroy as sense feeling */
bool destroy_identify;	/* Apply auto-destroy as identify an item */
bool leave_worth;	/* Auto-destroyer leaves known worthy items */
bool leave_equip;	/* Auto-destroyer leaves weapons and armour */
bool leave_chest;	/* Auto-destroyer leaves closed chests */
bool leave_wanted;	/* Auto-destroyer leaves wanted corpses */
bool leave_corpse;	/* Auto-destroyer leaves corpses and skeletons */
bool leave_junk;	/* Auto-destroyer leaves junk */
bool leave_special;	/* Auto-destroyer leaves items your race/class needs */


					/*** Play-record Options ***/

bool record_fix_art;	/* Record fixed artifacts */
bool record_rand_art;	/* Record random artifacts */
bool record_destroy_uniq;	/* Record when destroy unique monster */
bool record_fix_quest;	/* Record fixed quests */
bool record_rand_quest;	/* Record random quests */
bool record_maxdepth;	/* Record movements to deepest level */
bool record_stair;	/* Record recall and stair movements */
bool record_buy;	/* Record purchased items */
bool record_sell;	/* Record sold items */
bool record_danger;	/* Record hitpoint warning */
bool record_arena;	/* Record arena victories */
bool record_ident;	/* Record first identified items */
bool record_named_pet;	/* Record informations of named pets */
char record_o_name[MAX_NLEN];
s32b record_turn;


/* Cheating options */

bool cheat_peek;	/* Peek into object creation */
bool cheat_hear;	/* Peek into monster creation */
bool cheat_room;	/* Peek into dungeon creation */
bool cheat_xtra;	/* Peek into something else */
bool cheat_know;	/* Know complete monster info */
bool cheat_live;	/* Allow player to avoid death */
bool cheat_save;	/* Ask for saving death */
bool cheat_diary_output; /* Detailed info to diary */
bool cheat_turn;	/* Peek turn */
bool cheat_sight;


/* Special options */

byte hitpoint_warn;	/* Hitpoint warning (0 to 9) */
byte mana_warn;	/* Mana color (0 to 9) */

byte delay_factor;	/* Delay factor (0 to 9) */

bool autosave_l;	/* Autosave before entering new levels */
bool autosave_t;	/* Timed autosave */
s16b autosave_freq;     /* Autosave frequency */

