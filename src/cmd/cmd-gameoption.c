#include "system/angband.h"

#include "autopick/autopick.h"
#include "io/write-diary.h"
#include "cmd/cmd-gameoption.h"
#include "cmd/cmd-autopick.h"
#include "gameterm.h"
#include "view/display-main-window.h"
#include "cmd/cmd-dump.h"
#include "core/show-file.h"
#include "io/files.h"

#include "world/world.h"

/*
 * Software options (set via the '=' command).
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
bool target_pet;	/* Allow targeting pets */
bool easy_open;	/* Automatically open doors */
bool easy_disarm;	/* Automatically disarm traps */
bool easy_floor;	/* Display floor stacks in a list */
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
bool fresh_before;	/* Flush output while in repeated command */
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
bool check_abort;	/* Check for user abort while in repeated command */
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
bool show_actual_value;	/* Show actual value of skill */




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
bool record_named_pet;	/* Record information about named pets */
char record_o_name[MAX_NLEN];
GAME_TURN record_turn;


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

bool use_sound;			/* The "sound" mode is enabled */
bool use_music;			/* The "music" mode is enabled */
bool use_graphics;		/* The "graphics" mode is enabled */
bool use_bigtile = FALSE;

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

BIT_FLAGS option_flag[8]; //!< The array of normal options
BIT_FLAGS option_mask[8]; //!< The array of normal options
BIT_FLAGS window_flag[8]; //!< The array of window options
BIT_FLAGS window_mask[8]; //!< The array of window options

/*!
 * @brief オプションテーブル /
 * Available Options
 */
