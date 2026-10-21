#pragma once

#include "system/angband.h"
#include "system/h-type.h"
#include "util/bit-flags-calculator.h"
#include <concepts>
#include <cstdint>
#include <limits>
#include <string_view>
#include <tl/optional.hpp>
#include <type_traits>
#include <utility>

extern int error_idx; //!< エラーが発生したinfo ID

enum class FixedArtifactId : short;
enum class MonraceId : short;
enum class QuestId : short;
enum class RandomArtActType : short;

/*
 * 定義のIDの上限は、IDを格納する型の上限−1とする。
 * IDの最大値+1になる件数（種族の数など）も同じ型に収まるようにするため。
 */
constexpr int MAX_BASEITEM_ID = std::numeric_limits<short>::max() - 1; //!< ベースアイテムIDの上限 (BaseitemList は short でIDを扱う)
constexpr int MAX_MONRACE_ID = std::numeric_limits<std::underlying_type_t<MonraceId>>::max() - 1; //!< モンスター種族IDの上限
constexpr int MAX_FIXED_ARTIFACT_ID = std::numeric_limits<std::underlying_type_t<FixedArtifactId>>::max() - 1; //!< 固定アーティファクトIDの上限
constexpr int MAX_TERRAIN_ID = std::numeric_limits<FEAT_IDX>::max() - 1; //!< 地形IDの上限
constexpr int MAX_QUEST_ID = std::numeric_limits<std::underlying_type_t<QuestId>>::max() - 1; //!< クエストIDの上限

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
