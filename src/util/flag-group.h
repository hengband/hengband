#pragma once

#include "info-reader/info-reader-util.h"
#include <bitset>
#include <concepts>
#include <iterator>
#include <optional>
#include <stdint.h>
#include <type_traits>

template <typename T>
class EnumRange;

namespace flag_group {

/**
 * @brief 型がFlagGroupクラスで使用するフラグを指すイテレータであることを表すコンセプト
 *
 * Iter の型が以下の要件を満たすことを表す
 *
 * - 入力イテレータである
 * - そのイテレータが指す要素の型が FlagType である
 */
template <typename Iter, typename FlagType>
concept FlagIter = std::input_iterator<Iter> && std::same_as<std::iter_value_t<Iter>, FlagType>;

}

namespace flag_group::detail {

template <typename Func, size_t BITSET_SIZE>
void read_bitset(std::bitset<BITSET_SIZE> &bs, Func rd_byte_func, size_t count)
{
    for (auto i = 0U; i < count; i++) {
        const std::bitset<8> rd_byte_bs(rd_byte_func());

        for (auto j = 0U; j < 8; j++) {
            const size_t pos = i * 8 + j;
            if (pos >= bs.size()) {
                break;
            }

            bs[pos] = rd_byte_bs[j];
        }
    }
}

template <typename Func, size_t BITSET_SIZE>
void write_bitset(const std::bitset<BITSET_SIZE> &bs, Func wr_byte_func, size_t count)
{
    for (auto i = 0U; i < count; i++) {
        std::bitset<8> wr_byte_bs;

        for (auto j = 0U; j < 8; j++) {
            const size_t pos = i * 8 + j;
            if (pos >= bs.size()) {
                break;
            }

            wr_byte_bs[j] = bs[pos];
        }

        wr_byte_func(wr_byte_bs.to_ulong() & 0xff);
    }
}

template <typename InputIter>
constexpr unsigned long long calc_bitset_val(InputIter first, InputIter last) noexcept
{
    auto result = 0ULL;
    for (; first != last; ++first) {
        result |= 1ULL << static_cast<int>(*first);
    }
    return result;
}

}

/**
 * @brief フラグ集合を扱う、FlagGroupクラス
 *
 * @tparam FlagType 扱うフラグ集合を定義したenum型 もしくは enum class型
 * @tparam FlagTypeに列挙される値の最大値+1
 */
template <typename FlagType, FlagType MAX>
class FlagGroup {
private:
    /** フラグ集合のフラグ数 */
    static constexpr auto FLAG_TYPE_MAX = static_cast<size_t>(MAX);

public:
    using flag_type = FlagType;

    /**
     * @brief フラグ集合に含まれるフラグの種類数を返す
     *
     * @return フラグ集合に含まれるフラグの種類数
     */
    [[nodiscard]] constexpr size_t size() const noexcept
    {
        return FLAG_TYPE_MAX;
    };

    /**
     * @brief FlagGroupクラスのデフォルトコンストラクタ
     *
     * すべてのフラグがOFFの状態のFlagGroupクラスのインスタンスを生成する
     */
    constexpr FlagGroup() = default;

    /**
     * @brief FlagGroupクラスのコンストラクタ
     *
     * initializer_listで指定したフラグがON、それ以外はOFFの状態の
     * FlagGroupクラスのインスタンスを生成する
     *
     * @param il ONの状態で生成するフラグを指定した initializer_list
     */
    constexpr FlagGroup(std::initializer_list<FlagType> il)
        : FlagGroup(il.begin(), il.end())
    {
    }

    /**
     * @brief FlagGroupクラスのコンストラクタ
     *
     * EnumRangeクラスで指定した範囲のフラグがON、それ以外はOFFの状態の
     * FlagGroupクラスのインスタンスを生成する
     *
     * @param range 範囲を示すEnumRangeクラスのオブジェクト
     */
    constexpr FlagGroup(const EnumRange<FlagType> &range)
        : FlagGroup(range.begin(), range.end())
    {
    }