const option_type option_info[] =
{
	/*** Input Options ***/

	{ &rogue_like_commands,         FALSE, OPT_PAGE_INPUT, 0, 0,
	"rogue_like_commands",          _("ローグ風キー配置を使用する", "Rogue-like commands") },

	{ &always_pickup,               FALSE, OPT_PAGE_INPUT, 0, 5,
	"always_pickup",                _("常にアイテムを拾う" , "Pick things up by default") },

	{ &carry_query_flag,            FALSE, OPT_PAGE_INPUT, 0, 3,
	"carry_query_flag",             _("アイテムを拾う前に確認する", "Prompt before picking things up") },

	{ &quick_messages,              TRUE,  OPT_PAGE_INPUT, 0, 1,
	"quick_messages",               _("クイック・メッセージを使用する", "Activate quick messages") },

	{ &auto_more,                   FALSE, OPT_PAGE_INPUT, 2, 6,
	"auto_more",                    _("キー待ちしないで連続でメッセージを表示する", "Automatically clear '-more-' prompts") },

	{ &command_menu,                TRUE,  OPT_PAGE_INPUT, 2, 7,
	"command_menu",                 _("メニューによりコマンド選択を有効にする", "Enable command selection menu") },

	{ &other_query_flag,            FALSE, OPT_PAGE_INPUT, 0, 2,
	"other_query_flag",             _("床上のアイテムを使用するときに確認する", "Prompt for floor item selection") },

	{ &use_old_target,              FALSE, OPT_PAGE_INPUT, 0, 4,
	"use_old_target",               _("常に以前のターゲットを指定する", "Use old target by default") },

	{ &always_repeat,               TRUE,  OPT_PAGE_INPUT, 0, 6,
	"always_repeat",                _("コマンド自動繰り返し", "Repeat obvious commands") },

	{ &confirm_destroy,             FALSE, OPT_PAGE_INPUT, 5, 3,
	"confirm_destroy",              _("「無価値」なアイテムを破壊する時確認する", "Prompt for destruction of known worthless items") },

	{ &confirm_wear,                TRUE,  OPT_PAGE_INPUT, 5, 4,
	"confirm_wear",                 _("呪われた物を装備する時確認する", "Confirm to wear/wield known cursed items") },

	{ &confirm_quest,               TRUE,  OPT_PAGE_INPUT, 1, 9,
	"confirm_quest",                _("クエストを諦めて階段で逃げる前に確認する", "Prompt before exiting a quest level") },

	{ &target_pet,                  FALSE, OPT_PAGE_INPUT, 2, 5,
	"target_pet",                   _("ペットをターゲットにする", "Allow targeting pets") },

	{ &easy_open,                   TRUE,  OPT_PAGE_INPUT, 5, 7,
	"easy_open",                    _("自動的にドアを開ける", "Automatically open doors") },

	{ &easy_disarm,                 TRUE,  OPT_PAGE_INPUT, 5, 8,
	"easy_disarm",                  _("自動的に罠を解除する", "Automatically disarm traps") },

	{ &easy_floor,                  FALSE, OPT_PAGE_INPUT, 5, 9,
	"easy_floor",                   _("床上で重なったアイテムをリストする", "Display floor stacks in a list") },

	{ &use_command,                 FALSE, OPT_PAGE_INPUT, 5, 10,
	"use_command",                  _("「使う(a)」コマンドでアイテムを何でも使える", "Allow unified use command") },

	{ &over_exert,                  FALSE, OPT_PAGE_INPUT, 0, 29,
	"over_exert",                   _("MPが足りなくても魔法に挑戦する", "Allow casting spells when short of mana") },

	{ &numpad_as_cursorkey,         TRUE, OPT_PAGE_INPUT, 2, 31,
	"numpad_as_cursorkey",          _("エディタ内でテンキーをカーソルキーとして使う", "Use numpad keys as cursor keys in editor mode") },

/*** Map Screen Options ***/

	{ &center_player,               FALSE, OPT_PAGE_MAPSCREEN, 5, 11,
	"center_player",                _("常にプレイヤーを中心に置く(*遅い*)", "Center map while walking (*slow*)") },

	{ &center_running,              TRUE,  OPT_PAGE_MAPSCREEN, 5, 12,
	"center_running",               _("走っている時でも中心に置く", "Centering even while running") },

	{ &view_yellow_lite,            TRUE,  OPT_PAGE_MAPSCREEN, 1, 28,
	"view_yellow_lite",             _("明かりの範囲を特別な色で表示する", "Use special colors for torch-lit grids") },

	{ &view_bright_lite,            TRUE,  OPT_PAGE_MAPSCREEN, 1, 29,
	"view_bright_lite",             _("視界の範囲を特別な色で表示する", "Use special colors for 'viewable' grids") },

	{ &view_granite_lite,           TRUE,  OPT_PAGE_MAPSCREEN, 1, 30,
	"view_granite_lite",            _("壁を特別な色で表示する(重い)", "Use special colors for wall grids (slow)") },

	{ &view_special_lite,           TRUE,  OPT_PAGE_MAPSCREEN, 1, 31,
	"view_special_lite",            _("床を特別な色で表示する(重い)", "Use special colors for floor grids (slow)") },

	{ &view_perma_grids,            TRUE,  OPT_PAGE_MAPSCREEN, 1, 6,
	"view_perma_grids",             _("明るい場所はそのままにする", "Map remembers all perma-lit grids") },

	{ &view_torch_grids,            FALSE, OPT_PAGE_MAPSCREEN, 1, 7,
	"view_torch_grids",             _("明かりで照らした場所はそのままにする", "Map remembers all torch-lit grids") },

	{ &view_unsafe_grids,           FALSE, OPT_PAGE_MAPSCREEN, 1, 8,
	"view_unsafe_grids",            _("トラップ感知済みでない場所を表示する", "Map marked by detect traps") },

	{ &view_reduce_view,            FALSE, OPT_PAGE_MAPSCREEN, 1, 17,
	"view_reduce_view",             _("街では視野を狭くする", "Reduce view-radius in town") },

	{ &fresh_before,                TRUE,  OPT_PAGE_MAPSCREEN, 1, 23,
	"fresh_before",                 _("連続コマンド中に画面を再描画し続ける", "Flush output while in repeated command") },

	{ &fresh_after,                 FALSE, OPT_PAGE_MAPSCREEN, 1, 24,
	"fresh_after",                  _("コマンド後に画面を常に再描画し続ける", "Flush output after monster's move") },

	{ &fresh_message,               FALSE, OPT_PAGE_MAPSCREEN, 1, 25,
	"fresh_message",                _("メッセージの後に画面を再描画する", "Flush output after every message") },

	{ &hilite_player,               FALSE, OPT_PAGE_MAPSCREEN, 1, 27,
	"hilite_player",                _("プレイヤーにカーソルを合わせる", "Hilite the player with the cursor") },

	{ &display_path,                FALSE, OPT_PAGE_MAPSCREEN, 2, 8,
	"display_path",                 _("魔法や矢の軌跡を表示する", "Display actual path before shooting") },

/*** Text Display Options ***/

	{ &plain_descriptions,          TRUE,  OPT_PAGE_TEXT, 5, 1,
	"plain_descriptions",           _("アイテムの記述を簡略にする", "Plain object descriptions") },

	{ &plain_pickup,                FALSE, OPT_PAGE_TEXT, 6, 6,
	"plain_pickup",                 _("「拾った」メッセージを簡略化する", "Plain pickup messages(japanese only)") },

	{ &always_show_list,            TRUE,  OPT_PAGE_TEXT, 4, 0,
	"always_show_list",             _("選択時には常に一覧を表示する", "Always show list when choosing items") },

	{ &depth_in_feet,               FALSE, OPT_PAGE_TEXT, 0, 7,
	"depth_in_feet",                _("ダンジョンの深さをフィートで表示する", "Show dungeon level in feet") },

	{ &show_labels,                 TRUE,  OPT_PAGE_TEXT, 0, 10,
	"show_labels",                  _("装備一覧で装備場所を表示する", "Show labels in object listings") },

	{ &show_weights,                TRUE,  OPT_PAGE_TEXT, 0, 11,
	"show_weights",                 _("アイテム一覧で重量を表示する", "Show weights in object listings") },

	{ &show_item_graph,             TRUE,  OPT_PAGE_TEXT, 2, 0,
	"show_item_graph",              _("アイテムのシンボルを表示する", "Show items graphics") },

	{ &equippy_chars,               TRUE,  OPT_PAGE_TEXT, 1, 12,
	"equippy_chars",                _("ステータスに文字で装備を表示する", "Display 'equippy' chars") },

	{ &display_mutations,           FALSE, OPT_PAGE_TEXT, 5, 0,
	"display_mutations",            _("'C'コマンドで突然変異を表示する", "Display mutations in 'C'haracter Display") },

	{ &compress_savefile,           FALSE, OPT_PAGE_TEXT, 1, 26,
	"compress_savefile",            _("セーブ・ファイル中のメッセージを圧縮する", "Compress messages in savefiles") },

	{ &abbrev_extra,                FALSE, OPT_PAGE_TEXT, 2, 10,
	"abbrev_extra",                 _("アイテムに追加耐性/能力の略称を刻む", "Describe obj's extra resistances by abbreviation") },

	{ &abbrev_all,                  FALSE, OPT_PAGE_TEXT, 2, 11,
	"abbrev_all",                   _("アイテムに全ての耐性/能力の略称を刻む", "Describe obj's all resistances by abbreviation") },

	{ &exp_need,                    FALSE, OPT_PAGE_TEXT, 2, 12,
	"exp_need",                     _("次のレベルに必要な経験値を表示する", "Show the experience needed for next level") },

	{ &ignore_unview,               FALSE, OPT_PAGE_TEXT, 2, 13,
	"ignore_unview",                _("視界外のモンスターの行動を表示しない", "Ignore out-of-sight monster behavior") },

	{ &show_ammo_detail,            TRUE, OPT_PAGE_TEXT, 2, 14,
	"show_ammo_detail",             _("矢弾のダメージの説明を表示する", "Show description of ammo damage") },

	{ &show_ammo_no_crit,           FALSE, OPT_PAGE_TEXT, 2, 15,
	"show_ammo_no_crit",            _("会心を考慮しない場合の矢弾のダメージを表示する", "Show ammo damage with no critical") },

	{ &show_ammo_crit_ratio,        FALSE, OPT_PAGE_TEXT, 2, 16,
	"show_ammo_crit_ratio",         _("矢弾の会心発生率を表示する", "Show critical ratio of ammo") },

	{ &show_actual_value,           FALSE, OPT_PAGE_TEXT, 2, 17,
	"show_actual_vaule",            _("各技能値に実値を表示する", "Show actual value of skill") },


/*** Game-Play ***/

	{ &stack_force_notes,           TRUE,  OPT_PAGE_GAMEPLAY, 0, 8,
	"stack_force_notes",            _("異なる銘のアイテムをまとめる", "Merge inscriptions when stacking") },

	{ &stack_force_costs,           FALSE, OPT_PAGE_GAMEPLAY, 0, 9,
	"stack_force_costs",            _("異なる割引表示のアイテムをまとめる", "Merge discounts when stacking") },

	{ &expand_list,                 TRUE,  OPT_PAGE_GAMEPLAY, 1, 5,
	"expand_list",                  _("「一覧」コマンドを拡張する", "Expand the power of the list commands") },

	{ &small_levels,                TRUE,  OPT_PAGE_GAMEPLAY, 0, 30,
	"small_levels",                 _("非常に小さいフロアの生成を可能にする", "Allow unusually small dungeon levels") },

	{ &always_small_levels,         FALSE, OPT_PAGE_GAMEPLAY, 2, 3,
	"always_small_levels",          _("常に非常に小さいフロアを生成する", "Always create unusually small dungeon levels") },

	{ &empty_levels,                TRUE,  OPT_PAGE_GAMEPLAY, 0, 31,
	"empty_levels",                 _("空っぽの「アリーナ」レベルの生成を可能にする", "Allow empty 'arena' levels") },

	{ &bound_walls_perm,            FALSE, OPT_PAGE_GAMEPLAY, 2, 1,
	"bound_walls_perm",             _("ダンジョンの外壁を永久岩にする", "Boundary walls become 'permanent wall'") },

	{ &last_words,                  TRUE,  OPT_PAGE_GAMEPLAY, 0, 28,
	"last_words",                   _("キャラクターが死んだ時遺言をのこす", "Leave last words when your character dies") },

#ifdef WORLD_SCORE
	{ &send_score,                  TRUE,  OPT_PAGE_GAMEPLAY, 4, 6,
	"send_score",                   _("スコアサーバにスコアを送る", "Send score dump to the world score server") },
#endif

	{ &allow_debug_opts,            FALSE, OPT_PAGE_GAMEPLAY, 6, 11,
	"allow_debug_opts",             _("デバッグ/詐欺オプションを許可する", "Allow use of debug/cheat options") },

/*** Disturbance ***/

	{ &find_ignore_stairs,          FALSE, OPT_PAGE_DISTURBANCE, 0, 16,
	"find_ignore_stairs",           _("階段は通過する", "Run past stairs") },

	{ &find_ignore_doors,           TRUE,  OPT_PAGE_DISTURBANCE, 0, 17,
	"find_ignore_doors",            _("ドアは通過する", "Run through open doors") },

	{ &find_cut,                    FALSE, OPT_PAGE_DISTURBANCE, 0, 18,
	"find_cut",                     _("曲り角を斜めに最短距離で通過する", "Run past known corners") },

	{ &check_abort,                 TRUE,  OPT_PAGE_DISTURBANCE, 1, 18,
	"check_abort",                  _("連続コマンドはキー入力で中断する", "Check for user abort while in repeated command") },

	{ &flush_failure,               TRUE,  OPT_PAGE_DISTURBANCE, 1, 20,
	"flush_failure",                _("様々なミス発生時に入力をクリアする", "Flush input on various failures") },

	{ &flush_disturb,               FALSE, OPT_PAGE_DISTURBANCE, 1, 21,
	"flush_disturb",                _("障害発生時に入力をクリアする", "Flush input whenever disturbed") },

	{ &disturb_move,                FALSE, OPT_PAGE_DISTURBANCE, 0, 20,
	"disturb_move",                 _("どこのモンスターが動いても行動を中止する", "Disturb whenever any monster moves") },

	{ &disturb_high,                FALSE, OPT_PAGE_DISTURBANCE, 1, 3,
	"disturb_high",                 _("レベルの高いモンスターが動いたら行動を中止する", "Disturb whenever high-level monster moves") },

	{ &disturb_near,                TRUE,  OPT_PAGE_DISTURBANCE, 0, 21,
	"disturb_near",                 _("視界内のモンスターが動いたら行動を中止する", "Disturb whenever viewable monster moves") },

	{ &disturb_pets,                FALSE, OPT_PAGE_DISTURBANCE, 5, 6,
	"disturb_pets",                 _("視界内のペットが動いたら行動を中止する", "Disturb when visible pets move") },

	{ &disturb_panel,               TRUE,  OPT_PAGE_DISTURBANCE, 0, 22,
	"disturb_panel",                _("画面スクロール時に行動を中止する", "Disturb whenever map panel changes") },

	{ &disturb_state,               TRUE,  OPT_PAGE_DISTURBANCE, 0, 23,
	"disturb_state",                _("自分のステータス変化時に行動を中止する", "Disturb whenever player state changes") },

	{ &disturb_minor,               TRUE,  OPT_PAGE_DISTURBANCE, 0, 24,
	"disturb_minor",                _("些細なことが起きても行動を中止する", "Disturb whenever boring things happen") },

	{ &ring_bell,                   FALSE, OPT_PAGE_DISTURBANCE, 0, 14,
	"ring_bell",                    _("エラー時にビープ音を鳴らす", "Audible bell (on errors, etc)") },

	{ &disturb_trap_detect,         TRUE,  OPT_PAGE_DISTURBANCE, 0, 27,
	"disturb_trap_detect",          _("トラップ感知範囲外に出る直前に行動を中止する", "Disturb when leaving trap detected area") },

	{ &alert_trap_detect,           FALSE, OPT_PAGE_DISTURBANCE, 0, 25,
	"alert_trap_detect",            _("トラップ感知範囲外に出る直前に警告する", "Alert when leaving trap detected area") },

/*** Birth Options ***/
	{ &manual_haggle,               FALSE, OPT_PAGE_BIRTH, 1, 0,
	"manual_haggle",                _("店で値切り交渉をする", "Manually haggle in stores") },

	{ &easy_band,                   FALSE, OPT_PAGE_BIRTH, 6, 31,
	"easy_band",                    _("初心者用簡単モード(*)", "Easy Mode (*)") },

	{ &smart_learn,                 TRUE,  OPT_PAGE_BIRTH, 1, 14,
	"smart_learn",                  _("モンスターは失敗を学習する(*)", "Monsters learn from their mistakes (*)") },

	{ &smart_cheat,                 FALSE, OPT_PAGE_BIRTH, 1, 15,
	"smart_cheat",                  _("モンスターはプレイヤーの弱みを突く(*)", "Monsters exploit players weaknesses (*)") },

	{ &vanilla_town,                FALSE, OPT_PAGE_BIRTH, 6, 0,
	"vanilla_town",                 _("元祖の街/クエストと荒野なし", "Use 'vanilla' town without quests and wilderness") },

	{ &lite_town,                   FALSE, OPT_PAGE_BIRTH, 6, 1,
	"lite_town",                    _("小規模な街/荒野なし", "Use 'lite' town without a wilderness") },

	{ &ironman_shops,               FALSE, OPT_PAGE_BIRTH, 6, 2,
	"ironman_shops",                _("(鉄人用)店を使用しない(*)", "Stores are permanently closed (*)") },

	{ &ironman_small_levels,        FALSE, OPT_PAGE_BIRTH, 6, 3,
	"ironman_small_levels",         _("(鉄人用)常に非常に小さいフロアを生成(*)", "Always create unusually small dungeon levels (*)") },

	{ &ironman_downward,            FALSE, OPT_PAGE_BIRTH, 6, 4,
	"ironman_downward",             _("(鉄人用)帰還と上り階段なし(*)", "Disable recall and use of up stairs (*)") },

	{ &ironman_empty_levels,        FALSE, OPT_PAGE_BIRTH, 6, 8,
	"ironman_empty_levels",         _("(鉄人用)常に空っぽのアリーナレベルを生成(*)", "Always create empty 'arena' levels (*)") },

	{ &ironman_rooms,               FALSE, OPT_PAGE_BIRTH, 6, 12,
	"ironman_rooms",                _("(鉄人用)常に普通でない部屋を生成する(*)", "Always generate very unusual rooms (*)") },

	{ &ironman_nightmare,           FALSE, OPT_PAGE_BIRTH, 6, 18,
	"ironman_nightmare",            _("(鉄人用)悪夢モード(これは全く不条理です！)(*)", "Nightmare mode(it isn't even remotely fair!)(*)") },

	{ &left_hander,                 FALSE, OPT_PAGE_BIRTH, 6, 13,
	"left_hander",                  _("左利きである", "Left-Hander") },

	{ &preserve_mode,               TRUE,  OPT_PAGE_BIRTH, 6, 14,
	"preserve_mode",                _("伝説のアイテムを取り逃しても再生成される(*)", "Preserve artifacts (*)") },

	{ &autoroller,                  TRUE,  OPT_PAGE_BIRTH, 6, 15,
	"autoroller",                   _("能力値にオートローラー使用(*)", "Allow use of autoroller for stats (*)") },

	{ &autochara,                   FALSE, OPT_PAGE_BIRTH, 6, 16,
	"autochara",                   _("体格/地位にオートローラー使用", "Autoroll for weight, height and social status") },

	{ &powerup_home,                TRUE,  OPT_PAGE_BIRTH, 4, 3,
	"powerup_home",                 _("我が家を拡張する(*)", "Increase capacity of your home (*)") },

/*** Easy Object Auto-Destroyer ***/

	{ &destroy_items,               FALSE, OPT_PAGE_AUTODESTROY, 7, 0,
	"destroy_items",                _("アイテムの簡易自動破壊を使用する", "Use easy auto-destroyer") },

	{ &destroy_feeling,             FALSE, OPT_PAGE_AUTODESTROY, 7, 8,
	"destroy_feeling",              _("簡易鑑定したとき自動破壊を適用する", "Apply auto-destroy as sense feeling") },

	{ &destroy_identify,            FALSE, OPT_PAGE_AUTODESTROY, 7, 9,
	"destroy_identify",             _("鑑定したとき自動破壊を適用する", "Apply auto-destroy as identify an item") },

	{ &leave_worth,                 TRUE,  OPT_PAGE_AUTODESTROY, 7, 2,
	"leave_worth",                  _("価値があるアイテムは壊さない", "Auto-destroyer leaves known worthy items") },

	{ &leave_equip,                 FALSE, OPT_PAGE_AUTODESTROY, 7, 3,
	"leave_equip",                  _("武器/防具は壊さない", "Auto-destroyer leaves weapons and armour") },

	{ &leave_chest,                 TRUE,  OPT_PAGE_AUTODESTROY, 7, 7,
	"leave_chest",                  _("開封されていない箱は壊さない", "Auto-destroyer leaves closed chests") },

	{ &leave_wanted,                TRUE,  OPT_PAGE_AUTODESTROY, 7, 4,
	"leave_wanted",                 _("賞金首の死体/骨は壊さない", "Auto-destroyer leaves wanted corpses") },

	{ &leave_corpse,                FALSE, OPT_PAGE_AUTODESTROY, 7, 5,
	"leave_corpse",                 _("死体/骨は壊さない", "Auto-destroyer leaves corpses and skeletons") },

	{ &leave_junk,                  FALSE, OPT_PAGE_AUTODESTROY, 7, 6,
	"leave_junk",                   _("がらくたは壊さない", "Auto-destroyer leaves junk") },

	{ &leave_special,               TRUE,  OPT_PAGE_AUTODESTROY, 7, 1,
	"leave_special",                _("種族/職業で特別に必要なアイテムは壊さない", "Auto-destroyer leaves items your race/class needs") },

/*** Play-record Options ***/

	{ &record_fix_art,              TRUE,  OPT_PAGE_PLAYRECORD, 4, 11,
	"record_fix_art",               _("固定アーティファクトの入手を記録する", "Record fixed artifacts") },

	{ &record_rand_art,             TRUE,  OPT_PAGE_PLAYRECORD, 4, 12,
	"record_rand_art",              _("ランダムアーティファクトの入手を記録する", "Record random artifacts") },

	{ &record_destroy_uniq,         TRUE,  OPT_PAGE_PLAYRECORD, 4, 13,
	"record_destroy_uniq",          _("ユニークモンスターを倒したときを記録する", "Record when destroy unique monster") },

	{ &record_fix_quest,            TRUE,  OPT_PAGE_PLAYRECORD, 4, 14,
	"record_fix_quest",             _("固定クエストの達成を記録する", "Record fixed quests") },

	{ &record_rand_quest,           TRUE,  OPT_PAGE_PLAYRECORD, 4, 15,
	"record_rand_quest",            _("ランダムクエストの達成を記録する", "Record random quests") },

	{ &record_maxdepth,             TRUE,  OPT_PAGE_PLAYRECORD, 4, 16,
	"record_maxdepth",              _("最深階を更新したときに記録する", "Record movements to deepest level") },

	{ &record_stair,                TRUE,  OPT_PAGE_PLAYRECORD, 4, 17,
	"record_stair",                 _("階の移動を記録する", "Record recall and stair movements") },

	{ &record_buy,                  TRUE,  OPT_PAGE_PLAYRECORD, 4, 18,
	"record_buy",                   _("アイテムの購入を記録する", "Record purchased items") },

	{ &record_sell,                 FALSE, OPT_PAGE_PLAYRECORD, 4, 19,
	"record_sell",                  _("アイテムの売却を記録する", "Record sold items") },

	{ &record_danger,               TRUE,  OPT_PAGE_PLAYRECORD, 4, 20,
	"record_danger",                _("ピンチになったときを記録する", "Record hitpoint warning") },

	{ &record_arena,                TRUE,  OPT_PAGE_PLAYRECORD, 4, 21,
	"record_arena",                 _("アリーナでの勝利を記録する", "Record arena victories") },

	{ &record_ident,                TRUE,  OPT_PAGE_PLAYRECORD, 4, 22,
	"record_ident",                 _("未判明のアイテムの識別を記録する", "Record first identified items") },

	{ &record_named_pet,            FALSE, OPT_PAGE_PLAYRECORD, 4, 23,
	"record_named_pet",             _("名前つきペットの情報を記録する", "Record information about named pets") },

/*** End of Table ***/

	{ NULL,                         0, 0, 0, 0,
	NULL,                           NULL }
};

