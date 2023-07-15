#pragma once

#include "core/asking-player.h"
#include "term/screen-processor.h"
#include "util/finalizer.h"
#include <array>
#include <concepts>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

/// @note clang-formatによるconceptの整形が安定していないので抑制しておく
// clang-format off
/*!
 * @brief 型Argのオブジェクトの説明を生成する関数の型Funcを表すコンセプト
 */
template <typename Func, typename Arg>
concept Describer = requires(Func f, Arg a) {
    { std::invoke(f, a) } -> std::convertible_to<std::string>;
};

/*!
 * @brief サイズが既知のコンテナの型を表すコンセプト
 */
template <typename T>
concept SizedContainer = requires(T t) {
    { std::begin(t) } -> std::convertible_to<typename T::iterator>;
    { std::end(t) } -> std::convertible_to<typename T::iterator>;
    std::size(t);
    typename T::value_type;
};
// clang-format on

/*!
 * @brief 候補を選択するためのクラス
 */
class CandidateSelector {
public:
    CandidateSelector(const std::string &prompt, int start_col = 0);

    void set_max_per_page(size_t max_per_page = std::numeric_limits<size_t>::max());

    /*!
     * @brief 引数で与えられた候補リストを画面に表示し選択する
     *
     * 最上行に prompt を表示し、次の行から候補を
     *
     * <pre>
     * a) 候補1
     * b) 候補2
     *    ︙
     * </pre>
     *
     * のように表示する。
     * 候補名は関数 describe_candidate によって生成する。
     *
     * 先頭の記号をキーボードで入力することによって選択する。
     * 与えられた要素の数が max_per_page を超える場合はページ分けを行い、
     * ' ' によって次ページ、'-' によって前ページへの切り替えを行う。
     * ESCキーを押すと選択をキャンセルする。
     *
     * @param candidates 選択する候補
     * @param describe_candidates 候補名を生成する関数
     * @return 選択した要素を指すイテレータ
     *         キャンセルした場合はstd::end(candidates)
     */
    template <SizedContainer Candidates, Describer<typename Candidates::value_type> F>
    typename Candidates::const_iterator select(const Candidates &candidates, F &&describe_candidate)
    {
        const auto candidates_count = std::size(candidates);
        const auto page_max = (candidates_count - 1) / this->max_per_page + 1;
        auto current_page = 0U;

        screen_save();
        const auto finalizer = util::make_finalizer([] { screen_load(); });

        while (true) {
            this->display_page(current_page, candidates, describe_candidate);

            const auto cmd = input_command(this->prompt);
            if (!cmd) {
                return std::end(candidates);
            }

            const auto page_base_idx = current_page * this->max_per_page;
            const auto page_item_count = std::min(this->max_per_page, candidates_count - page_base_idx);

            const auto [new_page, idx] = process_input(*cmd, current_page, page_max);
            if (idx && *idx < page_item_count) {
                return std::next(std::begin(candidates), page_base_idx + *idx);
            }

            current_page = new_page;
        }
    }

private:
    static std::pair<size_t, std::optional<size_t>> process_input(char cmd, size_t current_page, size_t page_max);

    template <SizedContainer Candidates, Describer<typename Candidates::value_type> F>
    void display_page(size_t page, const Candidates &candidates, F &&describe_candidate)
    {
        const auto candidates_count = std::size(candidates);
        const auto page_max = (candidates_count - 1) / this->max_per_page + 1;
        const auto page_base_idx = page * this->max_per_page;
        const auto page_item_count = std::min(this->max_per_page, candidates_count - page_base_idx);

        for (auto i = 0U; i < this->max_per_page + 1; ++i) {
            term_erase(this->start_col, i + 1, 255);
        }

        auto it = std::next(std::begin(candidates), page_base_idx);
        for (auto i = 0U; i < page_item_count; ++i, ++it) {
            std::stringstream ss;
            ss << i2sym[i] << ") " << std::invoke(describe_candidate, *it);
            put_str(ss.str(), i + 1, this->start_col);
        }
        if (page_max > 1) {
            const auto page_info = format("-- more (%lu/%lu) --", page + 1, page_max);
            put_str(page_info, page_item_count + 1, this->start_col);
        }
    }

    static const std::array<char, 62> i2sym;

    std::string prompt;
    int start_col;
    size_t max_per_page;
};
