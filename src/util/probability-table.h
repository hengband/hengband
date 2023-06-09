#pragma once

#include "system/angband-exceptions.h"
#include "term/z-rand.h"
#include <algorithm>
#include <exception>
#include <stdexcept>
#include <tuple>
#include <vector>

/**
 * @brief 確率テーブルクラス
 *
 * 確率テーブルを作成し、確率に従った抽選を行うクラス
 *
 * @tparam IdType 確率テーブルに登録するIDの型
 */
template <typename IdType>
class ProbabilityTable {
public:
    /**
     * @brief コンストラクタ
     *
     * 空の確率テーブルを生成する
     */
    ProbabilityTable() = default;

    /**
     * @brief 確率テーブルを空にする
     */
    void clear()
    {
        return item_list_.clear();
    }

    /**
     * @brief 確率テーブルに項目を登録する
     *
     * 確率テーブルに項目のIDと選択確率のセットを登録する。
     * 追加した項目が選択される確率は、
     * 追加した項目の確率(引数prob) / すべての項目のprobの合計
     * となる。
     * probが0もしくは負数の場合はなにも登録しない。
     *
     * @param id 項目のID
     * @param prob 項目の選択確率
     */
    void entry_item(IdType id, int prob)
    {
        if (prob > 0) {
            // 二分探索を行うため、probは累積値を格納する
            auto cumulative_prob = item_list_.empty() ? 0 : std::get<1>(item_list_.back());
            item_list_.emplace_back(id, cumulative_prob + prob);
        }
    }

    /**
     * @brief 現在の確率テーブルのすべての項目の選択確率の合計を取得する
     *
     * 確率テーブルに登録されているすべての項目の確率(登録時の引数prob)の合計を取得する。
     *
     * @return int 現在の確率テーブルのすべての項目の選択確率の合計
     */
    int total_prob() const
    {
        if (item_list_.empty()) {
            return 0;
        }

        return std::get<1>(item_list_.back());
    }

    /**
     * @brief 確率テーブルに登録されている項目の数を取得する
     *
     * 確率テーブルに登録されている項目の数を取得する。
     * 登録されている項目のIDに被りがある場合は別の項目として加算した数となる。
     *
     * @return size_t 確率テーブルに登録されている項目の数
     */
    size_t item_count() const
    {
        return item_list_.size();
    }

    /**
     * @brief 確率テーブルの項目が空かどうかを調べる
     *
     * @return bool 確率テーブルに項目が一つも登録されておらず空であれば true
     *              項目が一つ以上登録されており空でなければ false
     */
    bool empty() const
    {
        return item_list_.empty();
    }

    /**
     * @brief 確率テーブルから項目をランダムに1つ選択する
     *
     * 確率テーブルに登録されているすべての項目から、確率に従って項目を1つ選択する
     * それぞれの項目が選択される確率は、確率設定probに対して
     * 該当項目のprob / すべての項目のprobの合計
     * となる。
     * 抽選は独立試行で行われ、選択された項目がテーブルから取り除かれる事はない。
     * 確率テーブルになにも登録されていない場合、std::runtime_error例外を送出する。
     *
     * @return int 選択された項目のID
     */
    IdType pick_one_at_random() const
    {
        if (empty()) {
            THROW_EXCEPTION(std::runtime_error, "There is no entry in the probability table.");
        }

        // probの合計の範囲からランダムでkeyを取得し、二分探索で選択する項目を決定する
        const int key = randint0(total_prob());
        auto it = std::partition_point(item_list_.begin(), item_list_.end(), [key](const auto &i) { return std::get<1>(i) <= key; });

        return std::get<0>(*it);
    }

    /**
     * @brief 確率テーブルから複数回抽選する
     *
     * 確率テーブルから引数 n で指定した回数抽選し、抽選の結果選択された項目のIDを
     * 出力イテレータに n 個書き込む。
     * 抽選は独立試行で行われ、選択された項目がテーブルから取り除かれる事はない。
     *
     * @tparam OutputIter 出力イテレータの型
     * @param first 結果を書き込む出力イテレータ
     * @param table 抽選を行う確率テーブル
     * @param n 抽選を行う回数
     */
    template <typename OutputIter>
    static void lottery(OutputIter first, const ProbabilityTable &table, size_t n)
    {
        std::generate_n(first, n, [&table] { return table.pick_one_at_random(); });
    }

private:
    /** 項目のIDと確率のセットを格納する配列 */
    std::vector<std::tuple<IdType, int>> item_list_;
};
