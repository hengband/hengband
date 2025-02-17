/*!
 * @brief fill_data_type構造体を使ってダンジョンを生成/構成する処理
 * @date 2020/07/24
 * @author Hourier
 */

#include "room/cave-filler.h"
#include "grid/grid.h"
#include "room/lake-types.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include <queue>

struct fill_data_type {
    POSITION xmin;
    POSITION ymin;
    POSITION xmax;
    POSITION ymax;

    /* cutoffs */
    int c1;
    int c2;
    int c3;

    /* features to fill with */
    FEAT_IDX feat1;
    FEAT_IDX feat2;
    FEAT_IDX feat3;

    int info1;
    int info2;
    int info3;

    /* number of filled squares */
    int amount;
};

static fill_data_type fill_data;

/*!
 * Store routine for the fractal floor generator
 * this routine probably should be an inline function or a macro.
 */
static void store_height(FloorType &floor, POSITION x, POSITION y, FEAT_IDX val)
{
    if (((x == fill_data.xmin) || (y == fill_data.ymin) || (x == fill_data.xmax) || (y == fill_data.ymax)) && (val <= fill_data.c1)) {
        val = fill_data.c1 + 1;
    }

    floor.grid_array[y][x].feat = val;
    return;
}

void generate_hmap(FloorType &floor, POSITION y0, POSITION x0, POSITION xsiz, POSITION ysiz, int grd, int roug, int cutoff)
{
    POSITION xsize = xsiz;
    POSITION ysize = ysiz;

    if (xsize > 254) {
        xsize = 254;
    }

    if (xsize < 4) {
        xsize = 4;
    }

    if (ysize > 254) {
        ysize = 254;
    }

    if (ysize < 4) {
        ysize = 4;
    }

    POSITION xhsize = xsize / 2;
    POSITION yhsize = ysize / 2;
    xsize = xhsize * 2;
    ysize = yhsize * 2;

    fill_data.xmin = x0 - xhsize;
    fill_data.ymin = y0 - yhsize;
    fill_data.xmax = x0 + xhsize;
    fill_data.ymax = y0 + yhsize;
    fill_data.c1 = cutoff;
    POSITION diagsize = 362;
    POSITION maxsize = (xsize > ysize) ? xsize : ysize;
    for (POSITION i = 0; i <= xsize; i++) {
        for (POSITION j = 0; j <= ysize; j++) {
            floor.grid_array[(int)(fill_data.ymin + j)][(int)(fill_data.xmin + i)].feat = -1;
            floor.grid_array[(int)(fill_data.ymin + j)][(int)(fill_data.xmin + i)].info &= ~(CAVE_ICKY);
        }
    }

    floor.grid_array[fill_data.ymin][fill_data.xmin].feat = (int16_t)maxsize;
    floor.grid_array[fill_data.ymax][fill_data.xmin].feat = (int16_t)maxsize;
    floor.grid_array[fill_data.ymin][fill_data.xmax].feat = (int16_t)maxsize;
    floor.grid_array[fill_data.ymax][fill_data.xmax].feat = (int16_t)maxsize;
    floor.grid_array[y0][x0].feat = 0;
    POSITION xstep = xsize * 256;
    POSITION xhstep = xsize * 256;
    POSITION ystep = ysize * 256;
    POSITION yhstep = ysize * 256;
    POSITION xxsize = xsize * 256;
    POSITION yysize = ysize * 256;
    while ((xhstep > 256) || (yhstep > 256)) {
        xstep = xhstep;
        xhstep /= 2;
        ystep = yhstep;
        yhstep /= 2;
        POSITION xstep2 = xstep / 256;
        POSITION ystep2 = ystep / 256;
        POSITION xhstep2 = xhstep / 256;
        POSITION yhstep2 = yhstep / 256;
        for (POSITION i = xhstep; i <= xxsize - xhstep; i += xstep) {
            for (POSITION j = 0; j <= yysize; j += ystep) {
                POSITION ii = i / 256 + fill_data.xmin;
                POSITION jj = j / 256 + fill_data.ymin;
                if (floor.grid_array[jj][ii].feat != -1) {
                    continue;
                }

                if (xhstep2 > grd) {
                    store_height(floor, ii, jj, randnum1<short>(maxsize));
                    continue;
                }

                store_height(floor, ii, jj,
                    (floor.grid_array[jj][fill_data.xmin + (i - xhstep) / 256].feat + floor.grid_array[jj][fill_data.xmin + (i + xhstep) / 256].feat) / 2 + (randint1(xstep2) - xhstep2) * roug / 16);
            }
        }

        for (POSITION j = yhstep; j <= yysize - yhstep; j += ystep) {
            for (POSITION i = 0; i <= xxsize; i += xstep) {
                POSITION ii = i / 256 + fill_data.xmin;
                POSITION jj = j / 256 + fill_data.ymin;
                if (floor.grid_array[jj][ii].feat != -1) {
                    continue;
                }

                if (xhstep2 > grd) {
                    store_height(floor, ii, jj, randnum1<short>(maxsize));
                    continue;
                }

                store_height(floor, ii, jj,
                    (floor.grid_array[fill_data.ymin + (j - yhstep) / 256][ii].feat + floor.grid_array[fill_data.ymin + (j + yhstep) / 256][ii].feat) / 2 + (randint1(ystep2) - yhstep2) * roug / 16);
            }
        }

        for (POSITION i = xhstep; i <= xxsize - xhstep; i += xstep) {
            for (POSITION j = yhstep; j <= yysize - yhstep; j += ystep) {
                POSITION ii = i / 256 + fill_data.xmin;
                POSITION jj = j / 256 + fill_data.ymin;
                if (floor.grid_array[jj][ii].feat != -1) {
                    continue;
                }

                if (xhstep2 > grd) {
                    store_height(floor, ii, jj, randnum1<short>(maxsize));
                    continue;
                }

                POSITION xm = fill_data.xmin + (i - xhstep) / 256;
                POSITION xp = fill_data.xmin + (i + xhstep) / 256;
                POSITION ym = fill_data.ymin + (j - yhstep) / 256;
                POSITION yp = fill_data.ymin + (j + yhstep) / 256;
                store_height(floor, ii, jj,
                    (floor.grid_array[ym][xm].feat + floor.grid_array[yp][xm].feat + floor.grid_array[ym][xp].feat + floor.grid_array[yp][xp].feat) / 4 + (randint1(xstep2) - xhstep2) * (diagsize / 16) / 256 * roug);
            }
        }
    }
}

