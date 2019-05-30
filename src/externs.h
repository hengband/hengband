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

/* tables.c */

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

/* variable.c */

extern bool repair_monsters;
extern bool repair_objects;



/*
 * Software options (set via the '=' command).  See "tables.c"
 */

extern int player_uid;
extern int player_euid;
extern int player_egid;
extern pos_list tmp_pos;
extern STR_OFFSET quark__num;
extern concptr *quark__str;

extern term *angband_term[8];

extern player_type *p_ptr;

extern MONSTER_IDX pet_t_m_idx;
extern MONSTER_IDX riding_t_m_idx;
extern MONRACE_IDX today_mon;
extern bool sukekaku;
extern bool new_mane;

extern DEPTH *max_dlv;
extern bool use_menu;

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
