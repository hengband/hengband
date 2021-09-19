#pragma once

#include "system/angband.h"
#include "util/bit-flags-calculator.h"
#include <string>
#include <string_view>
#include <unordered_map>

/*
 * Size of memory reserved for initialization of some arrays
 */
extern int error_idx; //!< エラーが発生したinfo ID
extern int error_line; //!< エラーが発生した行

byte grab_one_activation_flag(concptr what);

using sview = std::string_view;

/*!
 * @brief infoフラグ文字列をフラグビットに変換する
 * @param flags ビットフラグ変数
 * @param names フラグ文字列変換表
 * @param what フラグ文字列
 * @return 見つけたらtrue
 */
template <typename T>
bool info_grab_one_flag(uint32_t &flags, const std::unordered_map<sview, T> &names, sview what)
{
    if (auto it = names.find(what); it != names.end()) {
        set_bits(flags, it->second);
        return true;
    }
    return false;
}

/*!
 * @brief infoパラメータに値をセットする
 * @param パラメータ変数
 * @val 値
 */
template <typename T>
void info_set_value(T &arg, const std::string &val, int base = 10)
{
    arg = static_cast<T>(std::stoi(val, nullptr, base));
}