static bool hack_isnt_wall(PlayerType *player_ptr, POSITION y, POSITION x, int c1, int c2, int c3, FEAT_IDX feat1, FEAT_IDX feat2, FEAT_IDX feat3,
    BIT_FLAGS info1, BIT_FLAGS info2, BIT_FLAGS info3)
{
    auto &floor = *player_ptr->current_floor_ptr;
    if (floor.grid_array[y][x].info & CAVE_ICKY) {
        return false;
    }

    floor.grid_array[y][x].info |= (CAVE_ICKY);
    if (floor.grid_array[y][x].feat <= c1) {
        if (randint1(100) < 75) {
            floor.grid_array[y][x].feat = feat1;
            floor.grid_array[y][x].info &= ~(CAVE_MASK);
            floor.grid_array[y][x].info |= info1;
            return true;
        } else {
            floor.grid_array[y][x].feat = feat2;
            floor.grid_array[y][x].info &= ~(CAVE_MASK);
            floor.grid_array[y][x].info |= info2;
            return true;
        }
    }

    if (floor.grid_array[y][x].feat <= c2) {
        if (randint1(100) < 75) {
            floor.grid_array[y][x].feat = feat2;
            floor.grid_array[y][x].info &= ~(CAVE_MASK);
            floor.grid_array[y][x].info |= info2;
            return true;
        } else {
            floor.grid_array[y][x].feat = feat1;
            floor.grid_array[y][x].info &= ~(CAVE_MASK);
            floor.grid_array[y][x].info |= info1;
            return true;
        }
    }

    if (floor.grid_array[y][x].feat <= c3) {
        floor.grid_array[y][x].feat = feat3;
        floor.grid_array[y][x].info &= ~(CAVE_MASK);
        floor.grid_array[y][x].info |= info3;
        return true;
    }

    place_bold(player_ptr, y, x, GB_OUTER);
    return false;
}

