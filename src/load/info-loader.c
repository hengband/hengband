#include "load/info-loader.h"
#include "game-option/runtime-arguments.h"
#include "load/angband-version-comparer.h"
#include "load/load-util.h"
#include "load/option-loader.h"
#include "system/angband.h"
#include "view/display-messages.h"
#include "world/world.h"

void rd_version_info(void)
{
    strip_bytes(4);
    load_xor_byte = current_world_ptr->sf_extra;
    v_check = 0L;
    x_check = 0L;

    /* Old savefile will be version 0.0.0.3 */
    rd_byte(&current_world_ptr->h_ver_extra);
    rd_byte(&current_world_ptr->h_ver_patch);
    rd_byte(&current_world_ptr->h_ver_minor);
    rd_byte(&current_world_ptr->h_ver_major);

    load_note(format(_("バージョン %d.%d.%d.%d のセーブ・ファイルをロード中...", "Loading a %d.%d.%d.%d savefile..."),
        (current_world_ptr->h_ver_major > 9) ? current_world_ptr->h_ver_major - 10 : current_world_ptr->h_ver_major, current_world_ptr->h_ver_minor,
        current_world_ptr->h_ver_patch, current_world_ptr->h_ver_extra));

    rd_u32b(&current_world_ptr->sf_system);
    rd_u32b(&current_world_ptr->sf_when);
    rd_u16b(&current_world_ptr->sf_lives);
    rd_u16b(&current_world_ptr->sf_saves);
}

/*!
 * @brief 乱数状態を読み込む / Read RNG state (added in 2.8.0)
 * @return なし
 */
void rd_randomizer(void)
{
    u16b tmp16u;
    rd_u16b(&tmp16u);
    rd_u16b(&Rand_place);
    for (int i = 0; i < RAND_DEG; i++)
        rd_u32b(&Rand_state[i]);
}

/*!
 * @brief メッセージログを読み込む / Read the saved messages
 * @return なし
 */
void rd_messages(void)
{
    if (h_older_than(2, 2, 0, 75)) {
        u16b num;
        rd_u16b(&num);
        int message_max;
        message_max = (int)num;

        for (int i = 0; i < message_max; i++) {
            char buf[128];
            rd_string(buf, sizeof(buf));
            message_add(buf);
        }
    }

    u32b num;
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