    /**
     * @brief FlagGroupクラスのコンストラクタ
     *
     * 入力イテレータで指定した範囲のリストに含まれるフラグがON、
     * それ以外のフラグがOFFの状態のFlagGroupクラスのインスタンスを生成する
     *
     * FLAG_TYPE_MAXがunsigned long longのビットサイズ以下の場合、
     * unsigned long longの値を引数に取るstd::bitsetのconstexpr化された
     * コンストラクタが使用できるので、FlagGroupクラスでも
     * constexprコンストラクタとしてこちらを選択する。
     *
     * @tparam InputIter 入力イテレータの型
     * @param first 範囲の開始位置を示す入力イテレータ
     * @param last 範囲の終了位置を示す入力イテレータ
     */
    template <flag_group::FlagIter<FlagType> InputIter>
        requires(FLAG_TYPE_MAX <= sizeof(unsigned long long) * 8)
    constexpr FlagGroup(InputIter first, InputIter last)
        : bs_(flag_group::detail::calc_bitset_val(first, last))
    {
    }

    /**
     * @brief FlagGroupクラスのコンストラクタ
     *
     * 入力イテレータで指定した範囲のリストに含まれるフラグがON、
     * それ以外のフラグがOFFの状態のFlagGroupクラスのインスタンスを生成する
     *
     * FLAG_TYPE_MAXがunsigned long longのビットサイズより大きい場合、
     * C++20の範囲ではstd::bitsetをconstexprコンストラクタで初期化することは
     * できないため、constexprではない通常のコンストラクタとしてこちらを選択する。
     *
     * @todo C++23以降であればstd::bitsetの多くのメンバ関数がconstepxr化されているので
     * FLAG_TYPE_MAXがunsigned long longのビットサイズより大きくてもconstexpr化が
     * 可能になると思われる。
     *
     * @tparam InputIter 入力イテレータの型
     * @param first 範囲の開始位置を示す入力イテレータ
     * @param last 範囲の終了位置を示す入力イテレータ
     */
    template <flag_group::FlagIter<FlagType> InputIter>
        requires(FLAG_TYPE_MAX > sizeof(unsigned long long) * 8)
    FlagGroup(InputIter first, InputIter last)
    {
        for (; first != last; ++first) {
            set(*first);
        }
    }

    /**
     * @brief フラグ集合に含まれるフラグをすべてOFFにする
     *
     * @return *thisの参照を返す
     */
    FlagGroup<FlagType, MAX> &clear() & noexcept
    {
        bs_.reset();
        return *this;
    }

    /**
     * @brief フラグ集合に含まれるフラグをすべてOFFにする
     *
     * @return *thisを返す
     */
    FlagGroup<FlagType, MAX> clear() && noexcept
    {
        this->clear();
        return std::move(*this);
    }

    /**
     * @brief 指定したフラグを指定した値にセットする
     *
     * @param flag 値をセットするフラグを指定する
     * @param val セットする値。trueならフラグをON、falseならフラグをOFFにする。
     *            引数を省略した場合はフラグをONにする。
     * @return *thisの参照を返す
     */
    FlagGroup<FlagType, MAX> &set(FlagType flag, bool val = true) &
    {
        bs_.set(static_cast<size_t>(flag), val);
        return *this;
    }

    /**
     * @brief 指定したフラグを指定した値にセットする
     *
     * @param flag 値をセットするフラグを指定する
     * @param val セットする値。trueならフラグをON、falseならフラグをOFFにする。
     *            引数を省略した場合はフラグをONにする。
     * @return *thisを返す
     */
    FlagGroup<FlagType, MAX> set(FlagType flag, bool val = true) &&
    {
        this->set(flag, val);
        return std::move(*this);
    }

