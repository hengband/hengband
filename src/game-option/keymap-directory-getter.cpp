#include "game-option/keymap-directory-getter.h"
#include "game-option/input-options.h"
#include "io/input-key-requester.h"
#include "system/angband.h"
#include "util/int-char-converter.h"

/*
 * GH
 * Called from cmd4.c and a few other places. Just extracts
 * a direction from the keymap for ch (the last direction,
 * in fact) byte or char here? I'm thinking that keymaps should
 * generally only apply to single keys, which makes it no more
 * than 128, so a char should suffice... but keymap_act is 256...
 */
int get_keymap_dir(char ch)
{
    int d = 0;

    if (isdigit(ch)) {
        d = D2I(ch);
    } else {
        BIT_FLAGS mode;
        if (rogue_like_commands) {
            mode = KEYMAP_MODE_ROGUE;
        } else {
            mode = KEYMAP_MODE_ORIG;
        }

        concptr act = keymap_act[mode][(byte)(ch)];
        if (act) {
            for (concptr s = act; *s; ++s) {
                if (isdigit(*s)) {
                    d = D2I(*s);
                }
            }
        }
    }

    if (d == 5) {
        d = 0;
    }

    return d;
}
