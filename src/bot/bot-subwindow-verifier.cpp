/*!
 * @file bot-subwindow-verifier.cpp
 * @brief サブウィンドウの描き直しの漏れを調べる処理の実装
 * @details
 * サブウィンドウは描き直しの要求 (SubWindowRedrawingFlag) が立ったものだけを描くので、
 * 内容が変わる箇所で要求し忘れると古い内容が残る。また、描いても term_fresh() しなければ画面には出ない。
 * 入力待ちの時点でこの 2 つを調べ、食い違いのある行を返す。
 */

#include "bot/bot-subwindow-verifier.h"
#include "bot/bot-screen.h"
#include "core/window-redrawer.h"
#include "game-option/option-flags.h"
#include "io/input-key-acceptor.h"
#include "system/deferred-update-requests.h"
#include "term/gameterm.h"
#include "term/z-term.h"
#include "util/enum-converter.h"
#include <algorithm>
#include <nlohmann/json.hpp>
#include <utility>
#include <vector>

namespace {

/*!
 * @brief 調べる対象のサブウィンドウの番号を集める
 * @return サブウィンドウが 1 種類だけ割り当てられていて、画面に出せる端末の番号
 * @details 画面に出さない端末 (never_fresh) と、ウィンドウが表示されていない端末 (mapped_flag が無い) は、
 * term_fresh() が何もしないので除く (mapped_flag を下ろすのは今は x11 版だけ)。
 * まだ使っていない端末 (active_flag が無い) も、入力待ちで画面に出す処理 (fresh_all_terms()) と同じく除き、
 * 調べるために使い始めることはしない
 * (同じ種類を割り当てた使用中の端末があれば、その種類を描き直すときにゲームと同じく使い始める)。
 * 1 つの端末に複数の種類を割り当てていると、要求された種類だけを描くゲームとは違い、全ての種類を描き直すと
 * 最後に描いた種類の内容になって比べられないので、1 種類だけを割り当てた端末を調べる。
 */
std::vector<size_t> collect_subwindow_indices()
{
    std::vector<size_t> indices;
    for (auto i = 1U; i < angband_terms.size(); ++i) {
        const auto *t = angband_terms[i];
        if ((t != nullptr) && !t->never_fresh && t->mapped_flag && t->active_flag && (g_window_flags[i].count() == 1)) {
            indices.push_back(i);
        }
    }

    return indices;
}

/*!
 * @brief 2 つの画面バッファの行の、桁の範囲の違いを求める
 * @param lhs 比べる画面バッファ
 * @param rhs 比べる画面バッファ (lhs と同じ大きさ)
 * @param t 画面バッファの端末
 * @param y 行番号
 * @param x_min 比べる範囲の最初の桁
 * @param x_max 比べる範囲の最後の桁
 * @return 文字が違えば text、文字は同じで色が違えば color、タイルの下地だけが違えば tile、同じなら nullptr
 * @details タイルの下地は、タイルを使う端末 (always_pict・higher_pict) でしか画面に出ず、画面に出ている内容 (old) にも写されないので、
 * タイルを使う端末でだけ比べる。
 */
const char *find_row_difference(const term_win &lhs, const term_win &rhs, const term_type &t, int y, int x_min, int x_max)
{
    const auto is_same = [y, x_min, x_max](const auto &lhs_cells, const auto &rhs_cells) {
        return std::equal(lhs_cells[y].begin() + x_min, lhs_cells[y].begin() + x_max + 1, rhs_cells[y].begin() + x_min);
    };
    if (!is_same(lhs.c, rhs.c)) {
        return "text";
    }

    if (!is_same(lhs.a, rhs.a)) {
        return "color";
    }

    const auto uses_tiles = t.always_pict || t.higher_pict;
    if (uses_tiles && (!is_same(lhs.tc, rhs.tc) || !is_same(lhs.ta, rhs.ta))) {
        return "tile";
    }

    return nullptr;
}

/*!
 * @brief 食い違う行を JSON にする
 * @param lhs 比べた画面バッファ
 * @param rhs 比べた画面バッファ
 * @param y 行番号
 * @param width 桁数
 * @param difference 行の違い
 * @param lhs_key lhs の内容を入れるキー
 * @param rhs_key rhs の内容を入れるキー
 * @return 行番号 (row)・違い (difference)・両方の内容
 */
nlohmann::json make_row_json(const term_win &lhs, const term_win &rhs, int y, int width, const char *difference, const char *lhs_key, const char *rhs_key)
{
    // 波括弧の初期化子の中で一時オブジェクトを作ると、MSVC が評価順の警告 (C4868) を出すので先に求める
    auto lhs_text = make_bot_screen_line_text(lhs, y, width);
    auto rhs_text = make_bot_screen_line_text(rhs, y, width);
    return {
        { "row", y },
        { "difference", difference },
        { lhs_key, std::move(lhs_text) },
        { rhs_key, std::move(rhs_text) },
    };
}

/*!
 * @brief 描いた内容が画面に出ていない行を集める
 * @param t 調べる端末
 * @return 行ごとの、画面に出ている内容 (shown) と描いた内容 (drawn)
 * @details z-term は、描いてまだ画面に出していない範囲を記録し (total_erase と y1・y2・x1・x2)、term_fresh() で出すと消す。
 * 記録が残っていて、記録のある範囲で画面に出ている内容と描いた内容が違う行を返す。
 */
nlohmann::json make_unflushed_rows_json(const term_type &t)
{
    auto rows = nlohmann::json::array();
    const auto width = static_cast<int>(t.wid);
    const auto height = static_cast<int>(t.hgt);
    for (auto y = 0; y < height; y++) {
        const auto is_pending = t.total_erase || ((t.y1 <= y) && (y <= t.y2) && (t.x1[y] <= t.x2[y]));
        if (!is_pending) {
            continue;
        }

        // 記録のある桁の範囲だけを画面に出すので、その範囲を比べる。右下に書くとスクロールする端末 (icky_corner) は、
        // 右下の隅を画面に出さない (term_fresh() が範囲を詰める) ので除く。
        // 同じ内容で描き直しただけの行は、画面に出しても見た目が変わらないので数えない
        const auto x_min = t.total_erase ? 0 : static_cast<int>(t.x1[y]);
        const auto x_last = (t.icky_corner && (y == height - 1)) ? width - 2 : width - 1;
        const auto x_max = std::min(t.total_erase ? width - 1 : static_cast<int>(t.x2[y]), x_last);
        if (x_min > x_max) {
            continue;
        }

        const auto *difference = find_row_difference(*t.old, *t.scr, t, y, x_min, x_max);
        if (difference != nullptr) {
            rows.push_back(make_row_json(*t.old, *t.scr, y, width, difference, "shown", "drawn"));
        }
    }

    return rows;
}

/*!
 * @brief 全て描き直す前と後で食い違う行を集める
 * @param before 描き直す前の画面バッファ
 * @param t 描き直した後の端末
 * @return 行ごとの、描き直す前 (before) と後 (after) の内容
 */
nlohmann::json make_stale_rows_json(const term_win &before, const term_type &t)
{
    auto rows = nlohmann::json::array();
    const auto width = static_cast<int>(t.wid);
    for (auto y = 0; y < static_cast<int>(t.hgt); y++) {
        const auto *difference = find_row_difference(before, *t.scr, t, y, 0, width - 1);
        if (difference != nullptr) {
            rows.push_back(make_row_json(before, *t.scr, y, width, difference, "before", "after"));
        }
    }

    return rows;
}

}

