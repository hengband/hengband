#include "load/load-util.h"
#include "load/old/monster-loader-savefile50.h"
#include "monster/monster-flag-types.h"
#include "save/monster-entity-writer.h"
#include "save/save-util.h"
#include "system/angband-system.h"
#include "system/angband-version.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monster-entity.h"
#include "test/save/scoped-save-io.h"
#include "test/scoped-restore.h"
#include "util/finalizer.h"
#include <cstdio>
#include <doctest/doctest.h>

namespace {
MonsterEntity write_and_read(const MonsterEntity &monster)
{
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
    MonsterEntityWriter(monster).write_to_savedata();
    wr_u16b(0x1234);

    std::rewind(file);
    loading_savefile = file;
    load_xor_byte = 0;
    v_check = x_check = 0;
    loading_savefile_version = SAVEFILE_VERSION;
    system.set_version({ H_VER_MAJOR, H_VER_MINOR, H_VER_PATCH, H_VER_EXTRA });
    MonsterEntity loaded;
    loaded.mflag.set(MonsterTemporaryFlagType::LOS); // 読み込みで消えることを確かめるための古い値
    MonsterLoader50(nullptr).rd_monster(loaded);
    CHECK(rd_u16b() == 0x1234);
    return loaded;
}
}

TEST_CASE("Monster temporary flags survive a save and load")
{
    MonsterEntity monster;
    monster.r_idx = MonraceId::FILTHY_URCHIN;
    monster.ap_r_idx = MonraceId::FILTHY_URCHIN;
    monster.nickname = "Pochi";
    monster.parent_m_idx = 3;

    SUBCASE("The flags are restored with the data written after them")
    {
        monster.mflag.set(MonsterTemporaryFlagType::PREVENT_MAGIC).set(MonsterTemporaryFlagType::SANITY_BLAST);
        const auto loaded = write_and_read(monster);
        CHECK(loaded.mflag == monster.mflag);
        CHECK(loaded.nickname == "Pochi");
        CHECK(loaded.parent_m_idx == 3);
    }

    SUBCASE("A monster without the flags is loaded with none")
    {
        const auto loaded = write_and_read(monster);
        CHECK(loaded.mflag.none());
        CHECK(loaded.nickname == "Pochi");
    }
}

TEST_CASE("An empty monster slot is written and loaded as an empty slot")
{
    const auto loaded = write_and_read(MonsterEntity{});
    CHECK_FALSE(loaded.is_valid());
    CHECK(loaded.mflag.none());
}
