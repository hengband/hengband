/*!
 * @brief 必殺剣の一覧で選んだものから技の ID を引く処理 (cmd-action/cmd-hissatsu.cpp) のテスト
 */

#include "cmd-action/cmd-hissatsu.h"
#include <array>
#include <doctest/doctest.h>

TEST_CASE("hissatsu_spell_id_of_choice reads a spell id from the menu line")
{
    // 定義に抜けた枠 (1, 3, 4) があり、定義のある技を詰めた一覧と技の ID がずれる
    constexpr std::array selections{ 0, 2, 5 };
    // メニュー表示はカーソルが技の ID を指すので、一覧の添字として読み替えない
    CHECK(hissatsu_spell_id_of_choice(selections, true, 5) == 5);
    CHECK(hissatsu_spell_id_of_choice(selections, true, 2) == 2);
    CHECK(hissatsu_spell_id_of_choice(selections, true, 0) == 0);
    CHECK(hissatsu_spell_id_of_choice(selections, true, 31) == 31);
    CHECK_FALSE(hissatsu_spell_id_of_choice(selections, true, 32));
    CHECK_FALSE(hissatsu_spell_id_of_choice(selections, true, -1));
}

TEST_CASE("hissatsu_spell_id_of_choice reads a spell id from the list label")
{
    constexpr std::array selections{ 0, 2, 5 };
    // 記号は定義のある技を詰めた一覧の添字 (a が 0)
    CHECK(hissatsu_spell_id_of_choice(selections, false, 0) == 0);
    CHECK(hissatsu_spell_id_of_choice(selections, false, 1) == 2);
    CHECK(hissatsu_spell_id_of_choice(selections, false, 2) == 5);
    // 一覧より後ろの記号は選べない (以前は空きの 0 番の技になっていた)
    CHECK_FALSE(hissatsu_spell_id_of_choice(selections, false, 3));
    CHECK_FALSE(hissatsu_spell_id_of_choice(selections, false, 31));
    CHECK_FALSE(hissatsu_spell_id_of_choice(selections, false, -1));
}
