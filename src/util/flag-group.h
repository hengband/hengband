#pragma once

#include <algorithm>
#include <bitset>
#include <functional>
#include <map>

/**
 * @brief フラグ集合を扱う、FlagGroupクラス
 *
 * @tparam FlagType 扱うフラグ集合を定義したenum class型
 */
template <typename FlagType>
class FlagGroup {
public:
    /**
     * @brief フラグ集合に含まれるフラグの種類数を返す
     *
     * @return constexpr size_t フラグ集合に含まれるフラグの種類数
     */
    constexpr size_t size() const noexcept
    {
        return FLAG_TYPE_MAX;
    };

    /**
     * @brief FlagGroupクラスのデフォルトコンストラクタ
     *
     * すべてのフラグがOFFの状態のFlagGroupクラスのインスタンスを生成する
     */
    FlagGroup<FlagType>() = default;

    /**
     * @brief FlagGroupクラスのコンストラクタ
     *
     * initializer_listで指定したフラグがON、それ以外はOFFの状態の
     * FlagGroupクラスのインスタンスを生成する
     *
     * @param il ONの状態で生成するフラグを指定した initializer_list
     */
    FlagGroup<FlagType>(std::initializer_list<FlagType> il)
    {
        set(il);
    }

    /**
     * @brief FlagGroupクラスのコンストラクタ
     *
     * 入力イテレータで指定した範囲のリストに含まれるフラグがON、
     * それ以外のフラグがOFFの状態のFlagGroupクラスのインスタンスを生成する
     *
     * @tparam InputIter 入力イテレータの型
     * @param first 範囲の開始位置を示す入力イテレータ
     * @param last 範囲の終了位置を示す入力イテレータ
     */
    template <typename InputIter>
    FlagGroup<FlagType>(InputIter first, InputIter last)
    {
        set(first, last);
    }

    /**
     * @brief フラグ集合に含まれるフラグをすべてOFFにする
     *
     * @return FlagGroup<FlagType>& *thisを返す
     */
    FlagGroup<FlagType> &clear() noexcept
    {
        bs_.reset();
        return *this;
    }

    /**
     * @brief 指定したフラグを指定した値にセットする
     *
     * @param flag 値をセットするフラグを指定する
     * @param val セットする値。trueならフラグをON、falseならフラグをOFFにする。
     *            引数を省略した場合はフラグをONにする。
     * @return FlagGroup<FlagType>& *thisを返す
     */
    FlagGroup<FlagType> &set(FlagType flag, bool val = true)
    {
        bs_.set(static_cast<size_t>(flag), val);
        return *this;
    }

    /**
     * @brief 入力イテレータで指定した範囲のリストに含まれるフラグをONにする
     *
     * @tparam InputIter 入力イテレータの型
     * @param first 範囲の開始位置を示す入力イテレータ
     * @param last 範囲の終了位置を示す入力イテレータ
     * @return FlagGroup<FlagType>& *thisを返す
     */
    template <typename InputIter>
    FlagGroup<FlagType> &set(InputIter first, InputIter last)
    {
        static_assert(std::is_same<typename std::iterator_traits<InputIter>::value_type, FlagType>::value, "Iterator value type is invalid");

        std::for_each(first, last, [this](FlagType t) { set(t); });
        return *this;
    }

    /**
     * @brief 指定したFlagGroupのインスンタンスのONになっているフラグをONにする
     *
     * @param rhs ONにするフラグがONになっているFlagGroupのインスタンス
     * @return FlagGroup<FlagType>& *thisを返す
     */
    FlagGroup<FlagType> &set(const FlagGroup<FlagType> &rhs)
    {
        bs_ |= rhs.bs_;
        return *this;
    }

    /**
     * @brief 指定したinitializer_listに含まれるフラグをONにする
     *
     * @param list ONにするフラグを列挙したinitializer_list
     * @return FlagGroup<FlagType>& *thisを返す
     */
    FlagGroup<FlagType> &set(std::initializer_list<FlagType> list)
    {
        return set(std::begin(list), std::end(list));
    }

