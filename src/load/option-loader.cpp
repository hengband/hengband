#include "load/option-loader.h"
#include "cmd-io/cmd-gameoption.h"
#include "game-option/cheat-options.h"
#include "game-option/option-flags.h"
#include "game-option/special-options.h"
#include "load/angband-version-comparer.h"
#include "load/load-util.h"
#include "load/load-zangband.h"
#include "system/angband.h"
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

    byte b;
    rd_byte(&b);
    delay_factor = b;

    rd_byte(&b);
    hitpoint_warn = b;

    if (h_older_than(1, 7, 0, 0)) {
        mana_warn = 2;
    } else {
        rd_byte(&b);
        mana_warn = b;
    }

    uint16_t c;
    rd_u16b(&c);

    if (c & 0x0002)
        current_world_ptr->wizard = true;

    cheat_peek = (c & 0x0100) ? true : false;
    cheat_hear = (c & 0x0200) ? true : false;
    cheat_room = (c & 0x0400) ? true : false;
    cheat_xtra = (c & 0x0800) ? true : false;
    cheat_know = (c & 0x1000) ? true : false;
    cheat_live = (c & 0x2000) ? true : false;
    cheat_save = (c & 0x4000) ? true : false;
    cheat_diary_output = (c & 0x8000) ? true : false;
    cheat_turn = (c & 0x0080) ? true : false;
    cheat_sight = (c & 0x0040) ? true : false;
    cheat_immortal = (c & 0x0020) ? true : false;

    rd_byte((byte *)&autosave_l);
    rd_byte((byte *)&autosave_t);
    rd_s16b(&autosave_freq);

    BIT_FLAGS flag[8];
    for (int n = 0; n < 8; n++)
        rd_u32b(&flag[n]);

    BIT_FLAGS mask[8];
    for (int n = 0; n < 8; n++)
        rd_u32b(&mask[n]);

    for (int n = 0; n < 8; n++) {
        for (int i = 0; i < 32; i++) {
            if (!(mask[n] & (1UL << i)))
                continue;
            if (!(option_mask[n] & (1UL << i)))
                continue;

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
        rd_u32b(&flag[n]);

    for (int n = 0; n < 8; n++)
        rd_u32b(&mask[n]);

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
