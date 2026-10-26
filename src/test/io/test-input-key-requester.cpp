#include "io/input-context.h"
#include "io/input-key-requester.h"
#include "test/scoped-input-term.h"
#include <doctest/doctest.h>

TEST_CASE("request_command publishes the command context")
{
    test::ScopedInputTerm term;

    SUBCASE("the player's turn")
    {
        term.expect({ InputKind::COMMAND, "", CommandInputDetails{ false } }, "p");
        InputKeyRequestor(&term.player(), false).request_command();
        CHECK(command_cmd == 'p');
    }

    // 店の中では change_shopping_command() が、買うキー (p・m) を拾う g に、売るキー (s) を置く d に読み替える
    SUBCASE("p in a store")
    {
        term.expect({ InputKind::COMMAND, "", CommandInputDetails{ true } }, "p");
        InputKeyRequestor(&term.player(), true).request_command();
        CHECK(command_cmd == 'g');
    }

    SUBCASE("m in a store")
    {
        term.expect({ InputKind::COMMAND, "", CommandInputDetails{ true } }, "m");
        InputKeyRequestor(&term.player(), true).request_command();
        CHECK(command_cmd == 'g');
    }

    SUBCASE("s in a store")
    {
        term.expect({ InputKind::COMMAND, "", CommandInputDetails{ true } }, "s");
        InputKeyRequestor(&term.player(), true).request_command();
        CHECK(command_cmd == 'd');
    }
}

TEST_CASE("request_command does not wait when a command is pending")
{
    // 持ち物の一覧などから続くコマンドがあれば、入力を待たない
    test::ScopedInputTerm term;
    command_new = 'g';
    InputKeyRequestor(&term.player(), false).request_command();
    CHECK(command_cmd == 'g');
    CHECK(command_new == 0);
}
