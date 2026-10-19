/*!
 * @file scoped-input-term.h
 * @brief 入力を待つ処理のテストのために、メモリ上の端末とプレイヤーを用意する
 */

#pragma once

#include "cmd-io/macro-util.h"
#include "game-option/cheat-options.h"
#include "game-option/input-options.h"
#include "game-option/map-screen-options.h"
#include "io/command-repeater.h"
#include "io/input-context.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "io/macro-configurations-store.h"
#include "io/signal-handlers.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/gameterm.h"
#include "term/z-term.h"
#include "test/scoped-restore.h"
#include "util/enum-range.h"
#include "util/finalizer.h"
#include "util/flag-group.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <doctest/doctest.h>
#include <exception>
#include <stdexcept>
#include <string>
#include <utility>

namespace test {

/*!
 * @brief 更新フラグのうち、指定した型のフラグをすべて覚えておき、スコープを抜けるときに元の状態に戻す
 * @tparam Flag フラグの列挙型
 * @return スコープを抜けるときにフラグを戻すファイナライザ
 */
template <RedrawingFlagType Flag>
[[nodiscard]] auto scoped_restore_redrawing_flags()
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    constexpr EnumRange range(static_cast<Flag>(0), Flag::MAX);
    const EnumClassFlagGroup<Flag> all_flags(range);
    EnumClassFlagGroup<Flag> was_set;
    for (const auto flag : range) {
        if (rfu.has(flag)) {
            was_set.set(flag);
        }
    }

    return util::make_finalizer([&rfu, all_flags, was_set] {
        rfu.reset_flags(all_flags);
        rfu.set_flags(was_set);
    });
}

/*!
 * @brief 入力の処理が触るグローバル変数を覚えておき、スコープを抜けるときに元へ戻す
 */
[[nodiscard]] inline auto scoped_restore_input_globals()
{
    return test::scoped_restore(p_ptr, angband_terms, game_term, term_inkey_wait_hook, msg_flag, num_more, command_arg, command_cmd, command_new,
        command_dir, use_menu, inkey_base, inkey_xtra, inkey_scan, inkey_flag, inkey_next, signal_count);
}

/*!
 * @brief 入力の処理が参照するオプション・キーマップ・マクロを覚えておき、スコープを抜けるときに元へ戻す
 */
[[nodiscard]] inline auto scoped_restore_input_options()
{
    return test::scoped_restore(auto_more, skip_more, quick_messages, command_menu, rogue_like_commands, always_repeat, cheat_turn, fresh_once, fresh_message,
        hilite_player, keymap_actions_map, macro_patterns, macro_actions, active_macros);
}

/*!
 * @brief 入力の処理が参照・変更するゲームの進行の状態 (AngbandWorld) を覚えておき、スコープを抜けるときに元へ戻す
 */
[[nodiscard]] inline auto scoped_restore_world_state()
{
    auto &world = AngbandWorld::get_instance();
    return test::scoped_restore(world.timewalk_m_idx, world.character_generated, world.character_dungeon, world.character_saved, world.character_icky_depth);
}

/*!
 * @brief メモリ上の端末とプレイヤーを用意し、入力を待つたびに文脈を確かめてキーを渡す
 * @details 端末がキー入力を待つと、expect() で指定した文脈が公開されているかを確かめ、
 *          指定したキーを 1 つずつ渡す。キーを使い切った後にさらに待つと例外を投げてテストを失敗させるので、
 *          入力を待たないことは、expect() を呼ばずに処理を呼ぶことで確かめられる。
 *          スコープを抜けるときに、キーをすべて渡したことと、入力待ちの文脈が残っていないことを確かめる。
 *          入力の処理が触るグローバル変数と更新フラグは、スコープを抜けるときに元へ戻す。
 *          同時に使えるのは 1 つだけで、テストケースごとに作ること。
 */
class ScopedInputTerm {
public:
    /*!
     * @brief 端末とプレイヤーを用意して、ゲームの端末として登録する
     */
    ScopedInputTerm()
    {
        REQUIRE(current == nullptr);
        REQUIRE_FALSE(get_input_context());
        p_ptr = &this->player_;
        term_init(&this->term, 80, 24, 64);
        this->term.xtra_hook = event_hook;
        this->term.never_bored = true;
        this->term.never_fresh = true;
        angband_terms.fill(nullptr);
        angband_terms[0] = &this->term;
        term_activate(&this->term);
        term_inkey_wait_hook = nullptr;

        msg_flag = false;
        num_more = 0;
        command_arg = 0;
        command_new = 0;
        command_dir = Direction::none();
        use_menu = false;
        inkey_base = false;
        inkey_xtra = false;
        inkey_scan = false;
        inkey_flag = false;
        inkey_next = nullptr;
        signal_count = 0;
        auto_more = false;
        skip_more = false;
        quick_messages = true;
        command_menu = false;
        rogue_like_commands = false;
        always_repeat = false;
        cheat_turn = false;
        fresh_once = false;
        fresh_message = false;
        hilite_player = false;
        keymap_actions_map[KeymapMode::ORIGINAL].assign(256, tl::nullopt);
        keymap_actions_map[KeymapMode::ROGUE].assign(256, tl::nullopt);
        macro_patterns.clear();
        macro_actions.clear();
        active_macros = 0;
        auto &world = AngbandWorld::get_instance();
        world.timewalk_m_idx = 0;
        world.character_generated = false;
        world.character_dungeon = false;
        world.character_saved = false;
        world.character_icky_depth = 0;

        // 数量の入力が前のコマンドの繰り返しと取り違えないよう、繰り返しの記録を '.' だけにする
        command_cmd = '.';
        repeat_check();
        current = this;
    }

