/*!
 * @brief ProbabilityTableクラスのテストプログラム
 *
 * srcディレクトリで以下のコマンドでコンパイルして実行する
 *
 * g++ -std=c++20 -I. test/test-probability-table.cpp util/rng-xoshiro.cpp term/z-rand.cpp system/angband-system.cpp main-unix/stack-trace-unix.cpp system/angband-version.cpp term/z-form.cpp term/z-util.cpp
 *
 * 実行すると永久にテストを繰り返し、想定通りの確率で抽選されていなければassertでプログラムが停止する
 */

#include <cassert>
#include <iostream>
#include <map>
#include <numeric>
#include <random>

#include "system/angband-system.h"
#include "util/probability-table.h"

static void simulate(const ProbabilityTable<int> &table, const std::vector<std::tuple<int, int>> &test_list, int lottery_count)
{
    std::vector<int> result;
    ProbabilityTable<int>::lottery(std::back_inserter(result), table, lottery_count);

    // IDの抽選結果の分布
    std::map<int, int> m;
    for (auto i : result) {
        m[i]++;
    }

    printf("total_prob = %u\n", table.total_prob());
    // IDそれぞれに対し、選択された確率が計算上の確率にあっているか調べる
    for (const auto &[k, v] : m) {
        // 計算上の確率 → そのIDが選ばれるprob / 全体のprob
        auto item_prob = 0;
        for (auto [id, prob] : test_list) {
            if (id == k) {
                item_prob += prob;
            }
        }
        auto calc_rate = static_cast<double>(item_prob) / table.total_prob();

        // IDが実際に抽選された確率
        auto item_rate = static_cast<double>(v) / lottery_count;

        // 計算上の確率との誤差が0.3%未満ならOKとする
        assert(std::abs(item_rate - calc_rate) < 0.003);
    }
}

static void test(std::vector<std::tuple<int, int>> test_list, int lottery_count)
{
    ProbabilityTable<int> table;
    assert(table.empty());
    for (auto &&i : test_list) {
        table.entry_item(std::get<0>(i), std::get<1>(i));
    }

    assert(table.item_count() == test_list.size());
    auto sum = std::accumulate(
        test_list.begin(), test_list.end(), std::make_tuple(0, 0), [](const auto &a, const auto &b) { return std::make_tuple(0, std::get<1>(a) + std::get<1>(b)); });
    assert(table.total_prob() == std::get<1>(sum));

    // test_list をテスト
    simulate(table, test_list, lottery_count);

    // さらに test_list の前半分を追加したものをテスト
    std::vector<std::tuple<int, int>> half_test_list(test_list.begin(), test_list.begin() + test_list.size() / 2);
    for (auto &&i : half_test_list) {
        table.entry_item(std::get<0>(i), std::get<1>(i));
    }
    test_list.insert(test_list.end(), half_test_list.begin(), half_test_list.end());
    simulate(table, test_list, lottery_count);

    // 一旦クリアして test_list の前半分のみでテスト
    table.clear();
    for (auto &&i : half_test_list) {
        table.entry_item(std::get<0>(i), std::get<1>(i));
    }
    if (!half_test_list.empty()) {
        simulate(table, half_test_list, lottery_count);
    }
}

static int test_main()
{
    std::random_device rd;
    std::mt19937 mt(rd());
    Xoshiro128StarStar xoshiro(rd());

    AngbandSystem::get_instance().set_rng(xoshiro);

    // 選択肢1個のエッジケース
    test({ { 1, 1 } }, 50000);
    test({ { 1, 100 } }, 50000);

    // 選択肢3つ
    test({ { 1, 1 }, { 2, 2 }, { 3, 3 } }, 500000);
    test({ { 1, 3 }, { 2, 2 }, { 3, 1 } }, 500000); // 逆パターン
    test({ { 1, 13 }, { 2, 37 }, { 3, 23 } }, 500000); // 乱雑なprob

    // 選択肢1000個
    std::vector<std::tuple<int, int>> v;
    for (int i = 0; i < 1000; i++) {
        v.emplace_back(i + 1, i + 1);
    }

    test(v, 50000);

    // 逆順
    std::reverse(v.begin(), v.end());
    test(v, 50000);

    std::uniform_int_distribution<> dist(1, 100);

    // 選択肢1000個がランダムなprobを持つ
    v.clear();
    for (int i = 0; i < 1000; i++) {
        v.emplace_back(i + 1, dist(mt));
    }
    test(v, 50000);

    // その1000個の選択肢 + IDかぶり項目がある場合
    v.emplace_back(320, dist(mt));
    v.emplace_back(673, dist(mt));

    test(v, 50000);

    return 0;
}

int main()
{
    while (true) {
        test_main();
    }
}
