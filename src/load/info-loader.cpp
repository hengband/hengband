#include "load/info-loader.h"
#include "game-option/runtime-arguments.h"
#include "load/angband-version-comparer.h"
#include "load/load-util.h"
#include "load/option-loader.h"
#include "locale/character-encoding.h"
#include "system/angband-system.h"
#include "system/angband-version.h"
#include "system/angband.h"
#include "util/enum-converter.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief セーブファイルからバージョン情報及びセーブ情報を取得する
 * @details
 * バージョン0.x.x時代のバージョン情報である場合、サポート対象外
 * (FAKE_VERもH_VERも10台の数字のはず)
 */
void rd_version_info(void)
{
    auto tmp_major = rd_byte();
    auto is_old_ver = (10 <= tmp_major) && (tmp_major <= 13);
    constexpr auto variant_length = VARIANT_NAME.length();
    auto &system = AngbandSystem::get_instance();
    if (tmp_major == variant_length) {
        strip_bytes(variant_length);
        load_xor_byte = 0;
        const auto major = rd_byte();
        const auto minor = rd_byte();
        const auto patch = rd_byte();
        const auto extra = rd_byte();
        system.set_version({ major, minor, patch, extra });
        strip_bytes(1);
    } else if (is_old_ver) {
        strip_bytes(3);
    } else {
        THROW_EXCEPTION(std::runtime_error, _("異常なバージョンが検出されました！", "Invalid version is detected!"));
    }

    load_xor_byte = system.savefile_key;
    v_check = 0L;
    x_check = 0L;

    if (is_old_ver) {
        /* Old savefile will be version 0.0.0.3 */
        const auto major = rd_byte();
        const auto minor = rd_byte();
        const auto patch = rd_byte();
        const auto extra = rd_byte();
        system.set_version({ major, minor, patch, extra });
    }

    auto &world = AngbandWorld::get_instance();
    world.sf_system = rd_u32b();
    world.sf_when = rd_u32b();
    world.sf_lives = rd_u16b();
    world.sf_saves = rd_u16b();

    loading_savefile_version = rd_u32b();

    /* h_ver_majorがfake_ver_majorと同じだったころへの対策 */
    if (loading_savefile_version_is_older_than(10)) {
        constexpr auto fake_ver_plus = 10;
        auto &version = system.get_version();
        if (tmp_major - version.major < fake_ver_plus) {
            version.major -= fake_ver_plus;
        }
    }

    constexpr auto fmt = _("バージョン %s のセーブデータ(SAVE%u形式)をロード中...", "Loading a version %s savefile (SAVE%u format)...");
    load_note(format(fmt, system.build_version_expression(VersionExpression::WITHOUT_EXTRA).data(), loading_savefile_version));
}

/*!
 * @brief 乱数状態を読み込む / Read RNG state (added in 2.8.0)
 */
void rd_randomizer(void)
{
    strip_bytes(4);
    Xoshiro128StarStar::state_type state{};
    for (auto &s : state) {
        s = rd_u32b();
    }

    Xoshiro128StarStar game_rng;
    game_rng.set_state(state);
    AngbandSystem::get_instance().set_rng(game_rng);
    strip_bytes(4 * (RAND_DEG - state.size()));
}

/*!
 * @brief メッセージログを読み込む / Read the saved messages
 */
void rd_messages(void)
{
    if (!loading_savefile_version_is_older_than(22)) {
        rd_message_history();
        return;
    }

    const auto message_max = static_cast<int>(h_older_than(2, 2, 0, 75) ? rd_u16b() : rd_u32b());
    for (int i = 0; i < message_max; i++) {
        message_add(rd_string());
    }
}

void rd_system_info(void)
{
    loading_character_encoding = i2enum<CharacterEncoding>(rd_byte());
    rd_randomizer();
    load_note(_("乱数情報をロードしました", "Loaded Randomizer Info"));
    rd_options();
    load_note(_("オプションをロードしました", "Loaded Option Flags"));
    rd_messages();
    load_note(_("メッセージをロードしました", "Loaded Messages"));
}
