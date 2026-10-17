/*!
 * @brief ゲームが待っているキー入力の種類の管理のテスト
 */

#include "io/input-context.h"

#include <doctest/doctest.h>

TEST_CASE("InputContextGuard sets the input context only while it is alive")
{
    CHECK_FALSE(get_input_context().has_value());
    {
        InputContextGuard icg(InputContextKind::COMMAND);
        CHECK(get_input_context() == InputContextKind::COMMAND);
    }

    CHECK_FALSE(get_input_context().has_value());
}

TEST_CASE("InputContextGuard exposes the innermost context and restores the outer one")
{
    InputContextGuard command(InputContextKind::COMMAND);
    {
        InputContextGuard item(InputContextKind::ITEM);
        CHECK(get_input_context() == InputContextKind::ITEM);
        {
            InputContextGuard yes_no(InputContextKind::YES_NO);
            CHECK(get_input_context() == InputContextKind::YES_NO);
        }

        CHECK(get_input_context() == InputContextKind::ITEM);
    }

    CHECK(get_input_context() == InputContextKind::COMMAND);
}
