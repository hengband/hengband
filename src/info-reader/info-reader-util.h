#pragma once

#include "util/bit-flags-calculator.h"
#include <concepts>
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
RandomArtActType grab_one_activation_flag(std::string_view what);

#ifndef JP
void append_english_text(std::string &text, std::string_view add);
#endif

/// @note clang-formatによるconceptの整形が安定していないので抑制しておく
// clang-format off
/*!
 * @brief 型Keyをキーとして持つような連想配列型のコンセプト
 * std::mapやstd::unordered_mapなどが該当する
 */
template <typename T, typename Key>
concept DictIndexedBy = requires(T t, Key k) {
    std::same_as<typename T::key_type, Key>;
    typename T::mapped_type;
    { t.find(k) } -> std::same_as<typename T::iterator>;
    { t.find(k)->second } -> std::convertible_to<typename T::mapped_type>;
    { t.end() } -> std::same_as<typename T::iterator>;
};
// clang-format on

/*!
 * @brief info文字列から定数を取得し、それを返す
 * @param dict 文字列辞書
 * @param what 文字列
 * @return 見つけたら定数を返す。見つからなければnulloptを返す
 */
template <typename Key, DictIndexedBy<Key> Dict>
std::optional<typename Dict::mapped_type> info_get_const(const Dict &dict, Key &&what)
{
    if (auto it = dict.find(what); it != dict.end()) {
        return it->second;
    }
    return std::nullopt;
}

/*!
 * @brief infoフラグ文字列をフラグビットに変換する
 * @param flags ビットフラグ変数
 * @param dict フラグ文字列変換表
 * @param what フラグ文字列
 * @return 見つけたらtrue
 */
template <typename Key, DictIndexedBy<Key> Dict>
bool info_grab_one_flag(uint32_t &flags, const Dict &dict, Key &&what)
{
    if (auto it = dict.find(what); it != dict.end()) {
        set_bits(flags, it->second);
        return true;
    }
    return false;
}

/*!
 * @brief info文字列を定数に変換する
 * @param buf 格納変数
 * @param dict 定数文字列変換表
 * @param what 定数文字列
 * @return 見つけたらtrue
 */
template <typename Key, DictIndexedBy<Key> Dict>
bool info_grab_one_const(uint32_t &buf, const Dict &dict, Key &&what)
{
    auto val = info_get_const(dict, std::forward<Key>(what));
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