/*!
 * チートオプションの定義テーブル / Cheating options
 */
const option_type cheat_info[CHEAT_MAX] =
{
	{ &cheat_peek,		FALSE,	255,	0x01, 0x00,
	"cheat_peek",		_("アイテムの生成をのぞき見る", "Peek into object creation")
	},

	{ &cheat_hear,		FALSE,	255,	0x02, 0x00,
	"cheat_hear",		_("モンスターの生成をのぞき見る", "Peek into monster creation")
	},

	{ &cheat_room,		FALSE,	255,	0x04, 0x00,
	"cheat_room",		_("ダンジョンの生成をのぞき見る", "Peek into dungeon creation")
	},

	{ &cheat_xtra,		FALSE,	255,	0x08, 0x00,
	"cheat_xtra",		_("その他の事をのぞき見る", "Peek into something else")
	},

	{ &cheat_know,		FALSE,	255,	0x10, 0x00,
	"cheat_know",		_("完全なモンスターの思い出を知る", "Know complete monster info")
	},

	{ &cheat_live,		FALSE,	255,	0x20, 0x00,
	"cheat_live",		_("死を回避することを可能にする", "Allow player to avoid death")
	},

	{ &cheat_save,		FALSE,	255,	0x40, 0x00,
	"cheat_save",		_("死んだ時セーブするか確認する", "Ask for saving death")
	},

	{ &cheat_diary_output,	FALSE,	255,	0x80, 0x00,
	"cheat_diary_output",	_("ウィザードログを日記に出力する", "Output wizard log to diary.")
	},

	{ &cheat_turn,	FALSE,	255,	0x81, 0x00,
	"cheat_turn",	_("ゲームメッセージにターン表示を行う", "Put turn in game messages.")
	},

	{ &cheat_sight,	FALSE,	255,	0x82, 0x00,
	"cheat_sight",	_("「見る」コマンドを拡張する。", "Expand \"L\"ook command.")
	}
};


