/*!
 * @brief 画面表示とボット向けJSON出力が共有する判定関数のテスト
 *
 * キャラクター画面の技能評価、下部の状態表示の項目判定、アイテム表記の「充填中」判定を検証する。
 * いずれもJSON出力が画面と同じ値を出すために画面側から切り出した関数である。
 */

#include "flavor/flavor-describer.h"
#include "game-option/text-display-options.h"
#include "system/item/item-entity.h"
#include "system/player-type-definition.h"
#include "util/finalizer.h"
#include "view/status-bars-table.h"
#include "view/status-first-page.h"
#include "window/main-window-stat-poster.h"
#include <doctest/doctest.h>

TEST_CASE("Skill ratings are classified exactly as the character sheet prints them")
{
    CHECK(classify_skill_rating(-1, 12).first == SkillRating::VERY_BAD);
    CHECK(classify_skill_rating(0, 12).first == SkillRating::BAD);
    CHECK(classify_skill_rating(23, 12).first == SkillRating::BAD);
    CHECK(classify_skill_rating(24, 12).first == SkillRating::POOR);
    CHECK(classify_skill_rating(36, 12).first == SkillRating::FAIR);
    CHECK(classify_skill_rating(59, 12).first == SkillRating::FAIR);
    CHECK(classify_skill_rating(60, 12).first == SkillRating::GOOD);
    CHECK(classify_skill_rating(72, 12).first == SkillRating::VERY_GOOD);
    CHECK(classify_skill_rating(84, 12).first == SkillRating::EXCELLENT);
    CHECK(classify_skill_rating(108, 12).first == SkillRating::SUPERB);
    CHECK(classify_skill_rating(168, 12).first == SkillRating::HEROIC);
    CHECK(classify_skill_rating(215, 12).first == SkillRating::HEROIC);

    const auto [legendary, level] = classify_skill_rating(12 * 20, 12);
    CHECK(legendary == SkillRating::LEGENDARY);
    CHECK(level == 7);

    // 基準比が0以下なら1として扱う (ゼロ除算しない)
    CHECK(classify_skill_rating(5, 0).first == SkillRating::GOOD);
}

TEST_CASE("Skill rating text carries the number only while show_actual_value is on")
{
    const auto restore = util::make_finalizer([saved = show_actual_value] { show_actual_value = saved; });
    show_actual_value = false;
    const auto [hidden_text, hidden_color] = describe_skill_rating(60, 12);
    CHECK(hidden_text == _("良い", "Good"));
    CHECK(hidden_color == TERM_YELLOW);

    show_actual_value = true;
    const auto [shown_text, shown_color] = describe_skill_rating(60, 12);
    CHECK(shown_text == std::string(" 60-") + _("良い", "Good"));
    CHECK(shown_color == TERM_YELLOW);
}

TEST_CASE("Status bar flags follow the timed effects the status bar draws")
{
    PlayerType player;
    CHECK_FALSE(collect_status_bar_flags(&player).test(BAR_LEVITATE));
    player.tim_levitation = 10;
    player.word_recall = 15;
    player.tim_res_time = 3;
    const auto flags = collect_status_bar_flags(&player);
    CHECK(flags.test(BAR_LEVITATE));
    CHECK(flags.test(BAR_RECALL));
    CHECK(flags.test(BAR_RESTIME));
    CHECK_FALSE(flags.test(BAR_ULTIMATE));
}

TEST_CASE("Non-rod items show a single '(charging)' without the remaining turns")
{
    ItemEntity item;
    CHECK(calc_displayed_charging_count(item) == 0);
    item.timeout = 123;
    CHECK(calc_displayed_charging_count(item) == 1);
    CHECK_FALSE(calc_displayed_lamp_turns(item).has_value());
}