    /**
     * @brief 指定したフラグをOFFにする
     *
     * @param flag OFFにするフラグを指定する
     * @return FlagGroup<FlagType>& *thisを返す
     */
    FlagGroup<FlagType> &reset(FlagType flag)
    {
        bs_.reset(static_cast<size_t>(flag));
        return *this;
    }

    /**
     * @brief 入力イテレータで指定した範囲のリストに含まれるフラグをOFFにする
     *
     * @tparam InputIter 入力イテレータの型
     * @param first 範囲の開始位置を示す入力イテレータ
     * @param last 範囲の終了位置を示す入力イテレータ
     * @return FlagGroup<FlagType>& *thisを返す
     */
    template <typename InputIter>
    FlagGroup<FlagType> &reset(InputIter first, InputIter last)
    {
        static_assert(std::is_same<typename std::iterator_traits<InputIter>::value_type, FlagType>::value, "Iterator value type is invalid");

        std::for_each(first, last, [this](FlagType t) { reset(t); });
        return *this;
    }

    /**
     * @brief 指定したFlagGroupのインスンタンスのONになっているフラグをOFFにする
     *
     * @param rhs OFFにするフラグがONになっているFlagGroupのインスタンス
     * @return FlagGroup<FlagType>& *thisを返す
     */
    FlagGroup<FlagType> &reset(const FlagGroup<FlagType> &rhs)
    {
        bs_ &= ~rhs.bs_;
        return *this;
    }

    /**
     * @brief 指定したinitializer_listに含まれるフラグをOFFにする
     *
     * @param list OFFにするフラグを列挙したinitializer_list
     * @return FlagGroup<FlagType>& *thisを返す
     */
    FlagGroup<FlagType> &reset(std::initializer_list<FlagType> list)
    {
        return reset(std::begin(list), std::end(list));
    }

    /**
     * @brief 指定したフラグがONかOFFか調べる
     *
     * @param f 調べるフラグを指定する
     * @return bool 指定したフラグがONならtrue、OFFならfalse
     */
    bool has(FlagType f) const
    {
        return bs_.test(static_cast<size_t>(f));
    }

    /**
     * @brief 指定したフラグがOFFかONか調べる
     *
     * @param f 調べるフラグを指定する
     * @return bool 指定したフラグがOFFならtrue、ONならfalse
     */
    bool has_not(FlagType f) const
    {
        return !has(f);
    }

    /**
     * @brief フラグ集合のいずれかのフラグがONかどうかを調べる
     *
     * @return bool フラグ集合のいずれかのフラグがONならtrue
     *              フラグ集合のすべてのフラグがOFFならfalse
     */
    bool any() const noexcept
    {
        return bs_.any();
    }

    /**
     * @brief フラグ集合のすべてのフラグがOFFかどうかを調べる
     *
     * @return bool フラグ集合のすべてのフラグがOFFならtrue
     *              フラグ集合のいずれかのフラグがONならfalse
     */
    bool none() const noexcept
    {
        return bs_.none();
    }

    /**
     * @brief 入力イテレータで指定した範囲のリストに含まれるフラグがすべてONかどうかを調べる
     *
     * @tparam InputIter 入力イテレータの型
     * @param first 範囲の開始位置を示す入力イテレータ
     * @param last 範囲の終了位置を示す入力イテレータ
     * @return bool すべてのフラグがONであればtrue、そうでなければfalse
     */
    template <typename InputIter>
    bool has_all_of(InputIter first, InputIter last) const
    {
        static_assert(std::is_same<typename std::iterator_traits<InputIter>::value_type, FlagType>::value, "Iterator value type is invalid");

        return std::all_of(first, last, [this](FlagType f) { return has(f); });
    }

    /**
     * @brief 指定したinitializer_listに含まれるフラグがすべてONかどうかを調べる
     *
     * @param list 調べるフラグを列挙したinitializer_list
     * @return bool すべてのフラグがONであればtrue、そうでなければfalse
     */
    bool has_all_of(std::initializer_list<FlagType> list) const
    {
        return has_all_of(std::begin(list), std::end(list));
    }

