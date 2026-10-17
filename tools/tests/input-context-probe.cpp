/*!
 * @brief 実際の入力関数が待機中に公開する文脈と、入力終了後の復元を検証する。
 * @details メモリ上の端末を使う独立プロセス。通常のユニットテストとは別に実行する。
 */
#include "bot/bot-control-server.h"
#include "core/asking-player.h"
#include "io/command-repeater.h"
#include "io/input-key-requester.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "term/z-term.h"
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>

namespace {
BotInputContext expected;
std::string keys;
size_t key_index = 0;
int waits = 0;
int checks = 0;
bool passed = true;

void check(bool condition, const char *description)
{
    ++checks;
    if (!condition) {
        passed = false;
        std::cerr << "FAIL: " << description << '\n';
    }
}

errr event_hook(int action, int)
{
    if (action != TERM_XTRA_EVENT) {
        return 0;
    }

    const auto &context = get_bot_input_context();
    check(context.has_value(), "input context exists during the wait");
    if (context) {
        check(context->kind == expected.kind, "input kind");
        check(context->prompt == expected.prompt, "input prompt");
        if (const auto *text = std::get_if<BotStringInputDetails>(&expected.details)) {
            const auto *actual = std::get_if<BotStringInputDetails>(&context->details);
            check(actual != nullptr, "string details exist");
            if (actual != nullptr) {
                check(actual->max_length == text->max_length, "effective byte limit");
                check(actual->initial == text->initial, "initial text");
            }
        } else if (const auto *quantity = std::get_if<BotQuantityInputDetails>(&expected.details)) {
            const auto *actual = std::get_if<BotQuantityInputDetails>(&context->details);
            check(actual != nullptr, "quantity context is not replaced by string context");
            if (actual != nullptr) {
                check(actual->max == quantity->max, "quantity limit");
                check(actual->initial == quantity->initial, "initial quantity");
            }
        }
    }

    if (key_index >= keys.size()) {
        std::cerr << "Unexpected extra input wait\n";
        std::exit(1);
    }

    ++waits;
    term_key_push(keys[key_index++]);
    return 0;
}

void prepare(BotInputContext context, std::string input)
{
    expected = std::move(context);
    keys = std::move(input);
    key_index = 0;
    waits = 0;
    term_flush();
    command_arg = 0;
    command_cmd = '.';
    repeat_check();
}

void check_finished()
{
    check(waits > 0, "input actually waited");
    check(key_index == keys.size(), "all input keys consumed");
    check(!get_bot_input_context(), "input context cleared after returning");
}
}

int main()
{
    PlayerType player;
    p_ptr = &player;
    term_type term;
    term_init(&term, 80, 24, 64);
    term.xtra_hook = event_hook;
    term.never_bored = true;
    term.never_fresh = true;
    angband_terms[0] = &term;
    term_activate(&term);

    prepare({ "string", "Text: ", BotStringInputDetails{ 20, "seed" } }, "\r");
    check(input_string("Text: ", 20, "seed", false) == "seed", "accept initial text");
    check_finished();

    const std::string long_prompt(75, 'P');
    prepare({ "string", long_prompt, BotStringInputDetails{ 5, "" } }, "abcdefgh\r");
    check(input_string(long_prompt, 20, "", false) == "abcde", "editing obeys effective byte limit");
    check_finished();

    prepare({ "string", "Cancel: ", BotStringInputDetails{ 20, "seed" } }, "\033");
    check(!input_string("Cancel: ", 20, "seed", false), "cancel text input");
    check_finished();

    prepare({ "quantity", "Quantity: ", BotQuantityInputDetails{ 8, 1 } }, "3\r");
    check(input_quantity(8, "Quantity: ") == 3, "accept quantity");
    check_finished();

    command_cmd = 'n';
    repeat_check();
    check(input_quantity(2, "Quantity: ") == 2, "repeat quantity is clamped without waiting");
    check(!get_bot_input_context(), "repeat does not publish quantity context");

    prepare({ "quantity", "Quantity: ", BotQuantityInputDetails{ 8, 1 } }, "\033");
    check(input_quantity(8, "Quantity: ") == 0, "cancel quantity");
    check_finished();

    prepare({ "quantity", "Quantity: ", BotQuantityInputDetails{ 8, 1 } }, "a\r");
    check(input_quantity(8, "Quantity: ") == 8, "alphabetic quantity selects maximum");
    check_finished();

    check(!get_bot_input_context(), "no context before outer scope");
    {
        const BotInputContextScope outer("confirm", "Outer");
        command_arg = 9;
        check(input_quantity(4, "Quantity: ") == 4, "command argument avoids input wait");
        check(command_arg == 0, "command argument consumed");
        check(get_bot_input_context()->kind == "confirm", "argument preserves outer context");
    }
    check(!get_bot_input_context(), "outer context restored");

    {
        const BotInputContextScope outer({ "string", "Outer: ", BotStringInputDetails{ 12, "outer seed" } });
        prepare({ "quantity", "Nested: ", BotQuantityInputDetails{ 5, 1 } }, "2\r");
        check(input_quantity(5, "Nested: ") == 2, "nested quantity accepts input");
        check(waits > 0, "nested quantity actually waited");
        check(key_index == keys.size(), "nested quantity consumed all keys");
        const auto &restored = get_bot_input_context();
        check(restored.has_value(), "outer context exists after nested input");
        if (restored) {
            check(restored->kind == "string", "outer input kind restored");
            check(restored->prompt == "Outer: ", "outer prompt restored");
            const auto *details = std::get_if<BotStringInputDetails>(&restored->details);
            check(details != nullptr, "outer string details restored");
            if (details != nullptr) {
                check(details->max_length == 12, "outer string limit restored");
                check(details->initial == "outer seed", "outer initial text restored");
            }
        }
    }
    check(!get_bot_input_context(), "context cleared after nested input scopes");

    std::cout << "checks=" << checks << " result=" << (passed ? "PASS" : "FAIL") << '\n';
    angband_terms[0] = nullptr;
    game_term = nullptr;
    return passed ? 0 : 1;
}
