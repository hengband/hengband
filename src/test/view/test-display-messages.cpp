#include "game-option/input-options.h"
#include "io/input-context.h"
#include "test/scoped-input-term.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <doctest/doctest.h>
#include <stdexcept>
#include <string>

namespace {
// メッセージの後に表示する -続く- (英語版は -more-)。2バイト文字は16進エスケープで書く
#ifdef JP
#ifdef SJIS
constexpr auto EXPECTED_MORE_PROMPT = "-\x91\xb1\x82\xad-";
#else
constexpr auto EXPECTED_MORE_PROMPT = "-\xc2\xb3\xa4\xaf-";
#endif
#else
constexpr auto EXPECTED_MORE_PROMPT = "-more-";
#endif
}

TEST_CASE("ScopedMessageCapture collects long repeated diagnostics without input or history")
{
    test::ScopedInputTerm term;
    term.player().playing = false;
    const auto history_size = message_num();
    auto &world = AngbandWorld::get_instance();
    {
        // 捕捉が無ければ履歴に足される状態にして、捕捉中は履歴に残らないことを確かめる。
        world.character_generated = true;
        ScopedMessageCapture diagnostics;
        const std::string long_message(2000, 'x');
        msg_print(long_message);
        msg_print("Failed to load record");
        msg_print("Failed to load record");
        REQUIRE(diagnostics.messages().size() == 3);
        CHECK(diagnostics.messages()[0] == long_message);
        CHECK(diagnostics.messages()[1] == "Failed to load record");
        CHECK(diagnostics.messages()[2] == "Failed to load record");
    }
    world.character_generated = false;
    CHECK(message_num() == history_size);
    // スコープを抜けると通常の -続く- の動作に戻る。
    term.expect({ InputKind::MORE, EXPECTED_MORE_PROMPT, {} }, " ");
    msg_print("Normal message");
    msg_erase();
}

TEST_CASE("ScopedMessageCapture restores an outer capture after an exception")
{
    ScopedMessageCapture outer;
    msg_print("First record");
    try {
        ScopedMessageCapture inner;
        msg_print("Second record");
        REQUIRE(inner.messages().size() == 1);
        CHECK(inner.messages()[0] == "Second record");
        throw std::runtime_error("test");
    } catch (const std::runtime_error &) {
    }
    msg_print("Third record");
    REQUIRE(outer.messages().size() == 2);
    CHECK(outer.messages()[0] == "First record");
    CHECK(outer.messages()[1] == "Third record");
}

TEST_CASE("the -more- prompt publishes its context while not playing")
{
    // skip_more が有効でも、プレイ中でない間は -続く- を待つ
    test::ScopedInputTerm term;
    term.player().playing = false;
    skip_more = true;
    term.expect({ InputKind::MORE, EXPECTED_MORE_PROMPT, {} }, " ");
    msg_print("Message");
    msg_erase();
}

TEST_CASE("the -more- prompt publishes its context inside an outer context and restores it")
{
    // プレイ中は、auto_more・skip_more が無効なら num_more >= 0 のとき -続く- を待つ
    test::ScopedInputTerm term;
    term.player().playing = true;
    const InputContextScope outer(InputKind::CONFIRM, "Outer");
    term.expect({ InputKind::MORE, EXPECTED_MORE_PROMPT, {} }, " ");
    msg_print("Nested message");
    msg_erase();
    const auto &restored = get_input_context();
    REQUIRE(restored);
    CHECK(restored->kind == InputKind::CONFIRM);
    CHECK(restored->prompt == "Outer");
    CHECK(restored->details == InputDetails{});
}
