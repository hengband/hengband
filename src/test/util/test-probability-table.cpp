/*!
 * @brief ProbabilityTableクラスのテスト
 *
 * 確率テーブルからの抽選が、登録した確率どおりの分布になっているかを検証する。
 * 抽選結果は乱数に依存するため、テストが常に同じ結果になるよう乱数シードを固定している。
 */

#include "util/probability-table.h"

#include "test/scoped-rng.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <iterator>
#include <map>
#include <numeric>
#include <random>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace {

//! 確率テーブルに登録する (ID, prob) の並び
using TestList = std::vector<std::tuple<int, int>>;

/*!
 * @brief 確率テーブルから指定回数抽選し、IDごとの出現率が計算上の確率と一致するか調べる
 * @param table 抽選を行う確率テーブル
 * @param test_list tableに登録されている (ID, prob) の並び
 * @param lottery_count 抽選回数
 */
void simulate(const ProbabilityTable<int> &table, const TestList &test_list, int lottery_count)
{
    std::vector<int> result;
    ProbabilityTable<int>::lottery(std::back_inserter(result), table, lottery_count);

    // IDの抽選結果の分布
    std::map<int, int> lottery_result;
    for (auto id : result) {
        lottery_result[id]++;
    }

    // IDごとの登録probの合計 (IDかぶりの項目はここで合算される)
    std::map<int, int> entried_prob;
    for (const auto &[id, prob] : test_list) {
        entried_prob[id] += prob;
    }

    const auto total_prob = table.total_prob();

    // IDそれぞれに対し、選択された確率が計算上の確率にあっているか調べる
    for (const auto &[id, count] : lottery_result) {
        // 失敗時にどのIDで落ちたのかが分かるようにする。
        // CAPTURE はラムダで対象を包むが、構造化束縛の変数をラムダにキャプチャできない
        // 処理系 (clang 15以前) があるため、通常の変数に移してから渡す
        const auto lottery_id = id;
        CAPTURE(lottery_id);

        // 計算上の確率 → そのIDが選ばれるprob / 全体のprob
        const auto calc_rate = static_cast<double>(entried_prob.at(id)) / total_prob;

        // IDが実際に抽選された確率
        const auto item_rate = static_cast<double>(count) / lottery_count;

        // 抽選結果は二項分布に従うため、実際に抽選された確率のばらつきの大きさは
        // その標準偏差 sqrt(p(1-p)/n) となる。許容値をこれに基づく値にすることで、
        // 選ばれる確率が小さい項目でも重み付けの誤りを検出できる。
        // (固定値だと、項目数が多く1項目あたりの確率が許容値を下回るテーブルでは
        //  重みを無視した抽選でも判定が素通りしてしまう)
        // 係数6は正常であればまず超えない水準、末尾の項は抽選回数が整数値しか
        // 取らないことによる端数の補正。
        const auto allowable_error = 6.0 * std::sqrt(calc_rate * (1.0 - calc_rate) / lottery_count) + 0.5 / lottery_count;

        CHECK(std::abs(item_rate - calc_rate) <= allowable_error);
    }
}

/*!
 * @brief 確率テーブルの生成・追加登録・クリアを行いながら抽選結果を検証する
 * @param test_list 確率テーブルに登録する (ID, prob) の並び
 * @param lottery_count 1回の検証あたりの抽選回数
 */
void test_table(TestList test_list, int lottery_count)
{
    ProbabilityTable<int> table;
    REQUIRE(table.empty());
    for (const auto &[id, prob] : test_list) {
        table.entry_item(id, prob);
    }

    REQUIRE(table.item_count() == test_list.size());
    const auto total_prob = std::accumulate(
        test_list.begin(), test_list.end(), 0, [](int sum, const auto &item) { return sum + std::get<1>(item); });
    REQUIRE(table.total_prob() == total_prob);

    // test_list をテスト
    simulate(table, test_list, lottery_count);

    // さらに test_list の前半分を追加したものをテスト
    const TestList half_test_list(test_list.begin(), test_list.begin() + test_list.size() / 2);
    for (const auto &[id, prob] : half_test_list) {
        table.entry_item(id, prob);
    }
    test_list.insert(test_list.end(), half_test_list.begin(), half_test_list.end());
    simulate(table, test_list, lottery_count);

    // 一旦クリアして test_list の前半分のみでテスト
    table.clear();
    for (const auto &[id, prob] : half_test_list) {
        table.entry_item(id, prob);
    }
    if (!half_test_list.empty()) {
        simulate(table, half_test_list, lottery_count);
    }
}

}

TEST_CASE("ProbabilityTable lottery matches the entried probabilities")
{
    // 乱数生成器を固定シードにして抽選結果を決定的にする
    const auto restore_rng = test::scoped_rng();

    SUBCASE("single item")
    {
        test_table({ { 1, 1 } }, 50000);
        test_table({ { 1, 100 } }, 50000);
    }

    SUBCASE("three items")
    {
        test_table({ { 1, 1 }, { 2, 2 }, { 3, 3 } }, 500000);
        test_table({ { 1, 3 }, { 2, 2 }, { 3, 1 } }, 500000); // 逆パターン
        test_table({ { 1, 13 }, { 2, 37 }, { 3, 23 } }, 500000); // 乱雑なprob
    }

    SUBCASE("1000 items")
    {
        TestList test_list;
        for (auto i = 0; i < 1000; i++) {
            test_list.emplace_back(i + 1, i + 1);
        }
        test_table(test_list, 50000);

        // 逆順
        std::reverse(test_list.begin(), test_list.end());
        test_table(test_list, 50000);
    }

    SUBCASE("1000 items which have random probs")
    {
        std::mt19937 mt(test::DEFAULT_RNG_SEED);
        std::uniform_int_distribution<> dist(1, 100);

        TestList test_list;
        for (auto i = 0; i < 1000; i++) {
            test_list.emplace_back(i + 1, dist(mt));
        }
        test_table(test_list, 50000);

        // IDかぶり項目がある場合
        test_list.emplace_back(320, dist(mt));
        test_list.emplace_back(673, dist(mt));
        test_table(test_list, 50000);
    }
}

TEST_CASE("ProbabilityTable ignores an item which has non-positive prob")
{
    ProbabilityTable<int> table;
    table.entry_item(1, 0);
    table.entry_item(2, -1);

    CHECK(table.empty());
    CHECK(table.item_count() == 0);
    CHECK(table.total_prob() == 0);
}

TEST_CASE("ProbabilityTable throws an exception when it is empty")
{
    const ProbabilityTable<int> table;

    CHECK_THROWS_AS(table.pick_one_at_random(), std::runtime_error);
}