    /**
     * @brief 引数で指定したFlagGroupのインスンタンスのONになっているフラグがすべてONかどうかを調べる
     *
     * @param rhs FlagGroupのインスタンス
     * @return bool すべてのフラグがONであればtrue、そうでなければfalse
     */
    bool has_all_of(const FlagGroup<FlagType> &rhs) const
    {
        return (bs_ & rhs.bs_) == rhs.bs_;
    }

    /**
     * @brief 入力イテレータで指定した範囲のリストに含まれるフラグのいずれかがONかどうかを調べる
     *
     * @tparam InputIter 入力イテレータの型
     * @param first 範囲の開始位置を示す入力イテレータ
     * @param last 範囲の終了位置を示す入力イテレータ
     * @return bool いずれかのフラグがONであればtrue、そうでなければfalse
     */
    template <typename InputIter>
    bool has_any_of(InputIter first, InputIter last) const
    {
        static_assert(std::is_same<typename std::iterator_traits<InputIter>::value_type, FlagType>::value, "Iterator value type is invalid");

        return std::any_of(first, last, [this](FlagType f) { return has(f); });
    }

    /**
     * @brief 指定したinitializer_listに含まれるフラグのいずれかがONかどうかを調べる
     *
     * @param list 調べるフラグを列挙したinitializer_list
     * @return bool いずれかのフラグがONであればtrue、そうでなければfalse
     */
    bool has_any_of(std::initializer_list<FlagType> list) const
    {
        return has_any_of(std::begin(list), std::end(list));
    }

    /**
     * @brief 引数で指定したFlagGroupのインスンタンスのONになっているフラグのいずれかがONかどうかを調べる
     *
     * @param rhs FlagGroupのインスタンス
     * @return bool いずれかのフラグがONであればtrue、そうでなければfalse
     */
    bool has_any_of(const FlagGroup<FlagType> &rhs) const
    {
        return (bs_ & rhs.bs_) != 0;
    }

    /**
     * @brief 入力イテレータで指定した範囲のリストに含まれるフラグがすべてOFFかどうかを調べる
     *
     * @tparam InputIter 入力イテレータの型
     * @param first 範囲の開始位置を示す入力イテレータ
     * @param last 範囲の終了位置を示す入力イテレータ
     * @return bool すべてのフラグがOFFであればtrue、そうでなければfalse
     */
    template <typename InputIter>
    bool has_none_of(InputIter first, InputIter last) const
    {
        static_assert(std::is_same<typename std::iterator_traits<InputIter>::value_type, FlagType>::value, "Iterator value type is invalid");

        return !has_any_of(first, last);
    }

    /**
     * @brief 指定したinitializer_listに含まれるフラグがすべてOFFかどうかを調べる
     *
     * @param list 調べるフラグを列挙したinitializer_list
     * @return bool すべてのフラグがOFFであればtrue、そうでなければfalse
     */
    bool has_none_of(std::initializer_list<FlagType> list) const
    {
        return !has_any_of(list);
    }

    /**
     * @brief 引数で指定したFlagGroupのインスンタンスのONになっているフラグがすべてOFFかどうかを調べる
     *
     * @param rhs FlagGroupのインスタンス
     * @return bool すべてのフラグがOFFであればtrue、そうでなければfalse
     */
    bool has_none_of(const FlagGroup<FlagType> &rhs) const
    {
        return !has_any_of(rhs);
    }

    /**
     * @brief フラグ集合のONになっているフラグの数を返す
     *
     * @return size_t ONになっているフラグの数
     */
    size_t count() const noexcept
    {
        return bs_.count();
    }

    /**
     * @brief フラグ集合の状態を0と1で表した文字列を返す
     *
     * フラグ集合の上位番号から順に、フラグがONなら1、OFFなら0で表した文字列を返す。
     * 例: 5つのフラグ集合で、0:ON、1:OFF、2:OFF、3:ON、4:OFFの場合、"01001"
     *
     * @return std::string フラグ集合の状態を表した文字列
     */
    std::string str() const
    {
        return bs_.to_string();
    }

