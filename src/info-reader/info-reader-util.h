#pragma once

#include "system/angband.h"
#include "util/bit-flags-calculator.h"
#include <concepts>
#include <cstdint>
#include <string_view>
#include <tl/optional.hpp>
#include <type_traits>
#include <utility>

extern int error_idx; //!< エラーが発生したinfo ID

enum class RandomArtActType : short;
RandomArtActType grab_one_activation_flag(std::string_view what);

/*!
 * @brief 型Keyをキーとして持つような連想配列型のコンセプト
 * std::mapやstd::unordered_mapなどが該当する
 */
template <typename T, typename Key>
concept DictIndexedBy = requires(T t, Key k) {
    requires std::is_constructible_v<typename T::key_type, std::remove_cvref_t<Key>>;
    typename T::mapped_type;
    { t.find(k) } -> std::same_as<typename T::iterator>;
    { t.find(k)->second } -> std::convertible_to<typename T::mapped_type>;
    { t.end() } -> std::same_as<typename T::iterator>;
};

/*!
 * @brief info文字列から定数を取得し、それを返す
 * @param dict 文字列辞書
 * @param what 文字列
 * @return 見つけたら定数を返す。見つからなければnulloptを返す
 */
template <typename Key, DictIndexedBy<Key> Dict>
tl::optional<typename Dict::mapped_type> info_get_const(const Dict &dict, Key &&what)
{
    if (auto it = dict.find(what); it != dict.end()) {
        return it->second;
    }
    return tl::nullopt;
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
