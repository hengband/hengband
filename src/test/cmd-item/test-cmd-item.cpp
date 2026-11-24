#include "cmd-item/cmd-item.h"
#include "io/input-context.h"
#include "io/input-key-requester.h"
#include "test/scoped-input-term.h"
#include <doctest/doctest.h>

namespace {
#ifdef JP
#ifdef UTF8
constexpr auto INVENTORY_PROMPT = "\xe6\x8c\x81\xe3\x81\xa1\xe7\x89\xa9\xef\xbc\x9a \xe5\x90\x88\xe8\xa8\x88   0.0 kg (\xe9\x99\x90\xe7\x95\x8c\xe3\x81\xae\x30%) \xe3\x82\xb3\xe3\x83\x9e\xe3\x83\xb3\xe3\x83\x89:"; // 持ち物： 合計   0.0 kg (限界の0%) コマンド:
#endif
#else
constexpr auto INVENTORY_PROMPT = "Inventory: carrying 0.0 pounds (0% of capacity). Command:";
#endif
}

TEST_CASE("do_cmd_inven publishes the command context and passes the key to the next command")
{
    test::ScopedInputTerm term;

    SUBCASE("the player's turn")
    {
        term.expect({ InputKind::COMMAND, INVENTORY_PROMPT, CommandInputDetails{ .shopping = false, .item_list = ItemSelectionArea::INVENTORY } }, "q");
        do_cmd_inven(&term.player(), false);
        CHECK(command_new == 'q');
        CHECK(command_see);
    }

    SUBCASE("in a store")
    {
        term.expect({ InputKind::COMMAND, INVENTORY_PROMPT, CommandInputDetails{ .shopping = true, .item_list = ItemSelectionArea::INVENTORY } }, "s");
        do_cmd_inven(&term.player(), true);
        CHECK(command_new == 's');
        CHECK(command_see);
    }

    SUBCASE("ESC closes the list")
    {
        term.expect({ InputKind::COMMAND, INVENTORY_PROMPT, CommandInputDetails{ .shopping = false, .item_list = ItemSelectionArea::INVENTORY } }, "\033");
        do_cmd_inven(&term.player(), false);
        CHECK(command_new == 0);
        CHECK_FALSE(command_see);
        CHECK(command_gap == 80 - 30); // 端末の幅 - 30
    }
}
