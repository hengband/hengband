/*!
 * @brief pref ファイルの1行の解釈のテスト
 *
 * io/interpret-pref-file.h の interpret_pref_file() を検証する。
 * ゲーム内の「"」コマンドはユーザーが入力した1行をそのまま渡すため、
 * 範囲外の番号や数値として解釈できない値を指定しても、配列の範囲外に書き込まず、
 * 例外も送出せずに解釈の失敗 (0以外) を返すことを確かめる。
 */

#include "io/interpret-pref-file.h"
#include "io/macro-configurations-store.h"
#include "term/gameterm.h"
#include "util/finalizer.h"
#include "view/display-symbol.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <iterator>
#include <string_view>

namespace {

/*!
 * @brief pref の行で書き換わる表示設定の配列を退避する
 * @return スコープを抜けるときに元の値へ戻すファイナライザ
 * @details 戻り値は必ず変数で受けること。受けないとその場で元に戻ってしまう。
 */
[[nodiscard]] auto scoped_display_tables()
{
    std::array<byte, sizeof(angband_color_table)> color_table_backup{};
    std::memcpy(color_table_backup.data(), angband_color_table, sizeof(angband_color_table));
    std::array<byte, sizeof(tval_to_attr)> tval_to_attr_backup{};
    std::memcpy(tval_to_attr_backup.data(), tval_to_attr, sizeof(tval_to_attr));

    return util::make_finalizer([color_table_backup, tval_to_attr_backup, ds_bolt_backup = ds_bolt] {
        std::memcpy(angband_color_table, color_table_backup.data(), sizeof(angband_color_table));
        std::memcpy(tval_to_attr, tval_to_attr_backup.data(), sizeof(tval_to_attr));
        ds_bolt = ds_bolt_backup;
    });
}

/*!
 * @brief pref の行で書き換わるマクロトリガーの定義を退避する
 * @return スコープを抜けるときに元の定義へ戻すファイナライザ
 * @details 戻り値は必ず変数で受けること。受けないとその場で元に戻ってしまう。
 */
[[nodiscard]] auto scoped_macro_triggers()
{
    return util::make_finalizer([template_backup = macro_template, modifier_chr_backup = macro_modifier_chr, modifier_names_backup = macro_modifier_names,
                                    trigger_names_backup = macro_trigger_names, keycodes_backup = macro_trigger_keycodes, max_backup = max_macrotrigger] {
        macro_template = template_backup;
        macro_modifier_chr = modifier_chr_backup;
        macro_modifier_names = modifier_names_backup;
        macro_trigger_names = trigger_names_backup;
        macro_trigger_keycodes = keycodes_backup;
        max_macrotrigger = max_backup;
    });
}

/*!
 * @brief pref の1行を解釈する
 * @param line 解釈する行
 * @return interpret_pref_file() の戻り値
 * @details X: / Y: 以外の行はプレイヤーを参照しないため nullptr を渡す。
 */
int interpret(std::string_view line)
{
    return interpret_pref_file(nullptr, line);
}

}

TEST_CASE("interpret_pref_file rejects lines too short to have a command")
{
    CHECK(interpret("") != 0);
    CHECK(interpret("V") != 0);
}

TEST_CASE("interpret_pref_file sets a color within the color table")
{
    const auto restore = scoped_display_tables();

    CHECK(interpret("V:255:1:2:3:0x10") == 0);
    CHECK(angband_color_table[255][0] == 1);
    CHECK(angband_color_table[255][1] == 2);
    CHECK(angband_color_table[255][2] == 3);
    CHECK(angband_color_table[255][3] == 16);
}

TEST_CASE("interpret_pref_file rejects a color number outside the color table")
{
    const auto restore = scoped_display_tables();

    CHECK(interpret("V:256:1:2:3:4") != 0);
    CHECK(interpret("V:-1:1:2:3:4") != 0);
    CHECK(interpret("V:100000:1:2:3:4") != 0);
}

TEST_CASE("interpret_pref_file does not partially update a color on an invalid value")
{
    const auto restore = scoped_display_tables();

    constexpr byte original = 7;
    std::fill(std::begin(angband_color_table[0]), std::end(angband_color_table[0]), original);
    CHECK(interpret("V:0:1:2:x:4") != 0);
    CHECK(std::all_of(std::begin(angband_color_table[0]), std::end(angband_color_table[0]), [](byte value) { return value == original; }));
}

