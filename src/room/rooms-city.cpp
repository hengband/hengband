#include "room/rooms-city.h"
#include "floor/floor-generator.h"
#include "floor/floor-town.h"
#include "game-option/cheat-types.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "room/space-finder.h"
#include "store/store-util.h"
#include "store/store.h"
#include "system/angband-exceptions.h"
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

/*
 * Precalculate buildings' location of underground arcade
 */
std::optional<std::vector<UndergroundBuilding>> precalc_ugarcade(int town_hgt, int town_wid)
{
    const auto n = std::ssize(stores);
    std::vector<UndergroundBuilding> underground_buildings(n);
    const auto max_buildings_height = 3 * town_hgt / MAX_TOWN_HGT;
    const auto max_buildings_width = 5 * town_wid / MAX_TOWN_WID;
    std::vector<std::vector<bool>> ugarcade_used(town_hgt, std::vector<bool>(town_wid));
    int i;
    auto attempt = 10000;
    auto should_abort = false;
    for (i = 0; i < n; i++) {
        auto &underground_building = underground_buildings[i];
        do {
            underground_building.set_area(town_hgt, town_wid, max_buildings_height, max_buildings_width);
            should_abort = underground_building.is_area_used(ugarcade_used);
            attempt--;
        } while (should_abort && (attempt > 0));

        if (attempt == 0) {
            break;
        }

        underground_building.reserve_area(ugarcade_used);
    }

    if (i != n) {
        return std::nullopt;
    }

    return underground_buildings;
}

/* Create a new floor room with optional light */
void generate_room_floor(PlayerType *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2, int light)
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

void generate_fill_perm_bold(PlayerType *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2)
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
void build_stores(PlayerType *player_ptr, const Pos2D &pos_ug, const std::vector<UndergroundBuilding> &underground_buildings)
{
    for (const auto &ug_building : underground_buildings) {
        const auto &[north_west, south_east] = ug_building.get_room_positions(pos_ug);
        generate_room_floor(player_ptr, north_west.y, north_west.x, south_east.y, south_east.x, false);
    }

    for (auto i = 0; i < std::ssize(underground_buildings); i++) {
        const auto &ug_building = underground_buildings[i];
        const auto &[north_west, south_east] = ug_building.get_inner_room_positions(pos_ug);
        generate_fill_perm_bold(player_ptr, north_west.y, north_west.x, south_east.y, south_east.x);
        const auto pos = ug_building.pick_door_direction();
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
}

UndergroundBuilding::UndergroundBuilding()
    : north_west(Pos2D(0, 0))
    , south_east(Pos2D(0, 0))
{
}

Pos2D UndergroundBuilding::pick_door_direction() const
{
    switch (randint0(4)) {
    case 0: // South
        return { this->south_east.y, rand_range(this->north_west.x, this->south_east.x) };
    case 1: // North
        return { this->north_west.y, rand_range(this->north_west.x, this->south_east.x) };
    case 2: // East
        return { rand_range(this->north_west.y, this->south_east.y), this->south_east.x };
    case 3: // West
        return { rand_range(this->north_west.y, this->south_east.y), this->north_west.x };
    default:
        THROW_EXCEPTION(std::logic_error, "RNG is broken!");
    }
}

void UndergroundBuilding::set_area(int height, int width, int max_height, int max_width)
{
    const Pos2D center(rand_range(2, height - 3), rand_range(2, width - 3));
    auto north_west_y = center.y - randint1(max_height);
    north_west_y = std::max(north_west_y, 1);
    auto north_west_x = center.x - randint1(max_width);
    north_west_x = std::max(north_west_x, 1);
    this->north_west = { north_west_y, north_west_x };

    auto south_east_y = center.y + randint1(max_height);
    south_east_y = std::min(south_east_y, height - 2);
    auto south_east_x = center.x + randint1(max_width);
    south_east_x = std::min(south_east_x, width - 2);
    this->south_east = { south_east_y, south_east_x };
}

bool UndergroundBuilding::is_area_used(const std::vector<std::vector<bool>> &ugarcade_used) const
{
    for (auto y = this->north_west.y; (y <= this->south_east.y); y++) {
        for (auto x = this->north_west.x; x <= this->south_east.x; x++) {
            if (ugarcade_used[y][x]) {
                return true;
            }
        }
    }

    return false;
}

void UndergroundBuilding::reserve_area(std::vector<std::vector<bool>> &ugarcade_used) const
{
    for (auto y = this->north_west.y - 1; y <= this->south_east.y + 1; y++) {
        for (auto x = this->north_west.x - 1; x <= this->south_east.x + 1; x++) {
            ugarcade_used[y][x] = true;
        }
    }
}

std::pair<Pos2D, Pos2D> UndergroundBuilding::get_room_positions(const Pos2D &pos_ug) const
{
    const auto y1 = pos_ug.y + this->north_west.y - 2;
    const auto x1 = pos_ug.x + this->north_west.x - 2;
    const auto y2 = pos_ug.y + this->south_east.y + 2;
    const auto x2 = pos_ug.x + this->south_east.x + 2;
    return { { y1, x1 }, { y2, x2 } };
}

std::pair<Pos2D, Pos2D> UndergroundBuilding::get_inner_room_positions(const Pos2D &pos_ug) const
{
    const auto y1 = pos_ug.y + this->north_west.y;
    const auto x1 = pos_ug.x + this->north_west.x;
    const auto y2 = pos_ug.y + this->south_east.y;
    const auto x2 = pos_ug.x + this->south_east.x;
    return { { y1, x1 }, { y2, x2 } };
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
