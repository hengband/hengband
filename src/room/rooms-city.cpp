#include "room/rooms-city.h"
#include "game-option/cheat-types.h"
#include "grid/grid.h"
#include "room/space-finder.h"
#include "store/store.h"
#include "system/angband-exceptions.h"
#include "system/floor/floor-info.h"
#include "system/floor/town-list.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
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
tl::optional<std::vector<UndergroundBuilding>> precalc_ugarcade(int town_hgt, int town_wid)
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
        return tl::nullopt;
    }

    return underground_buildings;
}

/* Create a new floor room with optional light */
void generate_room_floor(PlayerType *player_ptr, const Rect2D &rectangle, int light)
{
    auto info = CAVE_ROOM;
    if (light) {
        info |= CAVE_GLOW;
    }

    for (const auto &pos : rectangle) {
        auto &grid = player_ptr->current_floor_ptr->get_grid(pos);
        place_grid(player_ptr, grid, GB_FLOOR);
        grid.add_info(info);
    }
}

void generate_fill_perm_bold(PlayerType *player_ptr, const Rect2D &rectangle)
{
    for (const auto &pos : rectangle) {
        place_bold(player_ptr, pos.y, pos.x, GB_INNER_PERM);
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
        const auto &rectangle = ug_building.get_outer_room(pos_ug);
        generate_room_floor(player_ptr, rectangle, false);
    }

    for (auto i = 0; i < std::ssize(underground_buildings); i++) {
        const auto &ug_building = underground_buildings[i];
        const auto &rectangle = ug_building.get_inner_room(pos_ug);
        generate_fill_perm_bold(player_ptr, rectangle);
        const auto vec = ug_building.pick_door_direction();
        const auto &terrains = TerrainList::get_instance();
        const auto end = terrains.end();
        const auto it = std::find_if(terrains.begin(), end,
            [subtype = stores[i]](const TerrainType &terrain) {
                return terrain.flags.has(TerrainCharacteristics::STORE) && (i2enum<StoreSaleType>(static_cast<int>(terrain.subtype)) == subtype);
            });
        if (it == end) {
            continue;
        }

        set_terrain_id_to_grid(player_ptr, pos_ug + vec, it->idx);
        store_init(VALID_TOWNS, stores[i]);
    }
}
}

UndergroundBuilding::UndergroundBuilding()
    : rectangle(Pos2D(0, 0), Pos2D(0, 0))
{
}

Pos2DVec UndergroundBuilding::pick_door_direction() const
{
    switch (randint0(4)) {
    case 0: // Bottom
        return { this->rectangle.bottom_right.y, rand_range(this->rectangle.top_left.x, this->rectangle.bottom_right.x) };
    case 1: // Top
        return { this->rectangle.top_left.y, rand_range(this->rectangle.top_left.x, this->rectangle.bottom_right.x) };
    case 2: // Right
        return { rand_range(this->rectangle.top_left.y, this->rectangle.bottom_right.y), this->rectangle.bottom_right.x };
    case 3: // Left
        return { rand_range(this->rectangle.top_left.y, this->rectangle.bottom_right.y), this->rectangle.top_left.x };
    default:
        THROW_EXCEPTION(std::logic_error, "RNG is broken!");
    }
}

void UndergroundBuilding::set_area(int height, int width, int max_height, int max_width)
{
    const Pos2D center(rand_range(2, height - 3), rand_range(2, width - 3));
    auto top = center.y - randint1(max_height);
    top = std::max(top, 1);
    auto left = center.x - randint1(max_width);
    left = std::max(left, 1);
    this->rectangle.top_left = { top, left };

    auto bottom = center.y + randint1(max_height);
    bottom = std::min(bottom, height - 2);
    auto right = center.x + randint1(max_width);
    right = std::min(right, width - 2);
    this->rectangle.bottom_right = { bottom, right };
}

bool UndergroundBuilding::is_area_used(const std::vector<std::vector<bool>> &ugarcade_used) const
{
    auto is_used = false;

    for (const auto &pos : this->rectangle) {
        if (ugarcade_used[pos.y][pos.x]) {
            is_used = true;
        }
    }

    return is_used;
}

void UndergroundBuilding::reserve_area(std::vector<std::vector<bool>> &ugarcade_used) const
{
    for (const auto &pos : this->rectangle.resized(1)) {
        ugarcade_used[pos.y][pos.x] = true;
    }
}

Rect2D UndergroundBuilding::get_outer_room(const Pos2D &pos_ug) const
{
    const auto top = pos_ug.y + this->rectangle.top_left.y - 2;
    const auto left = pos_ug.x + this->rectangle.top_left.x - 2;
    const auto bottom = pos_ug.y + this->rectangle.bottom_right.y + 2;
    const auto right = pos_ug.x + this->rectangle.bottom_right.x + 2;
    const Pos2D top_left(top, left);
    const Pos2D bottom_right(bottom, right);
    return { top_left, bottom_right };
}

Rect2D UndergroundBuilding::get_inner_room(const Pos2D &pos_ug) const
{
    const auto top = pos_ug.y + this->rectangle.top_left.y;
    const auto left = pos_ug.x + this->rectangle.top_left.x;
    const auto bottom = pos_ug.y + this->rectangle.bottom_right.y;
    const auto right = pos_ug.x + this->rectangle.bottom_right.x;
    const Pos2D top_left(top, left);
    const Pos2D bottom_right(bottom, right);
    return { top_left, bottom_right };
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
bool build_type16(PlayerType *player_ptr, DungeonData *dd_ptr)
{
    const auto town_hgt = rand_range(MIN_TOWN_HGT, MAX_TOWN_HGT);
    const auto town_wid = rand_range(MIN_TOWN_WID, MAX_TOWN_WID);
    const auto underground_buildings = precalc_ugarcade(town_hgt, town_wid);
    if (!underground_buildings) {
        return false;
    }

    const auto center = find_space(player_ptr, dd_ptr, town_hgt + 4, town_wid + 4);
    if (!center) {
        return false;
    }

    const Pos2D pos(center->y - (town_hgt / 2), center->x - (town_wid / 2));
    const Pos2DVec vec_top_left(town_hgt / 3, town_wid / 3);
    const auto top_left = pos + vec_top_left;
    const Pos2DVec vec_bottom_right(town_hgt * 2 / 3, town_wid * 2 / 3);
    const auto bottom_right = pos + vec_bottom_right;
    generate_room_floor(player_ptr, { top_left, bottom_right }, false);
    build_stores(player_ptr, pos, *underground_buildings);
    msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("地下街を生成しました", "Underground arcade was generated."));
    return true;
}
