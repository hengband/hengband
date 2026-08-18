#include "effect/attribute/fire-attribute.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"

bool FireAttribute::affect_feature(PlayerType *const player_ptr, const MONSTER_IDX, const TerrainType &terrain, Grid &grid, const Pos2D &pos, const int, const bool)
{
    auto tree_destroyed = this->destroy_tree(player_ptr, terrain, grid, pos, _("燃えた", "burns up!"));
    if (tree_destroyed != ProcessResult::PROCESS_CONTINUE) {
        return tree_destroyed == ProcessResult::PROCESS_TRUE;
    }
    return false;
}
