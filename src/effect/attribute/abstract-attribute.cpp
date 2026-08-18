#include "effect/attribute/abstract-attribute.h"
#include "grid/grid.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/grid-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "view/display-messages.h"

ProcessResult AbstractAttribute::destroy_tree(PlayerType *player_ptr, const TerrainType &terrain, const Grid &grid, const Pos2D &pos, const std::string &note)
{
    if (!terrain.flags.has(TerrainCharacteristics::TREE)) {
        return ProcessResult::PROCESS_CONTINUE;
    }

    msg_format(_("木は%s。", "A tree %s"), note.c_str());

    set_terrain_id_to_grid(player_ptr, pos, one_in_(3) ? TerrainTag::BRAKE : TerrainTag::GRASS);

    /* Observe */
    return grid.is_mark() ? ProcessResult::PROCESS_TRUE : ProcessResult::PROCESS_FALSE;
}