/*
 * Quick and nasty fill routine used to find the connected region
 * of floor in the middle of the grids
 */
static void cave_fill(PlayerType *player_ptr, const Pos2D &initial_pos)
{
    auto &floor = *player_ptr->current_floor_ptr;

    // 幅優先探索用のキュー。
    std::queue<Pos2D> que;
    que.push(initial_pos);

    while (!que.empty()) {
        // 参照で受けるとダングリング状態になるのでコピーする.
        const Pos2D pos = que.front();
        que.pop();
        for (const auto &d : Direction::directions_8()) {
            const auto pos_to = pos + d.vec();
            auto &grid = floor.get_grid(pos_to);
            if (!floor.contains(pos_to)) {
                grid.info |= CAVE_ICKY;
                continue;
            }

            if ((pos_to.x <= fill_data.xmin) || (pos_to.x >= fill_data.xmax) || (pos_to.y <= fill_data.ymin) || (pos_to.y >= fill_data.ymax)) {
                grid.info |= CAVE_ICKY;
                continue;
            }

            if (!hack_isnt_wall(player_ptr, pos_to.y, pos_to.x, fill_data.c1, fill_data.c2, fill_data.c3, fill_data.feat1, fill_data.feat2, fill_data.feat3,
                    fill_data.info1, fill_data.info2, fill_data.info3)) {
                continue;
            }

            que.push(pos_to);
            fill_data.amount++;
        }
    }
}

