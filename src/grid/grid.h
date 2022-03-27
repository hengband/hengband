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

#include "object/object-index-list.h"
#include "spell/spells-util.h"
#include "system/angband.h"

enum class AttributeType;

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
class PlayerType;
struct monster_race;
enum class FloorFeatureType;
bool new_player_spot(PlayerType *player_ptr);
bool is_hidden_door(PlayerType *player_ptr, grid_type *g_ptr);
bool player_can_enter(PlayerType *player_ptr, FEAT_IDX feature, BIT_FLAGS16 mode);
bool feat_uses_special(FEAT_IDX f_idx);
void update_local_illumination(PlayerType *player_ptr, POSITION y, POSITION x);
bool no_lite(PlayerType *player_ptr);
void print_rel(PlayerType *player_ptr, char c, TERM_COLOR a, POSITION y, POSITION x);
void print_bolt_pict(PlayerType *player_ptr, POSITION y, POSITION x, POSITION ny, POSITION nx, AttributeType typ);
void note_spot(PlayerType *player_ptr, POSITION y, POSITION x);
void lite_spot(PlayerType *player_ptr, POSITION y, POSITION x);
void update_flow(PlayerType *player_ptr);
FEAT_IDX feat_state(floor_type *floor_ptr, FEAT_IDX feat, FloorFeatureType action);
void cave_alter_feat(PlayerType *player_ptr, POSITION y, POSITION x, FloorFeatureType action);
bool is_open(PlayerType *player_ptr, FEAT_IDX feat);
bool check_local_illumination(PlayerType *player_ptr, POSITION y, POSITION x);
bool cave_monster_teleportable_bold(PlayerType *player_ptr, MONSTER_IDX m_idx, POSITION y, POSITION x, teleport_flags mode);
bool cave_player_teleportable_bold(PlayerType *player_ptr, POSITION y, POSITION x, teleport_flags mode);
void place_grid(PlayerType *player_ptr, grid_type *g_ptr, grid_bold_type pg_type);
bool darkened_grid(PlayerType *player_ptr, grid_type *g_ptr);
void delete_monster(PlayerType *player_ptr, POSITION y, POSITION x);
void place_bold(PlayerType *player_ptr, POSITION y, POSITION x, grid_bold_type gh_type);
void set_cave_feat(floor_type *floor_ptr, POSITION y, POSITION x, FEAT_IDX feature_idx);
int count_dt(PlayerType *player_ptr, POSITION *y, POSITION *x, bool (*test)(PlayerType *, FEAT_IDX), bool under);
void cave_lite_hack(floor_type *floor_ptr, POSITION y, POSITION x);
void cave_redraw_later(floor_type *floor_ptr, POSITION y, POSITION x);
void cave_note_and_redraw_later(floor_type *floor_ptr, POSITION y, POSITION x);
void cave_view_hack(floor_type *floor_ptr, POSITION y, POSITION x);
