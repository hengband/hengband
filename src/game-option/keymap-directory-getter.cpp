#include "game-option/keymap-directory-getter.h"
#include "game-option/input-options.h"
#include "io/input-key-requester.h"
#include "util/int-char-converter.h"

/*!
 * @brief キー入力された文字に対応する方向を取得する
 * @param ch キー入力された文字
 * @return キー入力された文字がが8方向いずれかに対応している場合、その方向を返す。それ以外の場合は無効な状態の方向を返す。
 */
Direction get_keymap_dir(char ch)
{
    auto dir = Direction::none();

    if (isdigit(ch)) {
        dir = Direction(D2I(ch));
    } else {
        const auto mode = rogue_like_commands ? KEYMAP_MODE_ROGUE : KEYMAP_MODE_ORIG;
        const auto act = keymap_act[mode][(byte)(ch)];
        if (!act) {
            return Direction::none();
        }
        for (auto s = act; *s; ++s) {
            if (isdigit(*s)) {
                dir = Direction(D2I(*s));
            }
        }
    }

    return dir.has_direction() ? dir : Direction::none();
}
