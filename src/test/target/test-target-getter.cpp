#include "floor/geometry.h"
#include "io/input-context.h"
#include "target/target-getter.h"
#include "target/target.h"
#include "test/scoped-input-term.h"
#include "test/string-helpers.h"
#include <doctest/doctest.h>
#include <string>

namespace {
// 方向の入力のプロンプト。2バイト文字は16進エスケープで書く
#ifdef JP
#ifdef SJIS
constexpr auto HOUKOU = "\x95\xfb\x8c\xfc"; // 方向
constexpr auto ESC_DE_CHUUDAN = "ESC\x82\xc5\x92\x86\x92\x66"; // ESCで中断
constexpr auto ASHIMOTO = "\x91\xab\x8c\xb3"; // 足元
constexpr auto DE_TARGET = "\x82\xc5\x83\x5e\x81\x5b\x83\x51\x83\x62\x83\x67"; // でターゲット
constexpr auto SENTAKU = "\x91\x49\x91\xf0"; // 選択
constexpr auto HE = "\x82\xd6"; // へ
constexpr auto SAI = "\x8d\xc4"; // 再
#else
constexpr auto HOUKOU = "\xca\xfd\xb8\xfe"; // 方向
constexpr auto ESC_DE_CHUUDAN = "ESC\xa4\xc7\xc3\xe6\xc3\xc7"; // ESCで中断
constexpr auto ASHIMOTO = "\xc2\xad\xb8\xb5"; // 足元
constexpr auto DE_TARGET = "\xa4\xc7\xa5\xbf\xa1\xbc\xa5\xb2\xa5\xc3\xa5\xc8"; // でターゲット
constexpr auto SENTAKU = "\xc1\xaa\xc2\xf2"; // 選択
constexpr auto HE = "\xa4\xd8"; // へ
constexpr auto SAI = "\xba\xc6"; // 再
#endif
const auto DIRECTION_PROMPT = test::cat(HOUKOU, " (", ESC_DE_CHUUDAN, ")?");
const auto DIRECTION_AT_FEET_PROMPT = test::cat(HOUKOU, " ('.'", ASHIMOTO, ", ", ESC_DE_CHUUDAN, ")?");
const auto AIM_PROMPT = test::cat(HOUKOU, " ('*'", DE_TARGET, SENTAKU, ", ", ESC_DE_CHUUDAN, ")?");
const auto AIM_WITH_TARGET_PROMPT = test::cat(HOUKOU, " ('5'", DE_TARGET, HE, ", '*'", DE_TARGET, SAI, SENTAKU, ", ", ESC_DE_CHUUDAN, ")?");
#else
const std::string DIRECTION_PROMPT = "Direction (Escape to cancel)?";
const std::string DIRECTION_AT_FEET_PROMPT = "Direction ('.' at feet, Escape to cancel)?";
const std::string AIM_PROMPT = "Direction ('*' to choose a target, Escape to cancel)?";
const std::string AIM_WITH_TARGET_PROMPT = "Direction ('5' for target, '*' to re-target, Escape to cancel)?";
#endif

InputContext direction_context(const std::string &prompt, DirectionInputDetails details)
{
    return { InputKind::DIRECTION, prompt, details };
}
}

TEST_CASE("get_direction publishes the direction context")
{
    test::ScopedInputTerm term;
    const auto context = direction_context(DIRECTION_PROMPT, { .can_choose_target = false, .has_target = false, .at_feet = false });

    SUBCASE("accept a direction")
    {
        term.expect(context, "6");
        CHECK(get_direction(&term.player()) == Direction(6));
    }

    SUBCASE("wait again after a key that is not a direction")
    {
        term.expect(context, "x2");
        CHECK(get_direction(&term.player()) == Direction(2));
    }

    SUBCASE("wait again after 5")
    {
        term.expect(context, "52");
        CHECK(get_direction(&term.player()) == Direction(2));
    }

    SUBCASE("cancel")
    {
        term.expect(context, "\033");
        CHECK_FALSE(get_direction(&term.player()));
    }
}

TEST_CASE("get_rep_dir publishes whether the player's own grid can be chosen")
{
    test::ScopedInputTerm term;

    const auto at_feet_context = direction_context(DIRECTION_AT_FEET_PROMPT, { .can_choose_target = false, .has_target = false, .at_feet = true });
    const auto not_at_feet_context = direction_context(DIRECTION_PROMPT, { .can_choose_target = false, .has_target = false, .at_feet = false });

    SUBCASE("at feet by .")
    {
        term.expect(at_feet_context, ".");
        CHECK(get_rep_dir(&term.player(), true) == Direction::self());
    }

    SUBCASE("at feet by 5")
    {
        term.expect(at_feet_context, "5");
        CHECK(get_rep_dir(&term.player(), true) == Direction::self());
    }

    SUBCASE("at feet by -")
    {
        term.expect(at_feet_context, "-");
        CHECK(get_rep_dir(&term.player(), true) == Direction::self());
    }

    SUBCASE("not at feet")
    {
        term.expect(not_at_feet_context, "8");
        CHECK(get_rep_dir(&term.player(), false) == Direction(8));
    }

    SUBCASE("wait again after 5 when the player's own grid cannot be chosen")
    {
        term.expect(not_at_feet_context, "58");
        CHECK(get_rep_dir(&term.player(), false) == Direction(8));
    }
}

TEST_CASE("get_aim_dir publishes that a target can be chosen")
{
    // ターゲットが無いので、'5' で今のターゲットへは向けられない
    test::ScopedInputTerm term;
    const auto context = direction_context(AIM_PROMPT, { .can_choose_target = true, .has_target = false, .at_feet = false });

    SUBCASE("accept a direction")
    {
        term.expect(context, "4");
        CHECK(get_aim_dir(&term.player()) == Direction(4));
    }

    SUBCASE("wait again after the target key without a target")
    {
        term.expect(context, "54");
        CHECK(get_aim_dir(&term.player()) == Direction(4));
    }

    SUBCASE("cancel")
    {
        term.expect(context, "\033");
        CHECK_FALSE(get_aim_dir(&term.player()));
    }
}

TEST_CASE("get_aim_dir publishes that the current target can be used")
{
    // 座標のターゲットはフロアを見ずに有効になる
    test::ScopedInputTerm term;
    Target::set_last_target(Target::create_grid_target(&term.player(), { 0, 0 }));
    const auto context = direction_context(AIM_WITH_TARGET_PROMPT, { .can_choose_target = true, .has_target = true, .at_feet = false });

    SUBCASE("accept a direction")
    {
        term.expect(context, "4");
        CHECK(get_aim_dir(&term.player()) == Direction(4));
    }

    SUBCASE("aim at the current target")
    {
        term.expect(context, "5");
        CHECK(get_aim_dir(&term.player()).is_targetting());
    }
}

TEST_CASE("get_direction does not wait when the direction is given")
{
    // 持ち物の一覧などから続くコマンドで、方向が決まっていれば入力を待たない
    test::ScopedInputTerm term;
    command_dir = Direction(3);
    CHECK(get_direction(&term.player()) == Direction(3));
}
