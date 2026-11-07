/*!
 * @brief 職業ごとのデータのセーブと読み込みのテスト
 */

#include "save/player-class-specific-data-writer.h"

#include "load/load-util.h"
#include "load/player-class-specific-data-loader.h"
#include "player-info/spell-hex-data-type.h"
#include "save/save-util.h"
#include "spell-realm/spells-hex.h"
#include "system/angband-version.h"
#include "test/save/scoped-save-io.h"
#include "test/scoped-restore.h"
#include "util/enum-converter.h"
#include "util/finalizer.h"
#include <cstdio>
#include <doctest/doctest.h>
#include <functional>
#include <memory>

namespace {
/*!
 * @brief 呪術のデータを書き込み、指定した版のセーブとして読み込む
 * @param write 呪術のデータを書き込む処理
 * @param version 読み込むときのセーブファイルの版
 * @return 読み込んだ呪術のデータ
 */
std::shared_ptr<spell_hex_data_type> write_and_read(const std::function<void()> &write, uint32_t version)
{
    auto *file = std::tmpfile();
    REQUIRE(file != nullptr);
    const auto close_file = util::make_finalizer([file] { std::fclose(file); });
    const auto restore_io = test::preserve_save_io();
    const auto restore_version = test::scoped_restore(loading_savefile_version);

    saving_savefile = file;
    save_xor_byte = 0;
    v_stamp = x_stamp = 0;
    write();
    wr_u16b(0x1234);

    std::rewind(file);
    loading_savefile = file;
    load_xor_byte = 0;
    v_check = x_check = 0;
    loading_savefile_version = version;
    auto loaded = std::make_shared<spell_hex_data_type>();
    PlayerClassSpecificDataLoader()(loaded);
    CHECK(rd_u16b() == 0x1234);
    return loaded;
}
}

TEST_CASE("Hex spells interrupted by dispel magic survive a save and load")
{
    spell_hex_data_type spell_hex_data;
    spell_hex_data.casting_spells.set(HEX_BLESS);
    spell_hex_data.revenge_type = SpellHexRevengeType::REVENGE;
    spell_hex_data.revenge_power = 123;
    spell_hex_data.revenge_turn = 4;
    spell_hex_data.interrupting_spells.set(HEX_CURE_LIGHT).set(HEX_DEMON_AURA);

    SUBCASE("The interrupted spells are restored with the other data")
    {
        const auto write = [&spell_hex_data] { PlayerClassSpecificDataWriter()(std::make_shared<spell_hex_data_type>(spell_hex_data)); };
        const auto loaded = write_and_read(write, SAVEFILE_VERSION);
        CHECK(loaded->casting_spells == spell_hex_data.casting_spells);
        CHECK(loaded->interrupting_spells == spell_hex_data.interrupting_spells);
        CHECK(loaded->revenge_type == SpellHexRevengeType::REVENGE);
        CHECK(loaded->revenge_power == 123);
        CHECK(loaded->revenge_turn == 4);
    }

    SUBCASE("A save older than version 31 has no interrupted spells")
    {
        // 版 30 までの書き方 (中断している呪術を書かない)
        const auto write = [&spell_hex_data] {
            wr_FlagGroup(spell_hex_data.casting_spells, wr_byte);
            wr_s32b(spell_hex_data.revenge_power);
            wr_byte(enum2i(spell_hex_data.revenge_type));
            wr_byte(spell_hex_data.revenge_turn);
        };
        const auto loaded = write_and_read(write, 30);
        CHECK(loaded->casting_spells == spell_hex_data.casting_spells);
        CHECK(loaded->interrupting_spells.none());
        CHECK(loaded->revenge_type == SpellHexRevengeType::REVENGE);
        CHECK(loaded->revenge_power == 123);
        CHECK(loaded->revenge_turn == 4);
    }
}
