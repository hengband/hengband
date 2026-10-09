#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item/item-entity.h"
#include "system/monster-entity.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "util/finalizer.h"
#include <algorithm>
#include <doctest/doctest.h>
#include <initializer_list>
#include <memory>

TEST_CASE("Walls between the old and new player rows and columns are redrawn")
{
    auto &terrains = TerrainList::get_instance();
    const auto old_size = terrains.size();
    const auto restore = util::make_finalizer([&terrains, old_size] { terrains.resize(old_size); });
    terrains.resize(old_size + 2);
    const auto floor_id = static_cast<short>(old_size);
    const auto wall_id = static_cast<short>(old_size + 1);
    auto &plain = terrains.get_terrain(floor_id);
    plain.idx = floor_id;
    plain.mimic = floor_id;
    plain.flags.clear();
    plain.flags.set(TerrainCharacteristics::FLOOR).set(TerrainCharacteristics::LOS).set(TerrainCharacteristics::PROJECTION);
    auto &wall = terrains.get_terrain(wall_id);
    wall.idx = wall_id;
    wall.mimic = wall_id;
    wall.flags.clear();
    wall.flags.set(TerrainCharacteristics::WALL).set(TerrainCharacteristics::REMEMBER);

    auto floor_ptr = std::make_unique<FloorType>();
    auto &floor = *floor_ptr;
    floor.width = 9;
    floor.height = 9;
    for (const auto &pos : floor.get_area()) {
        floor.get_grid(pos).feat = floor_id;
    }

    const auto check_marked = [&](const Pos2D &p_pos_old, const Pos2D &p_pos_new, std::initializer_list<Pos2D> walls_in_view, std::initializer_list<Pos2D> expected) {
        for (const auto &pos : walls_in_view) {
            floor.get_grid(pos).feat = wall_id;
            floor.set_view_at(pos);
        }

        floor.set_note_and_redraw_walls_lit_from_player_side(p_pos_old, p_pos_new);
        const auto points = floor.collect_redraw_points();
        CHECK(points.size() == expected.size());
        for (const auto &pos : walls_in_view) {
            const auto is_expected = std::find(expected.begin(), expected.end(), pos) != expected.end();
            CHECK(((floor.get_grid(pos).info & CAVE_NOTE) != 0) == is_expected);
            CHECK((std::find(points.begin(), points.end(), pos) != points.end()) == is_expected);
        }
    };

    SUBCASE("A step marks walls on the old and new rows and columns")
    {
        const Pos2D wall_on_row{ 4, 7 };
        const Pos2D wall_on_old_column{ 1, 3 };
        const Pos2D wall_on_new_column{ 7, 4 };
        const Pos2D wall_off_lines{ 1, 1 };
        check_marked({ 4, 3 }, { 4, 4 }, { wall_on_row, wall_on_old_column, wall_on_new_column, wall_off_lines }, { wall_on_row, wall_on_old_column, wall_on_new_column });
    }

    SUBCASE("A long move also marks walls on the rows and columns in between")
    {
        const Pos2D wall_on_middle_row{ 3, 7 };
        const Pos2D wall_on_middle_column{ 7, 3 };
        const Pos2D wall_off_lines{ 7, 7 };
        check_marked({ 1, 1 }, { 5, 5 }, { wall_on_middle_row, wall_on_middle_column, wall_off_lines }, { wall_on_middle_row, wall_on_middle_column });
    }

    SUBCASE("Floors and walls out of view are not marked")
    {
        const Pos2D floor_on_row{ 4, 1 };
        floor.set_view_at(floor_on_row);
        floor.get_grid({ 7, 3 }).feat = wall_id;
        check_marked({ 4, 3 }, { 4, 4 }, {}, {});
        CHECK((floor.get_grid(floor_on_row).info & CAVE_NOTE) == 0);
        CHECK((floor.get_grid({ 7, 3 }).info & CAVE_NOTE) == 0);
    }
}
