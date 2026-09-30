/*!
 * @brief 画面表示とボット向けJSON出力が共有する判定関数のテスト
 *
 * キャラクター画面の技能評価、下部の状態表示の項目判定、アイテム表記の「充填中」判定を検証する。
 * いずれもJSON出力が画面と同じ値を出すために画面側から切り出した関数である。
 */

#include "avatar/avatar.h"
#include "bot/bot-json-output.h"
#include "flavor/flavor-describer.h"
#include "game-option/text-display-options.h"
#include "mutation/mutation-flag-types.h"
#include "system/item/item-entity.h"
#include "system/player-type-definition.h"
#include "util/finalizer.h"
#include "view/status-bars-table.h"
#include "view/status-first-page.h"
#include "window/main-window-stat-poster.h"
#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

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

TEST_CASE("Invalid virtue types use the no-information text without throwing")
{
    const auto restore = util::make_finalizer([saved = show_actual_value] { show_actual_value = saved; });
    PlayerType player;
    player.virtues[0] = 25;
    for (const auto show_value : { false, true }) {
        show_actual_value = show_value;
        for (const auto type : { Virtue::NONE, static_cast<Virtue>(-1), Virtue::MAX, static_cast<Virtue>(static_cast<int>(Virtue::MAX) + 1) }) {
            player.vir_types[0] = type;
            CHECK(describe_virtue(&player, 0) == _("おっと。の情報なし。", "Oops. No info about ."));
        }
    }
}

TEST_CASE("Registered virtue descriptions retain their name and optional numeric value")
{
    const auto restore = util::make_finalizer([saved = show_actual_value] { show_actual_value = saved; });
    PlayerType player;
    player.virtues[0] = 25;
    for (const auto &[type, name] : virtue_names) {
        if (type == Virtue::NONE) {
            continue;
        }
        player.vir_types[0] = type;
        const auto expected = _("[" + name + "]の中徳者", "You are virtuous in " + name + ".");
        show_actual_value = false;
        CHECK(describe_virtue(&player, 0) == expected);
        show_actual_value = true;
        CHECK(describe_virtue(&player, 0) == expected + " (25)");
    }
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

TEST_CASE("Bot proficiency numbers follow independent normal and debug visibility")
{
    CHECK(make_bot_proficiency_values_json(1234, 1000, false, false).empty());
    const auto debug_only = make_bot_proficiency_values_json(1234, 1000, false, true);
    CHECK(debug_only.at("debug_exp") == 1234);
    CHECK_FALSE(debug_only.contains("exp"));
    CHECK_FALSE(debug_only.contains("max"));
    const auto normal = make_bot_proficiency_values_json(1234, 1000, true, false);
    CHECK(normal.at("exp") == 1234);
    CHECK(normal.at("max") == 1000);
    CHECK_FALSE(normal.contains("debug_exp"));
    const auto capped = make_bot_proficiency_values_json(1234, 1000, true, true, true);
    CHECK(capped.at("exp") == 1000);
    CHECK(capped.at("debug_exp") == 1234);
    const auto masked = make_bot_proficiency_values_json(1234, 1000, false, true, true);
    CHECK_FALSE(masked.contains("exp"));
    CHECK(masked.at("debug_exp") == 1234);
}

TEST_CASE("Bot character speed hides a base value during lightspeed and ignores it while riding")
{
    const auto normal = make_bot_character_speed_json(25, 10, false, false);
    CHECK(normal.at("base") == 15);
    CHECK(normal.at("temporary") == 10);
    CHECK_FALSE(normal.at("lightspeed").get<bool>());
    const auto lightspeed = make_bot_character_speed_json(99, 99, true, false);
    CHECK(lightspeed.at("base").is_null());
    CHECK(lightspeed.at("temporary") == 99);
    CHECK(lightspeed.at("lightspeed").get<bool>());
    const auto riding = make_bot_character_speed_json(25, 10, true, true);
    CHECK(riding.at("base") == 15);
    CHECK_FALSE(riding.at("lightspeed").get<bool>());
    CHECK(riding.at("riding").get<bool>());
}

TEST_CASE("Displayed melee rounds hide inactive hands and discard fractional damage")
{
    PlayerType player;
    player.num_blow[0] = 3;
    player.num_blow[1] = 1;
    const int damage[2] = { 125, 734 };
    const auto melee = calc_displayed_melee_statistics(&player, damage);
    CHECK(melee.blows[0] == 3);
    CHECK(melee.blows[1] == 0);
    CHECK(melee.blows[2] == 0);
    CHECK(melee.damage_per_round[0] == 3);
    CHECK(melee.damage_per_round[1] == 0);
    CHECK_FALSE(melee.damage_nil);
    for (const auto mutation : { PlayerMutationType::HORNS, PlayerMutationType::SCOR_TAIL, PlayerMutationType::BEAK, PlayerMutationType::TRUNK, PlayerMutationType::TENTACLES }) {
        player.muta.set(mutation);
    }
    CHECK(calc_displayed_melee_blows(&player)[2] == 5);
    const int zero_damage[2] = { 0, 0 };
    CHECK(calc_displayed_melee_statistics(&player, zero_damage).damage_nil);
}
