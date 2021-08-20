#pragma once

/*!
 * @file grid.h
 * @brief ダンジョンの生成処理の基幹部分ヘッダーファイル
 * @date 2014/08/15
 * @details
 * Purpose: header file for grid.c, used only in dungeon generation
 * files (generate.c, rooms.c)
 * @author
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#include "system/angband.h"
#include "object/object-index-list.h"
#include "spell/spells-util.h"

/*  A structure type for terrain template of saving dungeon floor */
struct grid_template_type {
    BIT_FLAGS info;
    FEAT_IDX feat;
    FEAT_IDX mimic;
    int16_t special;
    uint16_t occurrence;
};

enum grid_bold_type {
    GB_FLOOR,
    GB_EXTRA,
    GB_EXTRA_PERM,
    GB_INNER,
    GB_INNER_PERM,
    GB_OUTER,
    GB_OUTER_NOPERM,
    GB_SOLID,
    GB_SOLID_PERM,
    GB_SOLID_NOPERM
};

struct floor_type;
struct grid_type;
struct player_type;
struct monster_race;
enum class FF;
bool new_player_spot(player_type *creature_ptr);
bool is_hidden_door(player_type *player_ptr, grid_type *g_ptr);
bool player_can_enter(player_type *creature_ptr, FEAT_IDX feature, BIT_FLAGS16 mode);
bool feat_uses_special(FEAT_IDX f_idx);
void update_local_illumination(player_type *creature_ptr, POSITION y, POSITION x);
bool no_lite(player_type *creature_ptr);
void print_rel(player_type *subject_ptr, SYMBOL_CODE c, TERM_COLOR a, POSITION y, POSITION x);
void note_spot(player_type *player_ptr, POSITION y, POSITION x);
void lite_spot(player_type *player_ptr, POSITION y, POSITION x);
void update_flow(player_type *subject_ptr);
FEAT_IDX feat_state(floor_type *floor_ptr, FEAT_IDX feat, FF action);
void cave_alter_feat(player_type *player_ptr, POSITION y, POSITION x, FF action);
void remove_mirror(player_type *caster_ptr, POSITION y, POSITION x);
bool is_open(player_type *player_ptr, FEAT_IDX feat);
bool check_local_illumination(player_type *creature_ptr, POSITION y, POSITION x);
bool cave_monster_teleportable_bold(player_type *player_ptr, MONSTER_IDX m_idx, POSITION y, POSITION x, teleport_flags mode);
bool cave_player_teleportable_bold(player_type *player_ptr, POSITION y, POSITION x, teleport_flags mode);
void place_grid(player_type *player_ptr, grid_type *g_ptr, grid_bold_type pg_type);
bool darkened_grid(player_type *player_ptr, grid_type *g_ptr);
void delete_monster(player_type *player_ptr, POSITION y, POSITION x);
void place_bold(player_type *player_ptr, POSITION y, POSITION x, grid_bold_type gh_type);
void set_cave_feat(floor_type *floor_ptr, POSITION y, POSITION x, FEAT_IDX feature_idx);
int count_dt(player_type *creature_ptr, POSITION *y, POSITION *x, bool (*test)(player_type *, FEAT_IDX), bool under);
void cave_lite_hack(floor_type *floor_ptr, POSITION y, POSITION x);
void cave_redraw_later(floor_type *floor_ptr, POSITION y, POSITION x);
void cave_note_and_redraw_later(floor_type *floor_ptr, POSITION y, POSITION x);
void cave_view_hack(floor_type *floor_ptr, POSITION y, POSITION x);
