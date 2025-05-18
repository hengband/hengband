#pragma once

/*!
 * @file grid.h
 * @brief グリッドに関するコントローラ的処理定義
 * @author Hourier
 * @date 2015/01/25
 */

#include "object/object-index-list.h"
#include "spell/spells-util.h"
#include "system/angband.h"
#include "util/point-2d.h"
#include <optional>

enum class AttributeType;
class Grid;
class GridTemplate {
public:
    GridTemplate()
        : GridTemplate(0, 0, 0, 0, 0)
    {
    }

    GridTemplate(BIT_FLAGS info, FEAT_IDX feat, FEAT_IDX mimic, short special, uint16_t occurrence)
        : info(info)
        , feat(feat)
        , mimic(mimic)
        , special(special)
        , occurrence(occurrence)
    {
    }

    BIT_FLAGS info;
    FEAT_IDX feat;
    FEAT_IDX mimic;
    short special;
    uint16_t occurrence;

    bool matches(const Grid &grid) const;
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

class DisplaySymbol;
class FloorType;
class Grid;
class PlayerType;
class MonraceDefinition;
enum class GridCountKind;
enum class TerrainCharacteristics;
enum class TerrainTag;
void set_terrain_id_to_grid(PlayerType *player_ptr, const Pos2D &pos, TerrainTag tag);
void set_terrain_id_to_grid(PlayerType *player_ptr, const Pos2D &pos, short terrain_id);
std::optional<Pos2D> new_player_spot(PlayerType *player_ptr);
bool player_can_enter(PlayerType *player_ptr, FEAT_IDX feature, BIT_FLAGS16 mode);
void update_local_illumination(PlayerType *player_ptr, const Pos2D &pos);
bool no_lite(PlayerType *player_ptr);
void print_rel(PlayerType *player_ptr, const DisplaySymbol &symbol, POSITION y, POSITION x);
void print_bolt_pict(PlayerType *player_ptr, POSITION y, POSITION x, POSITION ny, POSITION nx, AttributeType typ);
void note_spot(PlayerType *player_ptr, const Pos2D &pos);
void lite_spot(PlayerType *player_ptr, const Pos2D &pos);
void update_flow(PlayerType *player_ptr);
void cave_alter_feat(PlayerType *player_ptr, POSITION y, POSITION x, TerrainCharacteristics action);
bool check_local_illumination(PlayerType *player_ptr, POSITION y, POSITION x);
bool cave_monster_teleportable_bold(PlayerType *player_ptr, MONSTER_IDX m_idx, POSITION y, POSITION x, teleport_flags mode);
bool cave_player_teleportable_bold(PlayerType *player_ptr, POSITION y, POSITION x, teleport_flags mode);
void place_grid(PlayerType *player_ptr, Grid &grid, grid_bold_type pg_type);
void place_bold(PlayerType *player_ptr, POSITION y, POSITION x, grid_bold_type gh_type);
void cave_lite_hack(FloorType &floor, POSITION y, POSITION x);
void cave_redraw_later(FloorType &floor, POSITION y, POSITION x);
void cave_note_and_redraw_later(FloorType &floor, POSITION y, POSITION x);
void cave_view_hack(FloorType &floor, POSITION y, POSITION x);
