#pragma once

#include <concepts>
#include <iterator>
#include <range/v3/range/concepts.hpp>
#include <type_traits>

/*!
 * @brief enum もしくは enum class の列挙値の範囲(半開区間)を扱うクラス
 *
 * @tparam EnumType 対象となる列挙型(enum もしくは enum class)
 */
template <typename EnumType>
    requires std::is_enum_v<EnumType>
class EnumRange {
public:
    /*!
     * @brief 列挙値の範囲のイテレータクラス
     */
    class iterator {
    public:
        // std::iterator_traits に対応するための定義
        using difference_type = int;
        using value_type = EnumType;
        using iterator_concept = std::input_iterator_tag;

        constexpr iterator() noexcept = default;

        /*!
         * @brief 引数で与えた列挙値を指すイテレータオブジェクトを生成する
         *
         * @param val イテレータオブジェクトが指す列挙値
         */
        constexpr iterator(EnumType val) noexcept
            : index(std::underlying_type_t<EnumType>(val))
        {
        }

        /*!
         * @brief イテレータが指している列挙値を取得する
         *
         * @return イテレータが指している列挙値
         */
        constexpr EnumType operator*() const noexcept
        {
            return static_cast<EnumType>(index);
        }

        /*!
         * @brief イテレータを前置インクリメントする
         *
         * @return *this の参照
         */
        constexpr iterator &operator++() noexcept
        {
            ++index;
            return *this;
        }

        /*!
         * @brief イテレータを後置インクリメントする
         *
         * @return *this の参照
         */
        constexpr iterator operator++(int) noexcept
        {
            auto old = *this;
            ++*this;
            return old;
        }

        /*!
         * @brief 2つのイテレータが指している列挙値が等しいかどうか調べる
         *
         * @param other 比較対象となるイテレータ
         * @return 2つのイテレータが指している列挙値が等しければ true、そうでなければ false
         */
        constexpr bool operator==(const iterator &other) const noexcept = default;

    private:
        //! 現在イテレータが指している列挙値の基底型における整数値
        std::underlying_type_t<EnumType> index;
    };

    using value_type = EnumType;
    using const_iterator = iterator;

    /*!
     * @brief 引数で与えた範囲(半開区間)のオブジェクトを生成する
     * @details 生成する範囲はfirstからlastの1つ前の値(lastを含まない、半開区間)。
     * 範囲のイテレーションは基底型の整数値をインクリメントする事によって行うので、
     * first から last までの間の整数値が飛んでいる場合、その部分は具体的な定義の無い列挙値がイテレートされる。
     * @param first 範囲の最初となる列挙値
     * @param last 範囲の最後となる列挙値の次の値(lastは範囲に含まない)
     */
    constexpr EnumRange(EnumType first, EnumType last) noexcept
        : begin_val(first)
        , end_val(last)
    {
    }

    /*!
     * @brief 引数で与えた列挙値が範囲に含まれるかどうか調べる
     *
     * @param val 調べる列挙値
     * @return true 引数で与えた列挙値が範囲に含まれる
     * @return false 引数で与えた列挙値が範囲に含まれない
     */
    constexpr bool contains(EnumType val) const noexcept
    {
        return begin_val <= val && val < end_val;
    }

    /*!
     * @brief 範囲の最初の列挙値を指すイテレータを取得する
     *
     * @return 範囲の最初の列挙値を指すイテレータ
     */
    constexpr iterator begin() const noexcept
    {
        return iterator(begin_val);
    }

    /*!
     * @brief 範囲の最後の列挙値の次の値を指すイテレータを取得する
     *
     * @return 範囲の最後の列挙値の次の値を指すイテレータ
     */
    constexpr iterator end() const noexcept
    {
        return iterator(end_val);
    }

    /*!
     * @brief 範囲に含まれる列挙値の種類数を取得する
     *
     * @return 範囲に含まれる列挙値の種類数
     */
    constexpr std::size_t size() const noexcept
    {
        return static_cast<std::size_t>(end_val) - static_cast<std::size_t>(begin_val);
    }

private:
    EnumType begin_val;
    EnumType end_val;
};

/*!
 * @brief enum もしくは enum class の列挙値の範囲(閉区間)を扱うクラス
 *
 * @tparam EnumType 対象となる列挙型(enum もしくは enum class)
 */
template <typename EnumType>
    requires std::is_enum_v<EnumType>
class EnumRangeInclusive {
public:
    using value_type = EnumType;
    using iterator = typename EnumRange<EnumType>::iterator;
    using const_iterator = iterator;

    /*!
     * @brief 引数で与えた範囲(閉区間)のオブジェクトを生成する
     * @details 生成する範囲はfirstからlastまで(lastを含む、閉区間)。
     * 範囲の最終値+1の列挙値が存在しない場合や、列挙値の一部の範囲を指定したい場合に使用する。
     * 範囲のイテレーションは基底型の整数値をインクリメントする事によって行うので、
     * first から last までの間の整数値が飛んでいる場合、その部分は具体的な定義の無い列挙値がイテレートされる。
     * @param first 範囲の最初となる列挙値
     * @param last 範囲の最後となる列挙値(lastも範囲に含む)
     */
    constexpr EnumRangeInclusive(EnumType first, EnumType last) noexcept
        : range(first, static_cast<EnumType>(std::underlying_type_t<EnumType>(last) + 1))
    {
    }

    /*!
     * @brief 引数で与えた列挙値が範囲に含まれるかどうか調べる
     *
     * @param val 調べる列挙値
     * @return true 引数で与えた列挙値が範囲に含まれる
     * @return false 引数で与えた列挙値が範囲に含まれない
     */
    constexpr bool contains(EnumType val) const noexcept
    {
        return range.contains(val);
    }

    /*!
     * @brief 範囲の最初の列挙値を指すイテレータを取得する
     *
     * @return 範囲の最初の列挙値を指すイテレータ
     */
    constexpr auto begin() const noexcept
    {
        return range.begin();
    }

    /*!
     * @brief 範囲の最後の列挙値の次の値を指すイテレータを取得する
     *
     * @return 範囲の最後の列挙値の次の値を指すイテレータ
     */
    constexpr auto end() const noexcept
    {
        return range.end();
    }

    /*!
     * @brief 範囲に含まれる列挙値の種類数を取得する
     *
     * @return 範囲に含まれる列挙値の種類数
     */
    constexpr auto size() const noexcept
    {
        return range.size();
    }

private:
    EnumRange<EnumType> range;
};

namespace ranges {
template <typename T>
inline constexpr bool enable_view<EnumRange<T>> = true;
template <typename T>
inline constexpr bool enable_view<EnumRangeInclusive<T>> = true;
}
