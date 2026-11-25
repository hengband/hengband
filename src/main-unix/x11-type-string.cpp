/*!
 * @file x11-type-string.cpp
 * @brief X11用の文字列処理
 * @date 2020/06/14
 * @author Hourier
 * @details Windowsでは使わない
 */

#include "main-unix/x11-type-string.h"
#include "term/gameterm.h"
#include "term/z-term.h"
#include <string_view>

/*!
 * @brief 文字列を打鍵としてメインの端末のキューの末尾へ積む
 * @param str 積む文字列
 * @return 全て積めた場合0、空きが足りず1つも積まなかった場合は非0
 * @details 積んだキーはマクロやキー配置の対象になる。途中に NUL があれば、その前までを積む。
 */
errr type_string(std::string_view str)
{
    const auto keys = str.substr(0, str.find('\0'));
    term_type *old = game_term;
    term_activate(term_screen);
    const auto err = term_keys_append(keys);
    term_activate(old);
    return err;
}
