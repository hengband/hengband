#include "load/option-loader.h"
#include "cmd-io/cmd-gameoption.h"
#include "game-option/cheat-options.h"
#include "game-option/option-flags.h"
#include "game-option/special-options.h"
#include "load/angband-version-comparer.h"
#include "load/load-util.h"
#include "load/load-zangband.h"
#include "system/angband.h"
#include "util/bit-flags-calculator.h"
#include "world/world.h"

/*!
 * @brief ゲームオプションを読み込む / Read options (ignore most pre-2.8.0 options)
 * @details
 * Note that the normal options are now stored as a set of 256 bit flags,
 * plus a set of 256 bit masks to indicate which bit flags were defined
 * at the time the savefile was created.  This will allow new options
 * to be added, and old options to be removed, at any time, without
 * hurting old savefiles.
 *
 * The window options are stored in the same way, but note that each
 * window gets 32 options, and their order is fixed by certain defines.
 */
void rd_options(void)
{
    strip_bytes(16);

    if (loading_savefile_version_is_older_than(9)) {
        auto b = rd_byte();
        delay_factor = b * b * b;
    } else {
        delay_factor = rd_s32b();
    }

    hitpoint_warn = rd_byte();

    if (h_older_than(1, 7, 0, 0)) {
        mana_warn = 2;
    } else {
        mana_warn = rd_byte();
    }

    auto c = rd_u16b();

    cheat_peek = any_bits(c, 0x0100);
    cheat_hear = any_bits(c, 0x0200);
    cheat_room = any_bits(c, 0x0400);
    cheat_xtra = any_bits(c, 0x0800);
    cheat_know = any_bits(c, 0x1000);
    cheat_live = any_bits(c, 0x2000);
    cheat_save = any_bits(c, 0x4000);
    cheat_diary_output = any_bits(c, 0x8000);
    cheat_turn = any_bits(c, 0x0080);
    cheat_sight = any_bits(c, 0x0040);
    cheat_immortal = any_bits(c, 0x0020);

    autosave_l = rd_byte() != 0;
    autosave_t = rd_byte() != 0;
    autosave_freq = rd_s16b();

    BIT_FLAGS flag[8];
    for (int n = 0; n < 8; n++)
        flag[n] = rd_u32b();

    BIT_FLAGS mask[8];
    for (int n = 0; n < 8; n++)
        mask[n] = rd_u32b();

    for (auto n = 0; n < 8; n++) {
        for (auto i = 0; i < 32; i++) {
            if (none_bits(mask[n], 1U << i) || none_bits(option_mask[n], 1U << i)) {
                continue;
            }

            if (flag[n] & (1UL << i)) {
                option_flag[n] |= (1UL << i);
            } else {
                option_flag[n] &= ~(1UL << i);
            }
        }
    }

    if (h_older_than(0, 4, 5))
        load_zangband_options();

    extract_option_vars();
    for (int n = 0; n < 8; n++)
        flag[n] = rd_u32b();

    for (int n = 0; n < 8; n++)
        mask[n] = rd_u32b();

    for (int n = 0; n < 8; n++) {
        for (int i = 0; i < 32; i++) {
            if (!(mask[n] & (1UL << i)))
                continue;
            if (!(window_mask[n] & (1UL << i)))
                continue;

            if (flag[n] & (1UL << i)) {
                window_flag[n] |= (1UL << i);
            } else {
                window_flag[n] &= ~(1UL << i);
            }
        }
    }
}
