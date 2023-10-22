#pragma once

#include "monster-race/race-ability-flags.h"
#include "system/angband.h"
#include "util/bit-flags-calculator.h"
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

/*
 * Size of memory reserved for initialization of some arrays
 */
extern int error_idx; //!< エラーが発生したinfo ID

enum class RandomArtActType : short;
RandomArtActType grab_one_activation_flag(concptr what);

#ifndef JP
void append_english_text(std::string &text, std::string_view add);
#endif

/*!
 * @brief info文字列から定数を取得し、それを返す
 * @param names 文字列辞書
 * @param what 文字列
 * @return 見つけたら定数を返す。見つからなければnulloptを返す
 */
template <typename T, typename Map>
std::optional<T> info_get_const(const Map &names, std::string_view what)
{
    if (auto it = names.find(what); it != names.end()) {
        return it->second;
    }
    return std::nullopt;
}

/*!
 * @brief infoフラグ文字列をフラグビットに変換する
 * @param flags ビットフラグ変数
 * @param names フラグ文字列変換表
 * @param what フラグ文字列
 * @return 見つけたらtrue
 */
template <typename T>
bool info_grab_one_flag(uint32_t &flags, const std::unordered_map<std::string_view, T> &names, std::string_view what)
{
    if (auto it = names.find(what); it != names.end()) {
        set_bits(flags, it->second);
        return true;
    }
    return false;
}

/*!
 * @brief info文字列を定数に変換する
 * @param buf 格納変数
 * @param names 定数文字列変換表
 * @param what 定数文字列
 * @return 見つけたらtrue
 */
template <typename T>
bool info_grab_one_const(uint32_t &buf, const std::unordered_map<std::string_view, T> &names, std::string_view what)
{
    auto val = info_get_const<T>(names, what);
    if (val) {
        buf = static_cast<uint32_t>(*val);
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