/*!
* 自動セーブオプションテーブル
*/
const option_type autosave_info[2] =
{
	{ &autosave_l,      FALSE, 255, 0x01, 0x00,
	"autosave_l",    _("新しい階に入る度に自動セーブする", "Autosave when entering new levels") },

	{ &autosave_t,      FALSE, 255, 0x02, 0x00,
	"autosave_t",   _("一定ターン毎に自動セーブする", "Timed autosave") },
};


#define OPT_NUM 15

typedef struct
{
	char key;
	concptr name;
	int row;
} opts;

opts option_fields[OPT_NUM] =
{
	{ '1', _("    キー入力     オプション", "Input Options"), 3 },
	{ '2', _("   マップ画面    オプション", "Map Screen Options"), 4 },
	{ '3', _("  テキスト表示   オプション", "Text Display Options"), 5 },
	{ '4', _("  ゲームプレイ   オプション", "Game-Play Options"), 6 },
	{ '5', _("  行動中止関係   オプション", "Disturbance Options"), 7 },
	{ '6', _("  簡易自動破壊   オプション", "Easy Auto-Destroyer Options"), 8 },
	{ 'r', _("   プレイ記録    オプション", "Play record Options"), 9 },

	{ 'p', _("自動拾いエディタ", "Auto-picker/destroyer editor"), 11 },
	{ 'd', _(" 基本ウェイト量 ", "Base Delay Factor"), 12 },
	{ 'h', _("低ヒットポイント", "Hitpoint Warning"), 13 },
	{ 'm', _("  低魔力色閾値  ", "Mana Color Threshold"), 14 },
	{ 'a', _("   自動セーブ    オプション", "Autosave Options"), 15 },
	{ 'w', _("ウインドウフラグ", "Window Flags"), 16 },

	{ 'b', _("      初期       オプション (参照のみ)", "Birth Options (Browse Only)"), 18 },
	{ 'c', _("      詐欺       オプション", "Cheat Options"), 19 },
};