    /**
     * @brief 入力イテレータで指定した範囲のリストに含まれるフラグをONにする
     *
     * @tparam InputIter 入力イテレータの型
     * @param first 範囲の開始位置を示す入力イテレータ
     * @param last 範囲の終了位置を示す入力イテレータ
     * @return *thisの参照を返す
     */
    template <flag_group::FlagIter<FlagType> InputIter>
    FlagGroup<FlagType, MAX> &set(InputIter first, InputIter last) &
    {
        return set(FlagGroup(first, last));
    }

    /**
     * @brief 入力イテレータで指定した範囲のリストに含まれるフラグをONにする
     *
     * @tparam InputIter 入力イテレータの型
     * @param first 範囲の開始位置を示す入力イテレータ
     * @param last 範囲の終了位置を示す入力イテレータ
     * @return *thisを返す
     */
    template <flag_group::FlagIter<FlagType> InputIter>
    FlagGroup<FlagType, MAX> set(InputIter first, InputIter last) &&
    {
        this->set(first, last);
        return std::move(*this);
    }

    /**
     * @brief 指定したFlagGroupのインスンタンスのONになっているフラグをONにする
     *
     * @param rhs ONにするフラグがONになっているFlagGroupのインスタンス
     * @return *thisの参照を返す
     */
    FlagGroup<FlagType, MAX> &set(const FlagGroup<FlagType, MAX> &rhs) &
    {
        bs_ |= rhs.bs_;
        return *this;
    }

    /**
     * @brief 指定したFlagGroupのインスンタンスのONになっているフラグをONにする
     *
     * @param rhs ONにするフラグがONになっているFlagGroupのインスタンス
     * @return *thisを返す
     */
    FlagGroup<FlagType, MAX> set(const FlagGroup<FlagType, MAX> &rhs) &&
    {
        this->set(rhs);
        return std::move(*this);
    }

    /**
     * @brief 指定したフラグをOFFにする
     *
     * @param flag OFFにするフラグを指定する
     * @return *thisの参照を返す
     */
    FlagGroup<FlagType, MAX> &reset(FlagType flag) &
    {
        bs_.reset(static_cast<size_t>(flag));
        return *this;
    }

    /**
     * @brief 指定したフラグをOFFにする
     *
     * @param flag OFFにするフラグを指定する
     * @return *thisを返す
     */
    FlagGroup<FlagType, MAX> reset(FlagType flag) &&
    {
        this->reset(flag);
        return std::move(*this);
    }

    /**
     * @brief 入力イテレータで指定した範囲のリストに含まれるフラグをOFFにする
     *
     * @tparam InputIter 入力イテレータの型
     * @param first 範囲の開始位置を示す入力イテレータ
     * @param last 範囲の終了位置を示す入力イテレータ
     * @return *thisの参照を返す
     */
    template <flag_group::FlagIter<FlagType> InputIter>
    FlagGroup<FlagType, MAX> &reset(InputIter first, InputIter last) &
    {
        return reset(FlagGroup(first, last));
    }

    /**
     * @brief 入力イテレータで指定した範囲のリストに含まれるフラグをOFFにする
     *
     * @tparam InputIter 入力イテレータの型
     * @param first 範囲の開始位置を示す入力イテレータ
     * @param last 範囲の終了位置を示す入力イテレータ
     * @return *thisを返す
     */
    template <flag_group::FlagIter<FlagType> InputIter>
    FlagGroup<FlagType, MAX> reset(InputIter first, InputIter last) &&
    {
        this->reset(first, last);
        return std::move(*this);
    }

    /**
     * @brief 指定したFlagGroupのインスンタンスのONになっているフラグをOFFにする
     *
     * @param rhs OFFにするフラグがONになっているFlagGroupのインスタンス
     * @return *thisの参照を返す
     */
    FlagGroup<FlagType, MAX> &reset(const FlagGroup<FlagType, MAX> &rhs) &
    {
        bs_ &= ~rhs.bs_;
        return *this;
    }

