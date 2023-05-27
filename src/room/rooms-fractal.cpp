#include "room/rooms-fractal.h"
#include "dungeon/dungeon-flag-types.h"
#include "floor/floor-generator.h"
#include "room/cave-filler.h"
#include "room/rooms-normal.h"
#include "room/space-finder.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"

/*!
 * @brief タイプ9の部屋…フラクタルカーブによる洞窟生成 / Type 9 -- Driver routine to create fractal grid
 * @return なし
 */
bool build_type9(PlayerType *player_ptr, dun_data_type *dd_ptr)
{
    int grd, roug, cutoff;
    POSITION xsize, ysize, y0, x0;

    bool done, light, room;

    /* get size: note 'Evenness'*/
    xsize = randint1(22) * 2 + 6;
    ysize = randint1(15) * 2 + 6;

    /* Find and reserve some space in the dungeon.  Get center of room. */
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (!find_space(player_ptr, dd_ptr, &y0, &x0, ysize + 1, xsize + 1)) {
        /* Limit to the minimum room size, and retry */
        xsize = 8;
        ysize = 8;

        /* Find and reserve some space in the dungeon.  Get center of room. */
        if (!find_space(player_ptr, dd_ptr, &y0, &x0, ysize + 1, xsize + 1)) {
            /*
             * Still no space?!
             * Try normal room
             */
            return build_type1(player_ptr, dd_ptr);
        }
    }

    light = done = false;
    room = true;

    if ((floor_ptr->dun_level <= randint1(25)) && floor_ptr->get_dungeon_definition().flags.has_not(DungeonFeatureType::DARKNESS)) {
        light = true;
    }

    while (!done) {
        /* Note: size must be even or there are rounding problems
         * This causes the tunnels not to connect properly to the room */

        /* testing values for these parameters feel free to adjust */
        grd = 1 << (randint0(4));

        /* want average of about 16 */
        roug = randint1(8) * randint1(4);

        /* about size/2 */
        cutoff = randint1(xsize / 4) + randint1(ysize / 4) +
                 randint1(xsize / 4) + randint1(ysize / 4);

        /* make it */
        generate_hmap(floor_ptr, y0, x0, xsize, ysize, grd, roug, cutoff);

        /* Convert to normal format + clean up */
        done = generate_fracave(player_ptr, y0, x0, xsize, ysize, cutoff, light, room);
    }

    return true;
}