bool generate_fracave(PlayerType *player_ptr, POSITION y0, POSITION x0, POSITION xsize, POSITION ysize, int cutoff, bool light, bool room)
{
    auto &floor = *player_ptr->current_floor_ptr;
    const auto &dungeon = floor.get_dungeon_definition();
    POSITION xhsize = xsize / 2;
    POSITION yhsize = ysize / 2;
    fill_data.c1 = cutoff;
    fill_data.c2 = 0;
    fill_data.c3 = 0;
    fill_data.feat1 = dungeon.select_floor_terrain_id();
    fill_data.feat2 = dungeon.select_floor_terrain_id();
    fill_data.feat3 = dungeon.select_floor_terrain_id();
    fill_data.info1 = CAVE_FLOOR;
    fill_data.info2 = CAVE_FLOOR;
    fill_data.info3 = CAVE_FLOOR;
    fill_data.amount = 0;
    cave_fill(player_ptr, { y0, x0 });
    if (fill_data.amount < 10) {
        for (POSITION x = 0; x <= xsize; ++x) {
            for (POSITION y = 0; y <= ysize; ++y) {
                place_bold(player_ptr, y0 + y - yhsize, x0 + x - xhsize, GB_EXTRA);
                floor.grid_array[y0 + y - yhsize][x0 + x - xhsize].info &= ~(CAVE_ICKY | CAVE_ROOM);
            }
        }

        return false;
    }

    for (int i = 0; i <= xsize; ++i) {
        auto &grid1 = floor.grid_array[y0 - yhsize][i + x0 - xhsize];
        if (grid1.is_icky() && (room)) {
            place_bold(player_ptr, y0 - yhsize, x0 + i - xhsize, GB_OUTER);
            if (light) {
                floor.grid_array[y0 - yhsize][x0 + i - xhsize].info |= (CAVE_GLOW);
            }

            grid1.info |= (CAVE_ROOM);
            place_bold(player_ptr, y0 - yhsize, x0 + i - xhsize, GB_OUTER);
        } else {
            place_bold(player_ptr, y0 - yhsize, x0 + i - xhsize, GB_EXTRA);
        }

        auto &grid2 = floor.grid_array[ysize + y0 - yhsize][i + x0 - xhsize];
        if (grid2.is_icky() && (room)) {
            place_bold(player_ptr, y0 + ysize - yhsize, x0 + i - xhsize, GB_OUTER);
            if (light) {
                grid2.info |= (CAVE_GLOW);
            }

            grid2.info |= (CAVE_ROOM);
            place_bold(player_ptr, y0 + ysize - yhsize, x0 + i - xhsize, GB_OUTER);
        } else {
            place_bold(player_ptr, y0 + ysize - yhsize, x0 + i - xhsize, GB_EXTRA);
        }

        grid1.info &= ~(CAVE_ICKY);
        grid2.info &= ~(CAVE_ICKY);
    }

    for (int i = 1; i < ysize; ++i) {
        auto &grid1 = floor.grid_array[i + y0 - yhsize][x0 - xhsize];
        if (grid1.is_icky() && room) {
            place_bold(player_ptr, y0 + i - yhsize, x0 - xhsize, GB_OUTER);
            if (light) {
                grid1.info |= (CAVE_GLOW);
            }

            grid1.info |= (CAVE_ROOM);
            place_bold(player_ptr, y0 + i - yhsize, x0 - xhsize, GB_OUTER);
        } else {
            place_bold(player_ptr, y0 + i - yhsize, x0 - xhsize, GB_EXTRA);
        }

        auto &grid2 = floor.grid_array[i + y0 - yhsize][xsize + x0 - xhsize];
        if (grid2.is_icky() && room) {
            place_bold(player_ptr, y0 + i - yhsize, x0 + xsize - xhsize, GB_OUTER);
            if (light) {
                grid2.info |= (CAVE_GLOW);
            }

            grid2.info |= (CAVE_ROOM);
            place_bold(player_ptr, y0 + i - yhsize, x0 + xsize - xhsize, GB_OUTER);
        } else {
            place_bold(player_ptr, y0 + i - yhsize, x0 + xsize - xhsize, GB_EXTRA);
        }

        grid1.info &= ~(CAVE_ICKY);
        grid2.info &= ~(CAVE_ICKY);
    }

    for (POSITION x = 1; x < xsize; ++x) {
        for (POSITION y = 1; y < ysize; ++y) {
            auto &grid1 = floor.grid_array[y0 + y - yhsize][x0 + x - xhsize];
            if (grid1.is_floor() && grid1.is_icky()) {
                grid1.info &= ~CAVE_ICKY;
                if (light) {
                    grid1.info |= (CAVE_GLOW);
                }

                if (room) {
                    grid1.info |= (CAVE_ROOM);
                }

                continue;
            }

            auto &grid2 = floor.grid_array[y0 + y - yhsize][x0 + x - xhsize];
            if (grid2.is_outer() && grid2.is_icky()) {
                grid2.info &= ~(CAVE_ICKY);
                if (light) {
                    grid2.info |= (CAVE_GLOW);
                }

                if (room) {
                    grid2.info |= (CAVE_ROOM);
                } else {
                    place_bold(player_ptr, y0 + y - yhsize, x0 + x - xhsize, GB_EXTRA);
                    grid2.info &= ~(CAVE_ROOM);
                }

                continue;
            }

            place_bold(player_ptr, y0 + y - yhsize, x0 + x - xhsize, GB_EXTRA);
            grid2.info &= ~(CAVE_ICKY | CAVE_ROOM);
        }
    }

    return true;
}