    /**
     * @brief セーブファイルからフラグ集合を読み出す
     *
     * @param fg 読み出したフラグ集合を格納するFlagGroupインスタンスの参照
     * @param rd_byte_func セーブファイルから1バイトデータを読み出す関数(rd_byte)へのポインタ
     */
    friend void rd_FlagGroup(FlagGroup<FlagType> &fg, std::function<void(uint8_t *)> rd_byte_func)
    {
        uint8_t tmp_l, tmp_h;
        rd_byte_func(&tmp_l);
        rd_byte_func(&tmp_h);
        const auto fg_size = static_cast<uint16_t>((tmp_h << 8) | tmp_l);

        for (int i = 0; i < fg_size; i++) {
            uint8_t flag_byte;
            rd_byte_func(&flag_byte);
            std::bitset<8> flag_bits(flag_byte);
            for (int j = 0; j < 8; j++) {
                const size_t pos = i * 8 + j;
                if (pos < fg.bs_.size()) {
                    fg.bs_[pos] = flag_bits[j];
                }
            }
        }
    }

    /**
     * @brief セーブファイルにフラグ集合を書き込む
     *
     * @param fg 書き込むフラグ集合を保持したFlagGroupインスタンスの参照
     * @param wr_byte_func セーブファイルに1バイトデータを書き込む関数(wr_byte)へのポインタ
     */
    friend void wr_FlagGroup(const FlagGroup<FlagType> &fg, std::function<void(uint8_t)> wr_byte_func)
    {
        const auto fg_size = static_cast<uint16_t>((fg.bs_.size() + 7) / 8);
        wr_byte_func(fg_size & 0xff);
        wr_byte_func((fg_size >> 8) & 0xff);

        for (int i = 0; i < fg_size; i++) {
            std::bitset<8> flag_bits;
            for (int j = 0; j < 8; j++) {
                const size_t pos = i * 8 + j;
                if (pos < fg.bs_.size()) {
                    flag_bits[j] = fg.bs_[pos];
                }
            }
            wr_byte_func(flag_bits.to_ulong() & 0xff);
        }
    }

    /**
     * @brief 文字列からフラグへのマップを指定した検索キーで検索し、
     *        見つかった場合はフラグ集合に該当するフラグをセットする
     *
     * @tparam Map std::map<string_view, FlagType> もしくは std::unordered_map<string_view, FlagType>
     * @param fg フラグをセットするフラグ集合
     * @param dict 文字列からフラグへのマップ
     * @param what マップの検索キー
     * @return bool 検索キーでフラグが見つかり、フラグをセットした場合 true
     *              見つからなかった場合 false
     */
    template <typename Map>
    static bool grab_one_flag(FlagGroup<FlagType>& fg, const Map &dict, std::string_view what)
    {
        auto it = dict.find(what);
        if (it == dict.end())
            return false;

        fg.set(it->second);
        return true;
    }

private:
    /**
     * @brief 指定したフラグ位置へのアクセスを提供するプロキシクラス
     */
    class reference {
    public:
        reference(std::bitset<FlagGroup::FLAG_TYPE_MAX> &flags, size_t pos)
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

        operator bool() const
        {
            return bs_[pos_];
        }

    private:
        std::bitset<FlagGroup::FLAG_TYPE_MAX> &bs_;
        size_t pos_;
    };

public:
    /**
     * @brief 指定したフラグへのアクセスを提供するプロキシオブジェクトを取得する
     *
     * @param f プロキシオブジェクトを取得するフラグ
     * @return reference 指定したフラグへのアクセスを提供するプロキシオブジェクト
     */
    reference operator[](FlagType f)
    {
        return reference(bs_, static_cast<size_t>(f));
    }

private:
    /** フラグ集合のフラグ数 */
    static constexpr auto FLAG_TYPE_MAX = static_cast<size_t>(FlagType::MAX);

    /** フラグ集合を保持するstd::bitsetのインスタンス */
    std::bitset<FLAG_TYPE_MAX> bs_;
};