/*!
 * @brief セーブ頻度ターンの次の値を返す
 * @param current 現在のセーブ頻度ターン値
 * @return 次のセーブ頻度ターン値
 */
static s16b toggle_frequency(s16b current)
{
	switch (current)
	{
	case 0: return 50;
	case 50: return 100;
	case 100: return 250;
	case 250: return 500;
	case 500: return 1000;
	case 1000: return 2500;
	case 2500: return 5000;
	case 5000: return 10000;
	case 10000: return 25000;
	default: return 0;
	}
}

/*!
 * @brief 自動セーブオプションを変更するコマンドのメインルーチン
 * @param info 表示メッセージ
 * @return なし
 */
static void do_cmd_options_autosave(concptr info)
{
	char ch;
	int i, k = 0, n = 2;
	char buf[80];

	Term_clear();

	/* Interact with the player */
	while (TRUE)
	{
		/* Prompt */
		sprintf(buf, _("%s ( リターンで次へ, y/n でセット, F で頻度を入力, ESC で決定 ) ",
			"%s (RET to advance, y/n to set, 'F' for frequency, ESC to accept) "), info);

		prt(buf, 0, 0);

		/* Display the options */
		for (i = 0; i < n; i++)
		{
			byte a = TERM_WHITE;

			/* Color current option */
			if (i == k) a = TERM_L_BLUE;

			/* Display the option text */
			sprintf(buf, "%-48s: %s (%s)",
				autosave_info[i].o_desc,
				(*autosave_info[i].o_var ? _("はい  ", "yes") : _("いいえ", "no ")),
				autosave_info[i].o_text);
			c_prt(a, buf, i + 2, 0);
		}
		prt(format(_("自動セーブの頻度： %d ターン毎", "Timed autosave frequency: every %d turns"), autosave_freq), 5, 0);

		/* Hilite current option */
		move_cursor(k + 2, 50);

		/* Get a key */
		ch = inkey();

		/* Analyze */
		switch (ch)
		{
		case ESCAPE:
		{
			return;
		}

		case '-':
		case '8':
		{
			k = (n + k - 1) % n;
			break;
		}

		case ' ':
		case '\n':
		case '\r':
		case '2':
		{
			k = (k + 1) % n;
			break;
		}

		case 'y':
		case 'Y':
		case '6':
		{

			(*autosave_info[k].o_var) = TRUE;
			k = (k + 1) % n;
			break;
		}

		case 'n':
		case 'N':
		case '4':
		{
			(*autosave_info[k].o_var) = FALSE;
			k = (k + 1) % n;
			break;
		}

		case 'f':
		case 'F':
		{
			autosave_freq = toggle_frequency(autosave_freq);
			prt(format(_("自動セーブの頻度： %d ターン毎", "Timed autosave frequency: every %d turns"), autosave_freq), 5, 0);
			break;
		}

		case '?':
		{
			(void)show_file(p_ptr, TRUE, _("joption.txt#Autosave", "option.txt#Autosave"), NULL, 0, 0);
			Term_clear();
			break;
		}

		default:
		{
			bell();
			break;
		}
		}
	}
}


