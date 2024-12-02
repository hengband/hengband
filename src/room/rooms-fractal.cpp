#include "room/rooms-fractal.h"
#include "dungeon/dungeon-flag-types.h"
#include "floor/floor-generator.h"
#include "room/cave-filler.h"
#include "room/rooms-normal.h"
#include "room/space-finder.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"

/*!
 * @brief タイプ9の部屋…フラクタルカーブによる洞窟生成 / Type 9 -- Driver routine to create fractal grid
 * @return なし
 */
bool build_type9(PlayerType *player_ptr, DungeonData *dd_ptr)
{
    /* get size: note 'Evenness'*/
    auto width = randint1(22) * 2 + 6;
    auto height = randint1(15) * 2 + 6;

    /* Find and reserve some space in the dungeon.  Get center of room. */
    auto &floor = *player_ptr->current_floor_ptr;
    int y0;
    int x0;
    if (!find_space(player_ptr, dd_ptr, &y0, &x0, height + 1, width + 1)) {
        /* Limit to the minimum room size, and retry */
        width = 8;
        height = 8;

        /* Find and reserve some space in the dungeon.  Get center of room. */
        if (!find_space(player_ptr, dd_ptr, &y0, &x0, height + 1, width + 1)) {
            /*
             * Still no space?!
             * Try normal room
             */
            return build_type1(player_ptr, dd_ptr);
        }
    }

    const auto should_brighten = (floor.dun_level <= randint1(25)) && floor.get_dungeon_definition().flags.has_not(DungeonFeatureType::DARKNESS);
    while (true) {
        /* Note: size must be even or there are rounding problems
         * This causes the tunnels not to connect properly to the room */

        /* testing values for these parameters feel free to adjust */
        const auto grd = 1 << (randint0(4));

        /* want average of about 16 */
        const auto roug = randint1(8) * randint1(4);

        /* about size/2 */
        const auto cutoff = randint1(width / 4) + randint1(height / 4) +
                            randint1(width / 4) + randint1(height / 4);

        /* make it */
        generate_hmap(&floor, y0, x0, width, height, grd, roug, cutoff);

        /* Convert to normal format + clean up */
        if (generate_fracave(player_ptr, y0, x0, width, height, cutoff, should_brighten, true)) {
            break;
        }
    }

    return true;
}