TEST_CASE("interpret_pref_file sets a special symbol within the table")
{
    const auto restore = scoped_display_tables();

    CHECK(interpret("S:0:1/65") == 0);
    CHECK(ds_bolt[0].color == 1);
    CHECK(ds_bolt[0].character == 'A');
}

TEST_CASE("interpret_pref_file rejects a special symbol number outside the table")
{
    const auto restore = scoped_display_tables();

    CHECK(interpret("S:256:1/65") != 0);
    CHECK(interpret("S:-1:1/65") != 0);
}

TEST_CASE("interpret_pref_file keeps wrapping an item kind number for its attribute")
{
    const auto restore = scoped_display_tables();

    // 128 以上の番号は従来どおり 128 で割った余りの位置に設定する
    CHECK(interpret("E:130:5") == 0);
    CHECK(tval_to_attr[2] == 5);
}

TEST_CASE("interpret_pref_file rejects a negative item kind number for its attribute")
{
    const auto restore = scoped_display_tables();

    CHECK(interpret("E:-1:5") != 0);
}

TEST_CASE("interpret_pref_file rejects negative ids without throwing")
{
    CHECK(interpret("R:-1:1/65") != 0);
    CHECK(interpret("K:-1:1/65") != 0);
    CHECK(interpret("F:-1:1/65") != 0);
}

TEST_CASE("interpret_pref_file rejects values that are not numbers without throwing")
{
    const auto restore = scoped_display_tables();

    CHECK(interpret("R:abc:1/65") != 0);
    CHECK(interpret("R:99999999999:1/65") != 0);
    CHECK(interpret("S:0:x/65") != 0);
    CHECK(interpret("E:1:x") != 0);
    CHECK(interpret("C:x:a") != 0);
}

TEST_CASE("interpret_pref_file replaces a macro template with more modifier characters than names")
{
    const auto restore = scoped_macro_triggers();

    // 修飾キーの文字列は 14 文字あるが、名前は MAX_MACRO_MOD (12) 個までしか持てない
    CHECK(interpret("T:&_#:ABCDEFGHIJKLMN:a:b:c:d:e:f:g:h:i:j:k:l") == 0);
    CHECK(macro_modifier_chr->length() == 14);

    // テンプレートを定義し直すときに、名前の配列の範囲外を消さない
    CHECK(interpret("T:&_#:NS:control-:shift-") == 0);
    CHECK(macro_modifier_names.at(0) == "control-");
    CHECK(macro_modifier_names.at(1) == "shift-");
    CHECK(macro_modifier_names.at(11).empty());
}

TEST_CASE("interpret_pref_file keeps the macro template when a redefinition is invalid")
{
    const auto restore = scoped_macro_triggers();

    REQUIRE(interpret("T:&_#:NS:control-:shift-") == 0);
    REQUIRE(interpret("T:F1:FFBE") == 0);

    // 修飾キーは2つなのに名前が3つあるので、トークン数が合わない
    CHECK(interpret("T:new:NS:a:b:c") != 0);
    CHECK(macro_template == "&_#");
    CHECK(macro_modifier_chr == "NS");
    CHECK(macro_modifier_names.at(0) == "control-");
    CHECK(macro_modifier_names.at(1) == "shift-");
    CHECK(max_macrotrigger == 1);
    CHECK(macro_trigger_names.at(0) == "F1");
    CHECK(macro_trigger_keycodes.at(ShiftStatus::OFF).at(0) == "FFBE");
}

TEST_CASE("interpret_pref_file clears the macro template with an empty template")
{
    const auto restore = scoped_macro_triggers();

    REQUIRE(interpret("T:&_#:NS:control-:shift-") == 0);
    REQUIRE(interpret("T:F1:FFBE") == 0);

    CHECK(interpret("T::::") == 0);
    CHECK_FALSE(macro_template);
    CHECK_FALSE(macro_modifier_chr);
    CHECK(macro_modifier_names.at(0).empty());
    CHECK(max_macrotrigger == 0);
    CHECK(macro_trigger_names.at(0).empty());
}
