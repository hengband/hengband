#pragma once
#include "feature.h"

extern void get_screen_size(TERM_LEN *wid_p, TERM_LEN *hgt_p);
extern int panel_col_of(int col);
extern void apply_default_feat_lighting(TERM_COLOR f_attr[F_LIT_MAX], SYMBOL_CODE f_char[F_LIT_MAX]);
extern void prt_map(void);
extern void map_info(POSITION y, POSITION x, TERM_COLOR *ap, SYMBOL_CODE *cp, TERM_COLOR *tap, SYMBOL_CODE *tcp);
extern void display_map(int *cy, int *cx);
extern void do_cmd_view_map(void);

extern void health_track(MONSTER_IDX m_idx);
extern void prt_time(void);
extern concptr map_name(void);
extern void print_monster_list(TERM_LEN x, TERM_LEN y, TERM_LEN max_lines);
extern void move_cursor_relative(int row, int col);
extern void prt_path(POSITION y, POSITION x);
extern void monster_race_track(MONRACE_IDX r_idx);
extern void object_kind_track(KIND_OBJECT_IDX k_idx);
extern void resize_map(void);
extern void redraw_window(void);
extern bool change_panel(POSITION dy, POSITION dx);

extern void window_stuff(void);

/*
 * Bit flags for the "window" variable (etc)
 */
#define PW_INVEN        0x00000001L     /*!<サブウィンドウ描画フラグ: 所持品-装備品 / Display inven/equip */
#define PW_EQUIP        0x00000002L     /*!<サブウィンドウ描画フラグ: 装備品-所持品 / Display equip/inven */
#define PW_SPELL        0x00000004L     /*!<サブウィンドウ描画フラグ: 魔法一覧 / Display spell list */
#define PW_PLAYER       0x00000008L     /*!<サブウィンドウ描画フラグ: プレイヤーのステータス / Display character */
#define PW_MONSTER_LIST 0x00000010L     /*!<サブウィンドウ描画フラグ: 視界内モンスターの一覧 / Display monster list */
 /* xxx */
 /* xxx */
#define PW_MESSAGE      0x00000040L     /*!<サブウィンドウ描画フラグ: メッセージログ / Display messages */
#define PW_OVERHEAD     0x00000080L     /*!<サブウィンドウ描画フラグ: 周辺の光景 / Display overhead view */
#define PW_MONSTER      0x00000100L     /*!<サブウィンドウ描画フラグ: モンスターの思い出 / Display monster recall */
#define PW_OBJECT       0x00000200L     /*!<サブウィンドウ描画フラグ: アイテムの知識 / Display object recall */
#define PW_DUNGEON      0x00000400L     /*!<サブウィンドウ描画フラグ: ダンジョンの地形 / Display dungeon view */
#define PW_SNAPSHOT     0x00000800L     /*!<サブウィンドウ描画フラグ: 記念写真 / Display snap-shot */
/* xxx */
/* xxx */
#define PW_BORG_1       0x00004000L     /*!<サブウィンドウ描画フラグ: ボーグメッセージ / Display borg messages */
#define PW_BORG_2       0x00008000L     /*!<サブウィンドウ描画フラグ: ボーグステータス / Display borg status */


/*
 * Bit flags for the "redraw" variable
 */
#define PR_MISC         0x00000001L     /*!< 再描画フラグ: 種族と職業 / Display Race/Class */
#define PR_TITLE        0x00000002L     /*!< 再描画フラグ: 称号 / Display Title */
#define PR_LEV          0x00000004L     /*!< 再描画フラグ: レベル / Display Level */
#define PR_EXP          0x00000008L     /*!< 再描画フラグ: 経験値 / Display Experience */
#define PR_STATS        0x00000010L     /*!< 再描画フラグ: ステータス /  Display Stats */
#define PR_ARMOR        0x00000020L     /*!< 再描画フラグ: AC / Display Armor */
#define PR_HP           0x00000040L     /*!< 再描画フラグ: HP / Display Hitpoints */
#define PR_MANA         0x00000080L     /*!< 再描画フラグ: MP / Display Mana */
#define PR_GOLD         0x00000100L     /*!< 再描画フラグ: 所持金 / Display Gold */
#define PR_DEPTH        0x00000200L     /*!< 再描画フラグ: ダンジョンの階 / Display Depth */
#define PR_EQUIPPY      0x00000400L     /*!< 再描画フラグ: 装備シンボル / Display equippy chars */
#define PR_HEALTH       0x00000800L     /*!< 再描画フラグ: モンスターのステータス / Display Health Bar */
#define PR_CUT          0x00001000L     /*!< 再描画フラグ: 負傷度 / Display Extra (Cut) */
#define PR_STUN         0x00002000L     /*!< 再描画フラグ: 朦朧度 / Display Extra (Stun) */
#define PR_HUNGER       0x00004000L     /*!< 再描画フラグ: 空腹度 / Display Extra (Hunger) */
#define PR_STATUS       0x00008000L     /*!< 再描画フラグ: プレイヤーの付与状態 /  Display Status Bar */
#define PR_XXX0         0x00010000L     /*!< (unused) */
#define PR_UHEALTH      0x00020000L     /*!< 再描画フラグ: ペットのステータス / Display Uma Health Bar */
#define PR_XXX1         0x00040000L     /*!< (unused) */
#define PR_XXX2         0x00080000L     /*!< (unused) */
#define PR_STATE        0x00100000L     /*!< 再描画フラグ: プレイヤーの行動状態 / Display Extra (State) */
#define PR_SPEED        0x00200000L     /*!< 再描画フラグ: 加速 / Display Extra (Speed) */
#define PR_STUDY        0x00400000L     /*!< 再描画フラグ: 学習 / Display Extra (Study) */
#define PR_IMITATION    0x00800000L     /*!< 再描画フラグ: ものまね / Display Extra (Imitation) */
#define PR_EXTRA        0x01000000L     /*!< 再描画フラグ: 拡張ステータス全体 / Display Extra Info */
#define PR_BASIC        0x02000000L     /*!< 再描画フラグ: 基本ステータス全体 / Display Basic Info */
#define PR_MAP          0x04000000L     /*!< 再描画フラグ: ゲームマップ / Display Map */
#define PR_WIPE         0x08000000L     /*!< 再描画フラグ: 画面消去 / Hack -- Total Redraw */
extern void redraw_stuff(void);

extern POSITION panel_row_min, panel_row_max;
extern POSITION panel_col_min, panel_col_max;
extern POSITION panel_col_prt, panel_row_prt;

/*
 * Determines if a map location is currently "on screen" -RAK-
 * Note that "panel_contains(Y,X)" always implies "in_bounds2(Y,X)".
 */
#define panel_contains(Y,X) \
  (((Y) >= panel_row_min) && ((Y) <= panel_row_max) && \
   ((X) >= panel_col_min) && ((X) <= panel_col_max))

