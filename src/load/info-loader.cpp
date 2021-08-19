#include "load/info-loader.h"
#include "game-option/runtime-arguments.h"
#include "load/angband-version-comparer.h"
#include "load/load-util.h"
#include "load/option-loader.h"
#include "system/angband.h"
#include "system/angband-version.h"
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
    byte fake_major;
    rd_byte(&fake_major);

    strip_bytes(3);
    load_xor_byte = current_world_ptr->sf_extra;
    v_check = 0L;
    x_check = 0L;

    /* Old savefile will be version 0.0.0.3 */
    rd_byte(&current_world_ptr->h_ver_extra);
    rd_byte(&current_world_ptr->h_ver_patch);
    rd_byte(&current_world_ptr->h_ver_minor);
    rd_byte(&current_world_ptr->h_ver_major);

    rd_u32b(&current_world_ptr->sf_system);
    rd_u32b(&current_world_ptr->sf_when);
    rd_u16b(&current_world_ptr->sf_lives);
    rd_u16b(&current_world_ptr->sf_saves);

    rd_u32b(&loading_savefile_version);

    /* h_ver_majorがfake_ver_majorと同じだったころへの対策 */
    if (fake_major - current_world_ptr->h_ver_major < FAKE_VER_PLUS)
        current_world_ptr->h_ver_major -= FAKE_VER_PLUS;

    load_note(format(_("バージョン %d.%d.%d のセーブデータ(SAVE%lu形式)をロード中...", "Loading a Verison %d.%d.%d savefile (SAVE%lu format)..."),
        current_world_ptr->h_ver_major, current_world_ptr->h_ver_minor, current_world_ptr->h_ver_patch,
        loading_savefile_version));
}

/*!
 * @brief 乱数状態を読み込む / Read RNG state (added in 2.8.0)
 */
void rd_randomizer(void)
{
    uint16_t tmp16u;
    rd_u16b(&tmp16u);
    rd_u16b(&Rand_place);
    for (int i = 0; i < RAND_DEG; i++)
        rd_u32b(&Rand_state[i]);
}

/*!
 * @brief メッセージログを読み込む / Read the saved messages
 */
void rd_messages(void)
{
    if (h_older_than(2, 2, 0, 75)) {
        uint16_t num;
        rd_u16b(&num);
        int message_max;
        message_max = (int)num;

        for (int i = 0; i < message_max; i++) {
            char buf[128];
            rd_string(buf, sizeof(buf));
            message_add(buf);
        }
    }

    uint32_t num;
    rd_u32b(&num);
    int message_max = (int)num;
    for (int i = 0; i < message_max; i++) {
        char buf[128];
        rd_string(buf, sizeof(buf));
        message_add(buf);
    }
}

void rd_system_info(void)
{
    rd_byte(&kanji_code);
    rd_randomizer();
    if (arg_fiddle)
        load_note(_("乱数情報をロードしました", "Loaded Randomizer Info"));

    rd_options();
    if (arg_fiddle)
        load_note(_("オプションをロードしました", "Loaded Option Flags"));

    rd_messages();
    if (arg_fiddle)
        load_note(_("メッセージをロードしました", "Loaded Messages"));
}
