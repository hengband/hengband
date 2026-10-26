#include "cmd-item/cmd-item.h"
#include "io/input-context.h"
#include "io/input-key-requester.h"
#include "test/scoped-input-term.h"
#include <doctest/doctest.h>

namespace {
#ifdef JP
#ifdef SJIS
constexpr auto INVENTORY_PROMPT = "\x8e\x9d\x82\xbf\x95\xa8\x81\x46 \x8d\x87\x8c\x76   0.0 kg (\x8c\xc0\x8a\x45\x82\xcc\x30%) \x83\x52\x83\x7d\x83\x93\x83\x68:"; // 持ち物： 合計   0.0 kg (限界の0%) コマンド:
#else
constexpr auto INVENTORY_PROMPT = "\xbb\xfd\xa4\xc1\xca\xaa\xa1\xa7 \xb9\xe7\xb7\xd7   0.0 kg (\xb8\xc2\xb3\xa6\xa4\xce\x30%) \xa5\xb3\xa5\xde\xa5\xf3\xa5\xc9:"; // 持ち物： 合計   0.0 kg (限界の0%) コマンド:
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
