#include "game-option/input-options.h"
#include "io/input-context.h"
#include "test/scoped-input-term.h"
#include "view/display-messages.h"
#include <doctest/doctest.h>

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