    /**
     * @brief 指定したFlagGroupのインスンタンスのONになっているフラグをOFFにする
     *
     * @param rhs OFFにするフラグがONになっているFlagGroupのインスタンス
     * @return *thisを返す
     */
    FlagGroup<FlagType, MAX> reset(const FlagGroup<FlagType, MAX> &rhs) &&
    {
        this->reset(rhs);
        return std::move(*this);
    }

    /**
     * @brief 指定したフラグがONかOFFか調べる
     *
     * @param f 調べるフラグを指定する
     * @return 指定したフラグがONならtrue、OFFならfalse
     */
    [[nodiscard]] bool has(FlagType f) const
    {
        if (f == MAX) {
            // どのフラグにも該当しないFlagTypeの型としてMAXを指定する事があるため、MAXが指定された時はfalseを返すようにする
            return false;
        }
        return bs_.test(static_cast<size_t>(f));
    }

    /**
     * @brief 指定したフラグがOFFかONか調べる
     *
     * @param f 調べるフラグを指定する
     * @return 指定したフラグがOFFならtrue、ONならfalse
     */
    [[nodiscard]] bool has_not(FlagType f) const
    {
        return !has(f);
    }

    /**
     * @brief フラグ集合のいずれかのフラグがONかどうかを調べる
     *
     * @return フラグ集合のいずれかのフラグがONならtrue
     *         フラグ集合のすべてのフラグがOFFならfalse
     */
    [[nodiscard]] bool any() const noexcept
    {
        return bs_.any();
    }

    /**
     * @brief フラグ集合のすべてのフラグがOFFかどうかを調べる
     *
     * @return フラグ集合のすべてのフラグがOFFならtrue
     *         フラグ集合のいずれかのフラグがONならfalse
     */
    [[nodiscard]] bool none() const noexcept
    {
        return bs_.none();
    }

    /**
     * @brief 入力イテレータで指定した範囲のリストに含まれるフラグがすべてONかどうかを調べる
     *
     * @tparam InputIter 入力イテレータの型
     * @param first 範囲の開始位置を示す入力イテレータ
     * @param last 範囲の終了位置を示す入力イテレータ
     * @return すべてのフラグがONであればtrue、そうでなければfalse
     */
    template <flag_group::FlagIter<FlagType> InputIter>
    [[nodiscard]] bool has_all_of(InputIter first, InputIter last) const
    {
        return has_all_of(FlagGroup(first, last));
    }

    /**
     * @brief 引数で指定したFlagGroupのインスンタンスのONになっているフラグがすべてONかどうかを調べる
     *
     * @param rhs FlagGroupのインスタンス
     * @return すべてのフラグがONであればtrue、そうでなければfalse
     */
    [[nodiscard]] bool has_all_of(const FlagGroup<FlagType, MAX> &rhs) const
    {
        return (bs_ & rhs.bs_) == rhs.bs_;
    }

    /**
     * @brief 入力イテレータで指定した範囲のリストに含まれるフラグのいずれかがONかどうかを調べる
     *
     * @tparam InputIter 入力イテレータの型
     * @param first 範囲の開始位置を示す入力イテレータ
     * @param last 範囲の終了位置を示す入力イテレータ
     * @return いずれかのフラグがONであればtrue、そうでなければfalse
     */
    template <flag_group::FlagIter<FlagType> InputIter>
    [[nodiscard]] bool has_any_of(InputIter first, InputIter last) const
    {
        return has_any_of(FlagGroup(first, last));
    }

    /**
     * @brief 引数で指定したFlagGroupのインスンタンスのONになっているフラグのいずれかがONかどうかを調べる
     *
     * @param rhs FlagGroupのインスタンス
     * @return いずれかのフラグがONであればtrue、そうでなければfalse
     */
    [[nodiscard]] bool has_any_of(const FlagGroup<FlagType, MAX> &rhs) const
    {
        return (bs_ & rhs.bs_).any();
    }

