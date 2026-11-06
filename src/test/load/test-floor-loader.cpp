#include "load/floor-loader.h"
#include "load/load-util.h"
#include "save/floor-writer.h"
#include "save/save-util.h"
#include "system/angband-system.h"
#include "system/angband-version.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "test/save/scoped-save-io.h"
#include "test/scoped-restore.h"
#include "test/system/monrace-list-test-access.h"
#include "util/finalizer.h"
#include <cstdio>
#include <doctest/doctest.h>
#include <memory>

TEST_CASE("An empty monster slot in a saved floor is kept without a grid, a count or a population")
{
    test::MonraceListTestAccess monraces_guard;
    auto &monraces = MonraceList::get_instance();
    monraces.emplace(MonraceId::PLAYER);
    monraces.emplace(MonraceId::FILTHY_URCHIN);

    auto floor_ptr = std::make_unique<FloorType>();
    auto &floor = *floor_ptr;
    PlayerType player;
    player.current_floor_ptr = &floor;
    floor.width = 5;
    floor.height = 5;
    floor.o_list.resize(1); // 0 番のダミーだけにして、アイテムを書かない

    // 1 番と 3 番が生きていて、2 番は倒されたモンスターの跡 (空きの番号)
    const Pos2D pos1{ 1, 1 };
    const Pos2D pos3{ 2, 3 };
    for (const auto &[m_idx, pos] : { std::pair{ 1, pos1 }, std::pair{ 3, pos3 } }) {
        auto &monster = floor.m_list[m_idx];
        monster.r_idx = MonraceId::FILTHY_URCHIN;
        monster.ap_r_idx = MonraceId::FILTHY_URCHIN;
        monster.fy = pos.y;
        monster.fx = pos.x;
        floor.get_grid(pos).m_idx = static_cast<short>(m_idx);
    }

    floor.m_max = 4;
    floor.m_cnt = 2;

    auto *file = std::tmpfile();
    REQUIRE(file != nullptr);
    const auto close_file = util::make_finalizer([file] { std::fclose(file); });
    const auto restore_io = test::preserve_save_io();
    const auto restore_version = test::scoped_restore(loading_savefile_version);
    auto &system = AngbandSystem::get_instance();
    const auto restore_system_version = util::make_finalizer([&system, old_version = system.get_version()] { system.set_version(old_version); });

    saving_savefile = file;
    save_xor_byte = 0;
    v_stamp = x_stamp = 0;
    wr_saved_floor(&player, nullptr);

    std::rewind(file);
    loading_savefile = file;
    load_xor_byte = 0;
    v_check = x_check = 0;
    loading_savefile_version = SAVEFILE_VERSION;
    system.set_version({ H_VER_MAJOR, H_VER_MINOR, H_VER_PATCH, H_VER_EXTRA });
    REQUIRE(rd_saved_floor(&player, nullptr) == 0);

    CHECK(floor.m_max == 4);
    CHECK(floor.m_cnt == 2);
    CHECK(floor.m_list[1].is_valid());
    CHECK_FALSE(floor.m_list[2].is_valid());
    CHECK(floor.m_list[3].is_valid());
    CHECK(floor.get_grid(pos1).m_idx == 1);
    CHECK(floor.get_grid(pos3).m_idx == 3);
    CHECK(floor.get_grid({ 0, 0 }).m_idx == 0);
    CHECK(monraces.get_monrace(MonraceId::FILTHY_URCHIN).cur_num == 2);
    CHECK(monraces.get_monrace(MonraceId::PLAYER).cur_num == 0);
}
