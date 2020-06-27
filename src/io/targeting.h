#pragma once

#include "system/angband.h"

extern MONSTER_IDX target_who;
extern POSITION target_col;
extern POSITION target_row;

void panel_bounds_center(void);
void verify_panel(player_type *creature_ptr);
bool target_able(player_type *creature_ptr, MONSTER_IDX m_idx);
bool target_okay(player_type *creature_ptr);

/*
 * target_set用関数の利用用途フラグ / Bit flags for the "target_set" function
 */
#define TARGET_KILL     0x01 /*!< モンスターへの狙いをつける(視界内モンスターのみクエリ対象) / Target monsters */
#define TARGET_LOOK     0x02 /*!< "L"ookコマンド向けの既存情報確認向け(全ての有為な情報をクエリ対象) / Describe grid fully */
#define TARGET_XTRA     0x04 /*!< 現在未使用 / Currently unused flag */
#define TARGET_GRID     0x08 /*!< 全てのマス対象にする(現在未使用) / Select from all grids */
bool target_set(player_type *creature_ptr, BIT_FLAGS mode);
void target_set_prepare_look(player_type *creature_ptr);
bool get_aim_dir(player_type *creature_ptr, DIRECTION *dp);
bool get_hack_dir(player_type *creature_ptr, DIRECTION *dp);
bool get_direction(player_type *creature_ptr, DIRECTION *dp, bool allow_under, bool with_steed);
bool get_rep_dir(player_type *creature_ptr, DIRECTION *dp, bool under);
bool tgt_pt(player_type *creature_ptr, POSITION *x, POSITION *y);
int get_max_range(player_type *creature_ptr);