    /**
     * @brief 入力イテレータで指定した範囲のリストに含まれるフラグがすべてOFFかどうかを調べる
     *
     * @tparam InputIter 入力イテレータの型
     * @param first 範囲の開始位置を示す入力イテレータ
     * @param last 範囲の終了位置を示す入力イテレータ
     * @return すべてのフラグがOFFであればtrue、そうでなければfalse
     */
    template <flag_group::FlagIter<FlagType> InputIter>
    [[nodiscard]] bool has_none_of(InputIter first, InputIter last) const
    {
        return !has_any_of(first, last);
    }

    /**
     * @brief 引数で指定したFlagGroupのインスンタンスのONになっているフラグがすべてOFFかどうかを調べる
     *
     * @param rhs FlagGroupのインスタンス
     * @return すべてのフラグがOFFであればtrue、そうでなければfalse
     */
    [[nodiscard]] bool has_none_of(const FlagGroup<FlagType, MAX> &rhs) const
    {
        return !has_any_of(rhs);
    }

    /**
     * @brief フラグ集合のONになっているフラグの数を返す
     *
     * @return ONになっているフラグの数
     */
    [[nodiscard]] size_t count() const noexcept
    {
        return bs_.count();
    }

    /**
     * @brief フラグ集合のONになっているフラグのうち最初のフラグを返す
     *
     * @return フラグ集合のONになっているフラグのうち最初のフラグ。但し一つもONになっているフラグがなければ std::nullopt
     */
    [[nodiscard]] std::optional<FlagType> first() const noexcept
    {
        for (size_t i = 0; i < bs_.size(); i++) {
            if (bs_.test(i)) {
                return static_cast<FlagType>(i);
            }
        }

        return std::nullopt;
    }

    /**
     * @brief フラグ集合の状態を0と1で表した文字列を返す
     *
     * フラグ集合の上位番号から順に、フラグがONなら1、OFFなら0で表した文字列を返す。
     * 例: 5つのフラグ集合で、0:ON、1:OFF、2:OFF、3:ON、4:OFFの場合、"01001"
     *
     * @return フラグ集合の状態を表した文字列
     */
    [[nodiscard]] std::string str() const
    {
        return bs_.to_string();
    }

    /*!
     * @brief フラグ集合の状態を表したビット列を unsigned long 型にして返す
     *
     * @return フラグ集合の状態を表したビット列
     */
    [[nodiscard]] unsigned long to_ulong() const
    {
        return bs_.to_ulong();
    }

    /*!
     * @brief フラグ集合の状態を表したビット列を unsigned long long 型にして返す
     *
     * @return フラグ集合の状態を表したビット列
     */
    [[nodiscard]] unsigned long long to_ullong() const
    {
        return bs_.to_ullong();
    }

    /**
     * @brief フラグ集合 *this と rhs が等値かどうかを調べる
     *
     * @param rhs 比較するフラグ集合
     * @return すべてのフラグの状態が等しければtrue、そうでなければfalse
     */
    [[nodiscard]] bool operator==(const FlagGroup<FlagType, MAX> &rhs) const noexcept
    {
        return bs_ == rhs.bs_;
    }

    /**
     * @brief フラグ集合 *this と rhs が非等値かどうかを調べる
     *
     * @param rhs 比較するフラグ集合
     * @return いずれかのフラグの状態が等しくなければtrue、そうでなければfalse
     */
    [[nodiscard]] bool operator!=(const FlagGroup<FlagType, MAX> &rhs) const noexcept
    {
        return bs_ != rhs.bs_;
    }

    /**
     * @brief フラグ集合 *this と rhs の論理積(AND)の複合演算を行う
     *
     * *this に対して、*this と rhs で共通してONのフラグをONのままにし、それ以外のフラグをOFFにする
     *
     * @param rhs 複合演算を行うフラグ集合
     * @return *thisを返す
     */
    FlagGroup<FlagType, MAX> &operator&=(const FlagGroup<FlagType, MAX> &rhs) noexcept
    {
        bs_ &= rhs.bs_;
        return *this;
    }

