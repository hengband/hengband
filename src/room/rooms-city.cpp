#include "room/rooms-city.h"
#include "floor/floor-generator.h"
#include "floor/floor-town.h"
#include "game-option/cheat-types.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "room/space-finder.h"
#include "store/store-util.h"
#include "store/store.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "system/terrain-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "wizard/wizard-messages.h"
#include <algorithm>

namespace {
const std::vector<StoreSaleType> stores = {
    StoreSaleType::GENERAL,
    StoreSaleType::ARMOURY,
    StoreSaleType::WEAPON,
    StoreSaleType::TEMPLE,
    StoreSaleType::ALCHEMIST,
    StoreSaleType::MAGIC,
    StoreSaleType::BLACK,
    StoreSaleType::BOOK,
};
}

/*
 * Precalculate buildings' location of underground arcade
 */
static std::optional<std::vector<ugbldg_type>> precalc_ugarcade(int town_hgt, int town_wid)
{
    const auto n = std::ssize(stores);
    std::vector<ugbldg_type> underground_buildings(n);
    const auto max_buildings_height = 3 * town_hgt / MAX_TOWN_HGT;
    const auto max_buildings_width = 5 * town_wid / MAX_TOWN_WID;
    std::vector<std::vector<bool>> ugarcade_used(town_hgt, std::vector<bool>(town_wid));
    int i;
    auto attempt = 10000;
    auto should_abort = false;
    for (i = 0; i < n; i++) {
        auto &cur_ugbldg = underground_buildings[i];
        do {
            const auto center_y = rand_range(2, town_hgt - 3);
            const auto center_x = rand_range(2, town_wid - 3);
            auto tmp = center_y - randint1(max_buildings_height);
            cur_ugbldg.y0 = std::max(tmp, 1);
            tmp = center_x - randint1(max_buildings_width);
            cur_ugbldg.x0 = std::max(tmp, 1);
            tmp = center_y + randint1(max_buildings_height);
            cur_ugbldg.y1 = std::min(tmp, town_hgt - 2);
            tmp = center_x + randint1(max_buildings_width);
            cur_ugbldg.x1 = std::min(tmp, town_wid - 2);
            should_abort = false;
            for (auto y = cur_ugbldg.y0; (y <= cur_ugbldg.y1) && !should_abort; y++) {
                for (auto x = cur_ugbldg.x0; x <= cur_ugbldg.x1; x++) {
                    if (ugarcade_used[y][x]) {
                        should_abort = true;
                        break;
                    }
                }
            }

            attempt--;
        } while (should_abort && (attempt > 0));

        if (attempt == 0) {
            break;
        }

        for (auto y = cur_ugbldg.y0 - 1; y <= cur_ugbldg.y1 + 1; y++) {
            for (auto x = cur_ugbldg.x0 - 1; x <= cur_ugbldg.x1 + 1; x++) {
                ugarcade_used[y][x] = true;
            }
        }
    }

    if (i != n) {
        return std::nullopt;
    }

    return underground_buildings;
}

/* Create a new floor room with optional light */
static void generate_room_floor(PlayerType *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2, int light)
{
    for (auto y = y1; y <= y2; y++) {
        for (auto x = x1; x <= x2; x++) {
            auto &grid = player_ptr->current_floor_ptr->grid_array[y][x];
            place_grid(player_ptr, &grid, GB_FLOOR);
            grid.info |= (CAVE_ROOM);
            if (light) {
                grid.info |= (CAVE_GLOW);
            }
        }
    }
}

static void generate_fill_perm_bold(PlayerType *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2)
{
    for (auto y = y1; y <= y2; y++) {
        for (auto x = x1; x <= x2; x++) {
            place_bold(player_ptr, y, x, GB_INNER_PERM);
        }
    }
}

/*!
 * @brief タイプ16の部屋…地下都市生成のサブルーチン / Actually create buildings
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pos_ug 地下都市エリアの左上座標
 * @param underground_buildings 生成する店舗のリスト
 */
static void build_stores(PlayerType *player_ptr, const Pos2D &pos_ug, const std::vector<ugbldg_type> &underground_buildings)
{
    for (const auto &ug_building : underground_buildings) {
        generate_room_floor(player_ptr, pos_ug.y + ug_building.y0 - 2, pos_ug.x + ug_building.x0 - 2, pos_ug.y + ug_building.y1 + 2, pos_ug.x + ug_building.x1 + 2, false);
    }

    for (auto i = 0; i < std::ssize(underground_buildings); i++) {
        const auto &ug_building = underground_buildings[i];
        generate_fill_perm_bold(player_ptr, pos_ug.y + ug_building.y0, pos_ug.x + ug_building.x0, pos_ug.y + ug_building.y1, pos_ug.x + ug_building.x1);

        /* Pick a door direction (S,N,E,W) */
        Pos2D pos(0, 0);
        switch (randint0(4)) {
        case 0: // Bottom side
            pos = { ug_building.y1, rand_range(ug_building.x0, ug_building.x1) };
            break;
        case 1: // Top side
            pos = { ug_building.y0, rand_range(ug_building.x0, ug_building.x1) };
            break;
        case 2: // Right side
            pos = { rand_range(ug_building.y0, ug_building.y1), ug_building.x1 };
            break;
        case 3: // Left side
            pos = { rand_range(ug_building.y0, ug_building.y1), ug_building.x0 };
            break;
        }

        const auto &terrains = TerrainList::get_instance();
        const auto end = terrains.end();
        const auto it = std::find_if(terrains.begin(), end,
            [subtype = stores[i]](const TerrainType &terrain) {
                return terrain.flags.has(TerrainCharacteristics::STORE) && (i2enum<StoreSaleType>(static_cast<int>(terrain.subtype)) == subtype);
            });
        if (it == end) {
            continue;
        }

        cave_set_feat(player_ptr, pos_ug.y + pos.y, pos_ug.x + pos.x, it->idx);
        store_init(VALID_TOWNS, stores[i]);
    }
}

/*!
 * @brief タイプ16の部屋…地下都市の生成 / Type 16 -- Underground Arcade
 * @details
 * Town logic flow for generation of new town\n
 * Originally from Vanilla 3.0.3\n
 *\n
 * We start with a fully wiped grids of normal floors.\n
 *\n
 * Note that town_gen_hack() plays games with the R.N.G.\n
 *\n
 * This function does NOT do anything about the owners of the stores,\n
 * nor the contents thereof.  It only handles the physical layout.\n
 */
bool build_type16(PlayerType *player_ptr, dun_data_type *dd_ptr)
{
    const auto town_hgt = rand_range(MIN_TOWN_HGT, MAX_TOWN_HGT);
    const auto town_wid = rand_range(MIN_TOWN_WID, MAX_TOWN_WID);
    const auto underground_buildings = precalc_ugarcade(town_hgt, town_wid);
    if (!underground_buildings) {
        return false;
    }

    int yval;
    int xval;
    if (!find_space(player_ptr, dd_ptr, &yval, &xval, town_hgt + 4, town_wid + 4)) {
        return false;
    }

    const Pos2D pos(yval - (town_hgt / 2), xval - (town_wid / 2));
    generate_room_floor(player_ptr, pos.y + town_hgt / 3, pos.x + town_wid / 3, pos.y + town_hgt * 2 / 3, pos.x + town_wid * 2 / 3, false);
    build_stores(player_ptr, pos, *underground_buildings);
    msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("地下街を生成しました", "Underground arcade was generated."));
    return true;
}