/*!
 * @brief サブウィンドウの描き直しの漏れを調べる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 端末ごとの調べた結果
 * @details
 * サブウィンドウが割り当てられている端末ごとに、次の 2 つを調べる。
 * - unflushed_rows: 描いた内容が画面に出ていない行。inkey() はキーを待つ前に使っている全ての端末を画面に出す
 *   (fresh_all_terms()) ので、その処理がある端末を飛ばす場合や、入力待ちの間に描いて画面に出さなかった場合に出る
 * - stale_rows: 全てのサブウィンドウを描き直すと内容が変わる行 (描き直しの要求の漏れ)
 *
 * 後者を調べるために、割り当てられている種類のサブウィンドウを全て描き直して画面に出すので、調べた後は漏れが無い状態になる。
 * 描き直しには副作用 (思い出や HP・MP の表示の比較の基準が更新される、視界内のモンスターの一覧を描くと BGM を選び直すなど) もあるため、
 * 検証のためだけに使う。
 * コマンドの入力待ちの間 (サブウィンドウの描き直しを済ませた後) に呼ぶ。
 */
nlohmann::json verify_bot_subwindows(PlayerType *player_ptr)
{
    // 描き直す前の様子を端末ごとに覚えておく
    // term_win は既定のコンストラクタを持たないので、コンストラクタを定義する (集成体にすると MSVC が C4623 を出す)
    struct TermSnapshot {
        TermSnapshot(size_t term_index, nlohmann::json rows, const term_win &win)
            : index(term_index)
            , unflushed_rows(std::move(rows))
            , before_redrawing(win)
        {
        }

        size_t index; //!< 端末の番号
        nlohmann::json unflushed_rows; //!< 描いた内容が画面に出ていない行
        term_win before_redrawing; //!< 描き直す前の描いた内容
    };
    std::vector<TermSnapshot> snapshots;
    EnumClassFlagGroup<SubWindowRedrawingFlag> shown_flags;
    for (const auto index : collect_subwindow_indices()) {
        const auto &t = *angband_terms[index];
        snapshots.emplace_back(index, make_unflushed_rows_json(t), *t.scr);
        shown_flags.set(g_window_flags[index]);
    }

    auto terms = nlohmann::json::array();
    if (snapshots.empty()) {
        return { { "consistent", true }, { "terms", std::move(terms) } };
    }

    // 入力待ちで描き直すとき (all_term_fresh()) と同じく、描き直しでずれたメイン画面のカーソルを後で戻す
    auto *old_term = game_term;
    term_activate(angband_terms[0]);
    const auto cursor = term_locate();

    // 割り当てのない種類まで要求すると、描かれないまま要求が残ってしまう
    auto &dur = DeferredUpdateRequests::get_instance();
    auto requested_flags = shown_flags;
    requested_flags.reset(dur.get_sub_intersection(shown_flags));
    const auto restore = [&dur, requested_flags, old_term, cursor] {
        // 描く処理の無い種類 (記念写真など) の要求は残るので、ここで要求したものは下ろしておく
        dur.reset_flags(requested_flags);

        // 描いた端末は描くたびに画面に出しているが、画面に出していない内容が残っていても、調べた後は画面に出しておく。
        // 入力待ちと同じく、カーソルを戻すためにメイン画面を最後に出す
        term_activate(angband_terms[0]);
        term_gotoxy(cursor.first, cursor.second);
        fresh_all_terms();
        term_activate(old_term);
    };

    // 描き直しの途中で例外が出ても (制御サーバは応答をエラーにしてゲームを続ける)、要求・端末・カーソルを元に戻す。
    // デストラクタで戻すと、戻す処理の例外で終了してしまうので、ここで捕まえて戻してから投げ直す
    dur.set_flags(requested_flags);
    try {
        window_stuff_including_deferred(player_ptr);
    } catch (...) {
        // 戻す処理でも例外が出たら、元の例外を伝えるために戻す処理の例外は捨てる
        try {
            restore();
        } catch (...) {
        }

        throw;
    }

    restore();

    auto is_consistent = true;
    for (auto &snapshot : snapshots) {
        auto stale_rows = make_stale_rows_json(snapshot.before_redrawing, *angband_terms[snapshot.index]);
        if (!snapshot.unflushed_rows.empty() || !stale_rows.empty()) {
            is_consistent = false;
        }

        const auto window_flag = *g_window_flags[snapshot.index].first();
        const auto was_requested = !requested_flags.has(window_flag);
        terms.push_back({
            { "term", snapshot.index },
            { "window_flag", enum2i(window_flag) },
            { "was_requested", was_requested },
            { "unflushed_rows", std::move(snapshot.unflushed_rows) },
            { "stale_rows", std::move(stale_rows) },
        });
    }

    return { { "consistent", is_consistent }, { "terms", std::move(terms) } };
}
