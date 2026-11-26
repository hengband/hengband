/*!
 * @brief 端末の表示に使う表と変換のテスト
 */

#include "term/gameterm.h"

#include "effect/attribute-types.h"
#include "system/system-variables.h"
#include "term/term-color-types.h"
#include "test/scoped-restore.h"
#include "test/scoped-rng.h"
#include "util/point-2d.h"
#include "view/display-symbol.h"

#include <doctest/doctest.h>

#include <set>
#include <string_view>

namespace {
/*!
 * @brief 動かないボルトの表 (ds_bolt の 0x30 から) を、色の番号がそのまま分かる形にして、魔法の色を調べる
 * @param colors 設定ファイルの Z: の行で指定する色の文字の並び
 * @return bolt_pict() が返した色を、何度か呼んで集めたもの
 */
std::set<TERM_COLOR> collect_bolt_colors(std::string_view colors)
{
    const auto restore = test::scoped_restore(ds_bolt, gf_colors, ANGBAND_GRAF);

    // 新しいタイル (new・ne2) では設定ファイルの色を使わないので、文字の表示にする
    ANGBAND_GRAF = "ascii";

    for (auto i = 0; i < 16; i++) {
        ds_bolt[0x30 + i] = DisplaySymbol(static_cast<uint8_t>(i), '*');
    }

    gf_colors[AttributeType::FIRE] = colors;
    xso::rng32 rng(test::DEFAULT_RNG_SEED);
    const Pos2D pos(1, 1);
    std::set<TERM_COLOR> result;
    for (auto i = 0; i < 100; i++) {
        result.insert(bolt_pict(rng, pos, pos, AttributeType::FIRE).color);
    }

    return result;
}
}

TEST_CASE("bolt_pict uses the spell colors in the pref file")
{
    CHECK(collect_bolt_colors("ry") == std::set<TERM_COLOR>{ TERM_RED, TERM_YELLOW });
}

TEST_CASE("bolt_pict uses white for a character which is not a color")
{
    CHECK(collect_bolt_colors("x") == std::set<TERM_COLOR>{ TERM_WHITE });
    CHECK(collect_bolt_colors("rx") == std::set<TERM_COLOR>{ TERM_RED, TERM_WHITE });
}
