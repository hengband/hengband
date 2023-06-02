#pragma once

#include <iterator>
#include <type_traits>

/*!
 * @brief enum もしくは enum class の列挙値の範囲を扱うクラス
 *
 * @tparam EnumType 対象となる列挙型(enum もしくは enum class)
 */
template <typename EnumType>
class EnumRange {
    static_assert(std::is_enum_v<EnumType>);

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
        constexpr bool operator==(const iterator &other) const noexcept
        {
            return index == other.index;
        }

    private:
        //! 現在イテレータが指している列挙値の基底型における整数値
        std::underlying_type_t<EnumType> index;
    };

    /*!
     * @brief 引数で与えた範囲の EnumRange クラスのオブジェクトを生成する
     * @details 生成する範囲はfirstからlastまで(lastを含む)。
     * 範囲の最終値+1の列挙値が存在しない場合を考慮し、半開区間ではなく閉区間で範囲を指定する。
     * 範囲のイテレーションは基底型の整数値をインクリメントする事によって行うので、
     * first から last までの間の整数値が飛んでいる場合、その部分は具体的な定義の無い列挙値がイテレートされる。
     * @param first 範囲の最初となる列挙値
     * @param last 範囲の最後となる列挙値(lastも含む)
     */
    constexpr EnumRange(EnumType first, EnumType last) noexcept
        : begin_val(first)
        , end_val(static_cast<EnumType>(std::underlying_type_t<EnumType>(last) + 1))
    {
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