bool generate_lake(PlayerType *player_ptr, POSITION y0, POSITION x0, POSITION xsize, POSITION ysize, int c1, int c2, int c3, int type)
{
    auto &floor = *player_ptr->current_floor_ptr;
    const auto &dungeon = floor.get_dungeon_definition();
    const auto &terrains = TerrainList::get_instance();
    const auto terrain_id_rubble = terrains.get_terrain_id(TerrainTag::RUBBLE);
    const auto terrain_id_deep_water = terrains.get_terrain_id(TerrainTag::DEEP_WATER);
    const auto terrain_id_shallow_water = terrains.get_terrain_id(TerrainTag::SHALLOW_WATER);
    const auto terrain_id_deep_lava = terrains.get_terrain_id(TerrainTag::DEEP_LAVA);
    const auto terrain_id_shallow_lava = terrains.get_terrain_id(TerrainTag::SHALLOW_LAVA);
    const auto terrain_id_grass = terrains.get_terrain_id(TerrainTag::GRASS);
    const auto xhsize = xsize / 2;
    const auto yhsize = ysize / 2;
    short feat1;
    short feat2;
    short feat3;
    switch (type) {
    case LAKE_T_LAVA: /* Lava */
        feat1 = terrain_id_deep_lava;
        feat2 = terrain_id_shallow_lava;
        feat3 = dungeon.select_floor_terrain_id();
        break;
    case LAKE_T_WATER: /* Water */
        feat1 = terrain_id_deep_water;
        feat2 = terrain_id_shallow_water;
        feat3 = dungeon.select_floor_terrain_id();
        break;
    case LAKE_T_CAVE: /* Collapsed floor.grid_array */
        feat1 = dungeon.select_floor_terrain_id();
        feat2 = dungeon.select_floor_terrain_id();
        feat3 = terrain_id_rubble;
        break;
    case LAKE_T_EARTH_VAULT: /* Earth vault */
        feat1 = terrain_id_rubble;
        feat2 = dungeon.select_floor_terrain_id();
        feat3 = terrain_id_rubble;
        break;
    case LAKE_T_AIR_VAULT: /* Air vault */
        feat1 = terrain_id_grass;
        feat2 = terrains.get_terrain_id(TerrainTag::TREE);
        feat3 = terrain_id_grass;
        break;
    case LAKE_T_WATER_VAULT: /* Water vault */
        feat1 = terrain_id_shallow_water;
        feat2 = terrain_id_deep_water;
        feat3 = terrain_id_shallow_water;
        break;
    case LAKE_T_FIRE_VAULT: /* Fire Vault */
        feat1 = terrain_id_shallow_lava;
        feat2 = terrain_id_deep_lava;
        feat3 = terrain_id_shallow_lava;
        break;
    default:
        return false;
    }

    fill_data.c1 = c1;
    fill_data.c2 = c2;
    fill_data.c3 = c3;
    fill_data.feat1 = feat1;
    fill_data.feat2 = feat2;
    fill_data.feat3 = feat3;
    fill_data.info1 = 0;
    fill_data.info2 = 0;
    fill_data.info3 = 0;
    fill_data.amount = 0;

    cave_fill(player_ptr, { y0, x0 });
    if (fill_data.amount < 10) {
        for (auto x = 0; x <= xsize; ++x) {
            for (auto y = 0; y <= ysize; ++y) {
                const Pos2D pos(y0 + y - yhsize, x0 + x - xhsize);
                place_bold(player_ptr, pos.y, pos.x, GB_FLOOR);
                floor.get_grid(pos).info &= ~(CAVE_ICKY);
            }
        }

        return false;
    }

    for (auto i = 0; i <= xsize; ++i) {
        const Pos2D pos(y0 - yhsize, x0 + i - xhsize);
        const Pos2DVec vec(ysize, 0);
        place_bold(player_ptr, pos.y, pos.x, GB_EXTRA);
        place_bold(player_ptr, (pos + vec).y, (pos + vec).x, GB_EXTRA);
        floor.get_grid(pos).info &= ~(CAVE_ICKY);
        floor.get_grid(pos + vec).info &= ~(CAVE_ICKY);
    }

    for (auto i = 1; i < ysize; ++i) {
        const Pos2D pos(y0 + i - yhsize, x0 - xhsize);
        const Pos2DVec vec(0, xsize);
        place_bold(player_ptr, pos.y, pos.x, GB_EXTRA);
        place_bold(player_ptr, (pos + vec).y, (pos + vec).x, GB_EXTRA);
        floor.get_grid(pos).info &= ~(CAVE_ICKY);
        floor.get_grid(pos + vec).info &= ~(CAVE_ICKY);
    }

    for (auto x = 1; x < xsize; ++x) {
        for (auto y = 1; y < ysize; ++y) {
            const Pos2D pos(y0 + y - yhsize, x0 + x - xhsize);
            auto &grid = floor.get_grid(pos);
            if (!grid.is_icky() || grid.is_outer()) {
                place_bold(player_ptr, pos.y, pos.x, GB_EXTRA);
            }

            grid.info &= ~(CAVE_ICKY | CAVE_ROOM);
            if (floor.has_terrain_characteristics(pos, TerrainCharacteristics::LAVA)) {
                if (floor.get_dungeon_definition().flags.has_not(DungeonFeatureType::DARKNESS)) {
                    grid.info |= CAVE_GLOW;
                }
            }
        }
    }

    return true;
}