/*!
 * @brief ウィンドウオプションを変更するコマンドのメインルーチン /
 * Modify the "window" options
 * @return なし
 */
static void do_cmd_options_win(void)
{
	int i, j, d;
	TERM_LEN y = 0;
	TERM_LEN x = 0;
	char ch;
	bool go = TRUE;
	u32b old_flag[8];


	/* Memorize old flags */
	for (j = 0; j < 8; j++)
	{
		/* Acquire current flags */
		old_flag[j] = window_flag[j];
	}

	Term_clear();

	/* Interact */
	while (go)
	{
		/* Prompt */
		prt(_("ウィンドウ・フラグ (<方向>で移動, tでチェンジ, y/n でセット, ESC)", "Window Flags (<dir>, t, y, n, ESC) "), 0, 0);

		/* Display the windows */
		for (j = 0; j < 8; j++)
		{
			byte a = TERM_WHITE;

			concptr s = angband_term_name[j];

			/* Use color */
			if (j == x) a = TERM_L_BLUE;

			/* Window name, staggered, centered */
			Term_putstr(35 + j * 5 - strlen(s) / 2, 2 + j % 2, -1, a, s);
		}

		/* Display the options */
		for (i = 0; i < 16; i++)
		{
			byte a = TERM_WHITE;

			concptr str = window_flag_desc[i];

			/* Use color */
			if (i == y) a = TERM_L_BLUE;

			/* Unused option */
			if (!str) str = _("(未使用)", "(Unused option)");

			/* Flag name */
			Term_putstr(0, i + 5, -1, a, str);

			/* Display the windows */
			for (j = 0; j < 8; j++)
			{
				char c = '.';
				a = TERM_WHITE;

				/* Use color */
				if ((i == y) && (j == x)) a = TERM_L_BLUE;

				/* Active flag */
				if (window_flag[j] & (1L << i)) c = 'X';

				/* Flag value */
				Term_putch(35 + j * 5, i + 5, a, c);
			}
		}

		/* Place Cursor */
		Term_gotoxy(35 + x * 5, y + 5);

		/* Get key */
		ch = inkey();

		/* Analyze */
		switch (ch)
		{
		case ESCAPE:
		{
			go = FALSE;
			break;
		}

		case 'T':
		case 't':
		{
			/* Clear windows */
			for (j = 0; j < 8; j++)
			{
				window_flag[j] &= ~(1L << y);
			}

			/* Clear flags */
			for (i = 0; i < 16; i++)
			{
				window_flag[x] &= ~(1L << i);
			}
		}
			/* Fall through */

		case 'y':
		case 'Y':
		{
			/* Ignore screen */
			if (x == 0) break;

			/* Set flag */
			window_flag[x] |= (1L << y);
			break;
		}

		case 'n':
		case 'N':
		{
			/* Clear flag */
			window_flag[x] &= ~(1L << y);
			break;
		}

		case '?':
		{
			(void)show_file(p_ptr, TRUE, _("joption.txt#Window", "option.txt#Window"), NULL, 0, 0);

			Term_clear();
			break;
		}

		default:
		{
			d = get_keymap_dir(ch);

			x = (x + ddx[d] + 8) % 8;
			y = (y + ddy[d] + 16) % 16;

			if (!d) bell();
		}
		}
	}

	/* Notice changes */
	for (j = 0; j < 8; j++)
	{
		term *old = Term;

		/* Dead window */
		if (!angband_term[j]) continue;

		/* Ignore non-changes */
		if (window_flag[j] == old_flag[j]) continue;

		/* Activate */
		Term_activate(angband_term[j]);
		Term_clear();
		Term_fresh();
		Term_activate(old);
	}
}


/*!
 * @brief チートオプションを変更するコマンドのメインルーチン
 * Interact with some options for cheating
 * @param info 表示メッセージ
 * @return なし
 */
static void do_cmd_options_cheat(concptr info)
{
	char	ch;
	int		i, k = 0, n = CHEAT_MAX;
	char	buf[80];
	Term_clear();

	/* Interact with the player */
	while (TRUE)
	{
		DIRECTION dir;

		/* Prompt */
		sprintf(buf, _("%s ( リターンで次へ, y/n でセット, ESC で決定 )", "%s (RET to advance, y/n to set, ESC to accept) "), info);

		prt(buf, 0, 0);

#ifdef JP
		/* 詐欺オプションをうっかりいじってしまう人がいるようなので注意 */
		prt("                                 <<  注意  >>", 11, 0);
		prt("      詐欺オプションを一度でも設定すると、スコア記録が残らなくなります！", 12, 0);
		prt("      後に解除してもダメですので、勝利者を目指す方はここのオプションはい", 13, 0);
		prt("      じらないようにして下さい。", 14, 0);
#endif
		/* Display the options */
		for (i = 0; i < n; i++)
		{
			byte a = TERM_WHITE;

			/* Color current option */
			if (i == k) a = TERM_L_BLUE;

			/* Display the option text */
			sprintf(buf, "%-48s: %s (%s)",
				cheat_info[i].o_desc,
				(*cheat_info[i].o_var ? _("はい  ", "yes") : _("いいえ", "no ")),
				cheat_info[i].o_text);
			c_prt(a, buf, i + 2, 0);
		}

		/* Hilite current option */
		move_cursor(k + 2, 50);

		/* Get a key */
		ch = inkey();

		/*
		 * HACK - Try to translate the key into a direction
		 * to allow using the roguelike keys for navigation.
		 */
		dir = get_keymap_dir(ch);
		if ((dir == 2) || (dir == 4) || (dir == 6) || (dir == 8))
			ch = I2D(dir);

		/* Analyze */
		switch (ch)
		{
		case ESCAPE:
		{
			return;
		}

		case '-':
		case '8':
		{
			k = (n + k - 1) % n;
			break;
		}

		case ' ':
		case '\n':
		case '\r':
		case '2':
		{
			k = (k + 1) % n;
			break;
		}

		case 'y':
		case 'Y':
		case '6':
		{
			if (!current_world_ptr->noscore)
				exe_write_diary(p_ptr, DIARY_DESCRIPTION, 0,
					_("詐欺オプションをONにして、スコアを残せなくなった。", "gave up sending score to use cheating options."));
			current_world_ptr->noscore |= (cheat_info[k].o_set * 256 + cheat_info[k].o_bit);
			(*cheat_info[k].o_var) = TRUE;
			k = (k + 1) % n;
			break;
		}

		case 'n':
		case 'N':
		case '4':
		{
			(*cheat_info[k].o_var) = FALSE;
			k = (k + 1) % n;
			break;
		}

		case '?':
		{
			strnfmt(buf, sizeof(buf), _("joption.txt#%s", "option.txt#%s"), cheat_info[k].o_text);
			/* Peruse the help file */
			(void)show_file(p_ptr, TRUE, buf, NULL, 0, 0);

			Term_clear();
			break;
		}

		default:
		{
			bell();
			break;
		}
		}
	}
}