    /**
     * @brief フラグ集合 *this と rhs の論理和(OR)の複合演算を行う
     *
     * *this に対して、*this と rhs でどちらか一方でもONのフラグをONにし、それ以外のフラグをOFFにする
     *
     * @param rhs 複合演算を行うフラグ集合
     * @return *thisを返す
     */
    FlagGroup<FlagType, MAX> &operator|=(const FlagGroup<FlagType, MAX> &rhs) noexcept
    {
        bs_ |= rhs.bs_;
        return *this;
    }

    /**
     * @brief フラグ集合のONになっているフラグを出力イテレータに書き込む
     *
     * @tparam OutputIter フラグを書き込む出力イテレータの型
     * @param flag_group 対象のフラグ集合
     * @param start フラグを書き込む出力イテレータ
     */
    template <typename OutputIter>
    static void get_flags(const FlagGroup<FlagType, MAX> &flag_group, OutputIter start)
    {
        for (size_t i = 0; i < flag_group.size(); i++) {
            if (flag_group.bs_.test(i)) {
                *start++ = static_cast<FlagType>(i);
            }
        }
    }

    /**
     * @brief セーブファイルからフラグ集合を読み出す
     *
     * @param fg 読み出したフラグ集合を格納するFlagGroupインスタンスの参照
     * @param rd_byte_func セーブファイルから1バイトデータを読み出す関数(rd_byte)へのポインタ
     */
    template <typename Func>
    friend void rd_FlagGroup(FlagGroup<FlagType, MAX> &fg, Func rd_byte_func)
    {
        auto tmp_l = rd_byte_func();
        auto tmp_h = rd_byte_func();
        const auto fg_size = static_cast<uint16_t>((tmp_h << 8) | tmp_l);

        flag_group::detail::read_bitset(fg.bs_, rd_byte_func, fg_size);
    }

    /**
     * @brief セーブファイルにフラグ集合を書き込む
     *
     * @param fg 書き込むフラグ集合を保持したFlagGroupインスタンスの参照
     * @param wr_byte_func セーブファイルに1バイトデータを書き込む関数(wr_byte)へのポインタ
     */
    template <typename Func>
    friend void wr_FlagGroup(const FlagGroup<FlagType, MAX> &fg, Func wr_byte_func)
    {
        const auto fg_size = static_cast<uint16_t>((fg.bs_.size() + 7) / 8);
        wr_byte_func(fg_size & 0xff);
        wr_byte_func((fg_size >> 8) & 0xff);

        flag_group::detail::write_bitset(fg.bs_, wr_byte_func, fg_size);
    }

    /**
     * @brief セーブファイルから固定バイト長でフラグ集合を読み出す
     *
     * @param fg 読み出したフラグ集合を格納するFlagGroupインスタンスの参照
     * @param rd_byte_func セーブファイルから1バイトデータを読み出す関数(rd_byte)へのポインタ
     * @param count 読み出すバイト数
     */
    template <typename Func>
    friend void rd_FlagGroup_bytes(FlagGroup<FlagType, MAX> &fg, Func rd_byte_func, size_t count)
    {
        flag_group::detail::read_bitset(fg.bs_, rd_byte_func, count);
    }

    /**
     * @brief セーブファイルに固定バイト長でフラグ集合を書き込む
     *
     * @param fg 書き込むフラグ集合を保持したFlagGroupインスタンスの参照
     * @param wr_byte_func セーブファイルに1バイトデータを書き込む関数(wr_byte)へのポインタ
     * @param count 書き込むバイト数
     */
    template <typename Func>
    friend void wr_FlagGroup_bytes(const FlagGroup<FlagType, MAX> &fg, Func wr_byte_func, size_t count)
    {
        flag_group::detail::write_bitset(fg.bs_, wr_byte_func, count);
    }