    // doctest の --abort-after を指定すると CHECK が例外を投げることがあるので、noexcept にしない
    ~ScopedInputTerm() noexcept(false)
    {
        current = nullptr;

        // テストが失敗して例外で抜けるときは、残ったキーや文脈で失敗を重ねて報告しない
        if (std::uncaught_exceptions() == 0) {
            CHECK(this->consumed_all_keys());
            CHECK_FALSE(get_input_context());
        }
    }

    ScopedInputTerm(const ScopedInputTerm &) = delete;
    ScopedInputTerm &operator=(const ScopedInputTerm &) = delete;
    ScopedInputTerm(ScopedInputTerm &&) = delete;
    ScopedInputTerm &operator=(ScopedInputTerm &&) = delete;

    /*!
     * @brief 入力の処理に渡すプレイヤー
     */
    PlayerType &player()
    {
        return this->player_;
    }

    /*!
     * @brief 次に待つ入力の文脈と、待つたびに渡すキーを指定する
     * @param context 待つたびに公開されているはずの文脈
     * @param keys 待つたびに 1 つずつ渡すキー
     * @details 前の expect() で指定したキーが残っていれば、テストを失敗させる。
     */
    void expect(InputContext context, std::string keys)
    {
        CHECK(this->consumed_all_keys());
        this->expected = std::move(context);
        this->keys_ = std::move(keys);
        this->key_index = 0;
    }

private:
    inline static ScopedInputTerm *current = nullptr; //!< 端末のフックから参照する、使用中のインスタンス

    // 端末を破棄する前にグローバル変数を戻すよう、端末より後に宣言する
    PlayerType player_;
    term_type term;
    decltype(scoped_restore_input_globals()) restore_globals = scoped_restore_input_globals();
    decltype(scoped_restore_input_options()) restore_options = scoped_restore_input_options();
    decltype(scoped_restore_world_state()) restore_world = scoped_restore_world_state();
    decltype(scoped_restore_redrawing_flags<MainWindowRedrawingFlag>()) restore_main_flags = scoped_restore_redrawing_flags<MainWindowRedrawingFlag>();
    decltype(scoped_restore_redrawing_flags<SubWindowRedrawingFlag>()) restore_sub_flags = scoped_restore_redrawing_flags<SubWindowRedrawingFlag>();
    decltype(scoped_restore_redrawing_flags<StatusRecalculatingFlag>()) restore_status_flags = scoped_restore_redrawing_flags<StatusRecalculatingFlag>();
    decltype(scoped_restore_redrawing_flags<InventoryArrangementFlag>()) restore_arrangement_flags = scoped_restore_redrawing_flags<InventoryArrangementFlag>();
    InputContext expected{};
    std::string keys_;
    size_t key_index = 0;

    /*!
     * @brief expect() で指定したキーをすべて渡したか
     */
    bool consumed_all_keys() const
    {
        return this->key_index == this->keys_.size();
    }

    /*!
     * @brief 端末が入力を待つたびに呼ばれ、公開されている文脈を確かめてキーを 1 つ渡す
     * @details 待たずにイベントを取り出すだけの呼び出し (v が 0) では、取り出すイベントが無いことを示す 1 を返す。
     */
    static errr event_hook(int action, int v)
    {
        if (action != TERM_XTRA_EVENT) {
            return 0;
        }

        if (v == 0) {
            return 1;
        }

        // 登録した端末はインスタンスと一緒に破棄するので通常は通らないが、使い方を誤ったときに待ち続けないよう例外にする
        if (current == nullptr) {
            throw std::runtime_error("Input wait without ScopedInputTerm");
        }

        if (current->consumed_all_keys()) {
            throw std::runtime_error("Unexpected extra input wait");
        }

        current->check_context();
        REQUIRE(term_key_push(current->keys_[current->key_index++]) == 0);
        return 0;
    }

    /*!
     * @brief 公開されている文脈が expect() で指定したものと一致するかを確かめる
     */
    void check_context() const
    {
        const auto &context = get_input_context();
        REQUIRE(context.has_value());
        CHECK(context->kind == this->expected.kind);
        CHECK(context->prompt == this->expected.prompt);
        CHECK(context->details == this->expected.details);
    }
};

}