/*!
 * @brief ビットセットからゲームオプションを展開する / Extract option variables from bit sets
 * @return なし
 */
void extract_option_vars(void)
{
	int i;

	for (i = 0; option_info[i].o_desc; i++)
	{
		int os = option_info[i].o_set;
		int ob = option_info[i].o_bit;

		/* Set the "default" options */
		if (option_info[i].o_var)
		{
			/* Set */
			if (option_flag[os] & (1L << ob))
			{
				/* Set */
				(*option_info[i].o_var) = TRUE;
			}
			else
			{
				(*option_info[i].o_var) = FALSE;
			}
		}
	}
}


/*!
 * @brief 標準オプションを変更するコマンドのメインルーチン /
 * Set or unset various options.
 * @return なし
 * @details
 * <pre>
 * The user must use the "Ctrl-R" command to "adapt" to changes
 * in any options which control "visual" aspects of the game.
 * </pre>
 */
void do_cmd_options(void)
{
	char k;
	int d, skey;
	TERM_LEN i, y = 0;
	screen_save();

	/* Interact */
	while (TRUE)
	{
		int n = OPT_NUM;

		/* Does not list cheat option when cheat option is off */
		if (!current_world_ptr->noscore && !allow_debug_opts) n--;
		Term_clear();

		/* Why are we here */
		prt(_("[ オプションの設定 ]", "Game options"), 1, 0);

		while (TRUE)
		{
			/* Give some choices */
			for (i = 0; i < n; i++)
			{
				byte a = TERM_WHITE;
				if (i == y) a = TERM_L_BLUE;
				Term_putstr(5, option_fields[i].row, -1, a,
					format("(%c) %s", toupper(option_fields[i].key), option_fields[i].name));
			}

			prt(_("<方向>で移動, Enterで決定, ESCでキャンセル, ?でヘルプ: ", "Move to <dir>, Select to Enter, Cancel to ESC, ? to help: "), 21, 0);

			/* Get command */
			skey = inkey_special(TRUE);
			if (!(skey & SKEY_MASK)) k = (char)skey;
			else k = 0;

			if (k == ESCAPE) break;

			if (my_strchr("\n\r ", k))
			{
				k = option_fields[y].key;
				break;
			}

			for (i = 0; i < n; i++)
			{
				if (tolower(k) == option_fields[i].key) break;
			}

			/* Command is found */
			if (i < n) break;

			/* Hack -- browse help */
			if (k == '?') break;

			/* Move cursor */
			d = 0;
			if (skey == SKEY_UP) d = 8;
			if (skey == SKEY_DOWN) d = 2;
			y = (y + ddy[d] + n) % n;
			if (!d) bell();
		}

		if (k == ESCAPE) break;

		/* Analyze */
		switch (k)
		{
		case '1':
		{
			/* Process the general options */
			do_cmd_options_aux(OPT_PAGE_INPUT, _("キー入力オプション", "Input Options"));
			break;
		}

		case '2':
		{
			/* Process the general options */
			do_cmd_options_aux(OPT_PAGE_MAPSCREEN, _("マップ画面オプション", "Map Screen Options"));
			break;
		}

		case '3':
		{
			/* Spawn */
			do_cmd_options_aux(OPT_PAGE_TEXT, _("テキスト表示オプション", "Text Display Options"));
			break;
		}

		case '4':
		{
			/* Spawn */
			do_cmd_options_aux(OPT_PAGE_GAMEPLAY, _("ゲームプレイ・オプション", "Game-Play Options"));
			break;
		}

		case '5':
		{
			/* Spawn */
			do_cmd_options_aux(OPT_PAGE_DISTURBANCE, _("行動中止関係のオプション", "Disturbance Options"));
			break;
		}

		case '6':
		{
			/* Spawn */
			do_cmd_options_aux(OPT_PAGE_AUTODESTROY, _("簡易自動破壊オプション", "Easy Auto-Destroyer Options"));
			break;
		}

		/* Play-record Options */
		case 'R':
		case 'r':
		{
			/* Spawn */
			do_cmd_options_aux(OPT_PAGE_PLAYRECORD, _("プレイ記録オプション", "Play-record Options"));
			break;
		}

		/* Birth Options */
		case 'B':
		case 'b':
		{
			/* Spawn */
			do_cmd_options_aux(OPT_PAGE_BIRTH, (!current_world_ptr->wizard || !allow_debug_opts) ?
				_("初期オプション(参照のみ)", "Birth Options(browse only)") :
				_("初期オプション((*)はスコアに影響)", "Birth Options((*)s effect score)"));
			break;
		}

		/* Cheating Options */
		case 'C':
		{
			if (!current_world_ptr->noscore && !allow_debug_opts)
			{
				/* Cheat options are not permitted */
				bell();
				break;
			}

			/* Spawn */
			do_cmd_options_cheat(_("詐欺師は決して勝利できない！", "Cheaters never win"));
			break;
		}

		case 'a':
		case 'A':
		{
			do_cmd_options_autosave(_("自動セーブ", "Autosave"));
			break;
		}

		/* Window flags */
		case 'W':
		case 'w':
		{
			/* Spawn */
			do_cmd_options_win();
			p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_SPELL |
				PW_PLAYER | PW_MESSAGE | PW_OVERHEAD |
				PW_MONSTER | PW_OBJECT | PW_SNAPSHOT |
				PW_DUNGEON | PW_MONSTER_LIST);
			break;
		}

		/* Auto-picker/destroyer editor */
		case 'P':
		case 'p':
		{
			do_cmd_edit_autopick(p_ptr);
			break;
		}

		/* Hack -- Delay Speed */
		case 'D':
		case 'd':
		{
			/* Prompt */
			clear_from(18);
			prt(_("コマンド: 基本ウェイト量", "Command: Base Delay Factor"), 19, 0);

			/* Get a new value */
			while (TRUE)
			{
				int msec = delay_factor * delay_factor * delay_factor;
				prt(format(_("現在のウェイト: %d (%dミリ秒)", "Current base delay factor: %d (%d msec)"), delay_factor, msec), 22, 0);
				prt(_("ウェイト (0-9) ESCで決定: ", "Delay Factor (0-9 or ESC to accept): "), 20, 0);
				k = inkey();
				if (k == ESCAPE) break;
				else if (k == '?')
				{
					(void)show_file(p_ptr, TRUE, _("joption.txt#BaseDelay", "option.txt#BaseDelay"), NULL, 0, 0);
					Term_clear();
				}
				else if (isdigit(k)) delay_factor = D2I(k);
				else bell();
			}

			break;
		}

		/* Hack -- hitpoint warning factor */
		case 'H':
		case 'h':
		{
			/* Prompt */
			clear_from(18);
			prt(_("コマンド: 低ヒットポイント警告", "Command: Hitpoint Warning"), 19, 0);

			/* Get a new value */
			while (TRUE)
			{
				prt(format(_("現在の低ヒットポイント警告: %d0%%", "Current hitpoint warning: %d0%%"), hitpoint_warn), 22, 0);
				prt(_("低ヒットポイント警告 (0-9) ESCで決定: ", "Hitpoint Warning (0-9 or ESC to accept): "), 20, 0);
				k = inkey();
				if (k == ESCAPE) break;
				else if (k == '?')
				{
					(void)show_file(p_ptr, TRUE, _("joption.txt#Hitpoint", "option.txt#Hitpoint"), NULL, 0, 0);
					Term_clear();
				}
				else if (isdigit(k)) hitpoint_warn = D2I(k);
				else bell();
			}

			break;
		}

		/* Hack -- mana color factor */
		case 'M':
		case 'm':
		{
			/* Prompt */
			clear_from(18);
			prt(_("コマンド: 低魔力色閾値", "Command: Mana Color Threshold"), 19, 0);

			/* Get a new value */
			while (TRUE)
			{
				prt(format(_("現在の低魔力色閾値: %d0%%", "Current mana color threshold: %d0%%"), mana_warn), 22, 0);
				prt(_("低魔力閾値 (0-9) ESCで決定: ", "Mana color Threshold (0-9 or ESC to accept): "), 20, 0);
				k = inkey();
				if (k == ESCAPE) break;
				else if (k == '?')
				{
					(void)show_file(p_ptr, TRUE, _("joption.txt#Manapoint", "option.txt#Manapoint"), NULL, 0, 0);
					Term_clear();
				}
				else if (isdigit(k)) mana_warn = D2I(k);
				else bell();
			}

			break;
		}

		case '?':
			(void)show_file(p_ptr, TRUE, _("joption.txt", "option.txt"), NULL, 0, 0);
			Term_clear();
			break;

			/* Unknown option */
		default:
		{
			bell();
			break;
		}
		}

		msg_erase();
	}

	screen_load();

	/* Hack - Redraw equippy chars */
	p_ptr->redraw |= (PR_EQUIPPY);
}