    /**
     * @brief 文字列からフラグへのマップを指定した検索キーで検索し、
     *        見つかった場合はフラグ集合に該当するフラグをセットする
     *
     * @param fg フラグをセットするフラグ集合
     * @param dict 文字列からフラグへのマップ
     * @param what マップの検索キー
     * @return 検索キーでフラグが見つかり、フラグをセットした場合 true
     *         見つからなかった場合 false
     */
    template <typename Key, DictIndexedBy<Key> Dict>
    static bool grab_one_flag(FlagGroup<FlagType, MAX> &fg, const Dict &dict, Key &&what)
    {
        auto val = info_get_const(dict, std::forward<Key>(what));
        if (val) {
            fg.set(*val);
            return true;
        }
        return false;
    }

private:
    /**
     * @brief 指定したフラグ位置へのアクセスを提供するプロキシクラス
     */
    class reference {
    public:
        reference(std::bitset<FLAG_TYPE_MAX> &flags, size_t pos)
            : bs_(flags)
            , pos_(pos)
        {
        }

        reference &operator=(const reference &rhs)
        {
            bs_[pos_] = static_cast<bool>(rhs);
            return *this;
        }

        reference &operator=(bool val)
        {
            bs_[pos_] = val;
            return *this;
        }

        [[nodiscard]] operator bool() const
        {
            return bs_[pos_];
        }

    private:
        std::bitset<FLAG_TYPE_MAX> &bs_;
        size_t pos_;
    };

public:
    /**
     * @brief 指定したフラグへのアクセスを提供するプロキシオブジェクトを取得する
     *
     * @param f プロキシオブジェクトを取得するフラグ
     * @return 指定したフラグへのアクセスを提供するプロキシオブジェクト
     */
    [[nodiscard]] reference operator[](FlagType f)
    {
        return reference(bs_, static_cast<size_t>(f));
    }

private:
    /** フラグ集合を保持するstd::bitsetのインスタンス */
    std::bitset<FLAG_TYPE_MAX> bs_;
};

/**
 * @brief enum clsas 型に対してFlagGroupクラスを使用するエイリアステンプレート
 *
 * FlagGroupクラスのテンプレート引数に、使用する型として FlagType、列挙値の最大値として FlagType::MAX を渡す。
 *
 * @tparam FlagType FlagGroupクラスを使用する enum class 型
 */
template <typename FlagType>
using EnumClassFlagGroup = FlagGroup<FlagType, FlagType::MAX>;

/**
 * @brief フラグ集合 lhs と rhs に対して論理積(AND)を取ったフラグ集合を生成する
 *
 * lhs と rhs で共通してONのフラグがON、それ以外のフラグがOFFのフラグ集合を生成する
 *
 * @tparam FlagType 扱うフラグ集合を定義したenum class型
 * @param lhs フラグ集合1
 * @param rhs フラグ集合2
 * @return lhs と rhs の論理積を取ったフラグ集合
 */
template <typename FlagType, FlagType MAX>
[[nodiscard]] FlagGroup<FlagType, MAX> operator&(const FlagGroup<FlagType, MAX> &lhs, const FlagGroup<FlagType, MAX> &rhs) noexcept
{
    return FlagGroup<FlagType, MAX>(lhs) &= rhs;
}

/**
 * @brief フラグ集合 lhs と rhs に対して論理和(OR)を取ったフラグ集合を生成する
 *
 * lhs と rhs でどちらか一方でもONのフラグがON、それ以外のフラグがOFFのフラグ集合を生成する
 *
 * @tparam FlagType 扱うフラグ集合を定義したenum class型
 * @param lhs フラグ集合1
 * @param rhs フラグ集合2
 * @return lhs と rhs の論理積を取ったフラグ集合
 */
template <typename FlagType, FlagType MAX>
[[nodiscard]] FlagGroup<FlagType, MAX> operator|(const FlagGroup<FlagType, MAX> &lhs, const FlagGroup<FlagType, MAX> &rhs) noexcept
{
    return FlagGroup<FlagType, MAX>(lhs) |= rhs;
}