/*!
 * @brief 標準オプションを変更するコマンドのサブルーチン /
 * Interact with some options
 * @param page オプションページ番号
 * @param info 表示メッセージ
 * @return なし
 */
void do_cmd_options_aux(int page, concptr info)
{
	char    ch;
	int     i, k = 0, n = 0, l;
	int     opt[24];
	char    buf[80];
	bool    browse_only = (page == OPT_PAGE_BIRTH) && current_world_ptr->character_generated &&
		(!current_world_ptr->wizard || !allow_debug_opts);


	/* Lookup the options */
	for (i = 0; i < 24; i++) opt[i] = 0;

	/* Scan the options */
	for (i = 0; option_info[i].o_desc; i++)
	{
		/* Notice options on this "page" */
		if (option_info[i].o_page == page) opt[n++] = i;
	}
	Term_clear();

	/* Interact with the player */
	while (TRUE)
	{
		DIRECTION dir;

		/* Prompt */
		sprintf(buf, _("%s (リターン:次, %sESC:終了, ?:ヘルプ) ", "%s (RET:next, %s, ?:help) "),
			info, browse_only ? _("", "ESC:exit") : _("y/n:変更, ", "y/n:change, ESC:accept"));
		prt(buf, 0, 0);

		/* HACK -- description for easy-auto-destroy options */
		if (page == OPT_PAGE_AUTODESTROY)
			c_prt(TERM_YELLOW, _("以下のオプションは、簡易自動破壊を使用するときのみ有効",
				"Following options will protect items from easy auto-destroyer."), 6, _(6, 3));

		/* Display the options */
		for (i = 0; i < n; i++)
		{
			byte a = TERM_WHITE;

			/* Color current option */
			if (i == k) a = TERM_L_BLUE;

			/* Display the option text */
			sprintf(buf, "%-48s: %s (%.19s)",
				option_info[opt[i]].o_desc,
				(*option_info[opt[i]].o_var ? _("はい  ", "yes") : _("いいえ", "no ")),
				option_info[opt[i]].o_text);
			if ((page == OPT_PAGE_AUTODESTROY) && i > 2) c_prt(a, buf, i + 5, 0);
			else c_prt(a, buf, i + 2, 0);
		}

		if ((page == OPT_PAGE_AUTODESTROY) && (k > 2)) l = 3;
		else l = 0;

		/* Hilite current option */
		move_cursor(k + 2 + l, 50);

		/* Get a key */
		ch = inkey();

		/*
		 * HACK - Try to translate the key into a direction
		 * to allow using the roguelike keys for navigation.
		 */
		dir = get_keymap_dir(ch);
		if ((dir == 2) || (dir == 4) || (dir == 6) || (dir == 8))
			ch = I2D(dir);

		/* Analyze */
		switch (ch)
		{
		case ESCAPE:
		{
			return;
		}

		case '-':
		case '8':
		{
			k = (n + k - 1) % n;
			break;
		}

		case ' ':
		case '\n':
		case '\r':
		case '2':
		{
			k = (k + 1) % n;
			break;
		}

		case 'y':
		case 'Y':
		case '6':
		{
			if (browse_only) break;
			(*option_info[opt[k]].o_var) = TRUE;
			k = (k + 1) % n;
			break;
		}

		case 'n':
		case 'N':
		case '4':
		{
			if (browse_only) break;
			(*option_info[opt[k]].o_var) = FALSE;
			k = (k + 1) % n;
			break;
		}

		case 't':
		case 'T':
		{
			if (!browse_only) (*option_info[opt[k]].o_var) = !(*option_info[opt[k]].o_var);
			break;
		}

		case '?':
		{
			strnfmt(buf, sizeof(buf), _("joption.txt#%s", "option.txt#%s"), option_info[opt[k]].o_text);
			/* Peruse the help file */
			(void)show_file(p_ptr, TRUE, buf, NULL, 0, 0);

			Term_clear();
			break;
		}

		default:
		{
			bell();
			break;
		}
		}
	}
}
