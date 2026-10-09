/*!
 * @brief 設定ファイルの条件式の解析のテスト
 *
 * io/pref-file-expressor.h の process_pref_file_expr() を検証する。
 * 構文の解析は evaluate_condition_expression() に任せているので、そのテストで確かめる。
 * 「$」で始まらない語や、語だけを使う条件式はプレイヤーの情報を使わないので、プレイヤーには nullptr を渡す。
 */

#include "io/pref-file-expressor.h"

#include "autopick/autopick-dirty-flags.h"
#include "autopick/autopick-drawer.h"
#include "autopick/autopick-editor-util.h"
#include "autopick/autopick-inserter-killer.h"
#include "autopick/autopick-util.h"
#include "io/files-util.h"
#include "io/interpret-pref-file.h"
#include "io/read-pref-file.h"
#include "system/player-type-definition.h"
#include "test/info-reader/scoped-reader-state.h"
#include "test/temporary-json-files.h"
#include "util/angband-files.h"
#include "util/finalizer.h"

#include <doctest/doctest.h>

#include <string>
#include <string_view>
#include <vector>

#ifdef SET_UID
#include <pwd.h>
#include <unistd.h>
#endif

namespace {

/*!
 * @brief 条件式を1つ解析する
 * @param expr 条件式
 * @return 解析の結果
 * @details 文字列の終端を越えて読むと AddressSanitizer で検出できるよう、
 * 条件式だけが入る大きさの (終端の NUL の無い) バッファに置いてから解析する。
 */
std::string parse(std::string_view expr)
{
    const std::vector<char> buf(expr.begin(), expr.end());
    return process_pref_file_expr(nullptr, std::string_view(buf.data(), buf.size()));
}

}

TEST_CASE("process_pref_file_expr returns a plain word as is")
{
    CHECK(parse("abc") == "abc");
}

TEST_CASE("process_pref_file_expr compares words with EQU")
{
    CHECK(parse("[EQU abc abc]") == "1");
    CHECK(parse("[EQU abc abd]") == "0");
}

TEST_CASE("process_pref_file_expr rejects an unknown variable")
{
    CHECK(parse("$FOO") == "0");
}

TEST_CASE("process_pref_file_expr_checked distinguishes errors from a false condition")
{
    const auto valid = process_pref_file_expr_checked(nullptr, "[EQU a b]");
    REQUIRE(valid.has_value());
    CHECK(*valid == "0");
    const auto invalid = process_pref_file_expr_checked(nullptr, "[LEQ 0 2O]");
    REQUIRE_FALSE(invalid.has_value());
    CHECK(invalid.error() == ConditionExpressionError::INVALID_NUMBER);
}

TEST_CASE("preference file readers stop and return an error for an invalid condition")
{
    test::TemporaryJsonFiles files;
    test::ScopedReaderState messages;
    const auto restore = util::make_finalizer([directory = ANGBAND_DIR_USER, history = histpref_buf] {
        ANGBAND_DIR_USER = directory;
        histpref_buf = history;
    });
    ANGBAND_DIR_USER = files.directory;
    PlayerType player{};
    player.lev = 10;
    for (const auto expression : { "[GEQ $LEVEL 2O]", "[IOR 1 [EQU $TYPO x]]", "[FOO a]", "[EQU a a" }) {
        files.write_raw("condition.prf", std::string("# first line\n?:") + expression + "\n?:1\nH:after\n");
        for (auto mode = 0; mode < 3; ++mode) {
            CAPTURE(expression);
            CAPTURE(mode);
            histpref_buf = "before";
            const auto result = mode == 0   ? process_pref_file(&player, "condition.prf", true)
                                : mode == 1 ? process_autopick_file(&player, "condition.prf")
                                            : process_histpref_file(&player, "condition.prf");
            CHECK(result > 0);
            REQUIRE(histpref_buf.has_value());
            CHECK(*histpref_buf == "before");
        }
    }
    files.write_raw("parent.prf", "%:child.prf\nH:after\n");
    files.write_raw("child.prf", "?:[LEQ 0 abc]\n");
    for (auto mode = 0; mode < 3; ++mode) {
        CAPTURE(mode);
        histpref_buf = "before";
        const auto result = mode == 0   ? process_pref_file(&player, "parent.prf", true)
                            : mode == 1 ? process_autopick_file(&player, "parent.prf")
                                        : process_histpref_file(&player, "parent.prf");
        CHECK(result > 0);
        REQUIRE(histpref_buf.has_value());
        CHECK(*histpref_buf == "before");
    }
    files.write_raw("condition.prf", "?:[GEQ $LEVEL +10]\nH:accepted\n?:[EQU a b]\nH:skipped\n");
    histpref_buf = "";
    CHECK(process_pref_file(&player, "condition.prf", true) == 0);
    REQUIRE(histpref_buf.has_value());
    CHECK(*histpref_buf == "accepted");
}

TEST_CASE("autopick editor keeps lines after a condition error inactive until it is fixed")
{
    text_body_type body(0, 0);
    for (const auto line : { "?:[FOO a]", "apple", "?:1", "banana" }) {
        body.lines_list.push_back(std::make_unique<std::string>(line));
    }
    body.lines_list.push_back(nullptr);
    body.dirty_flags = DIRTY_EXPRESSION;
    update_autopick_expression_states(nullptr, &body);
    for (auto line = 0; line < 4; ++line) {
        CAPTURE(line);
        CHECK((body.states[line] & LSTAT_BYPASS) != 0);
        CHECK((body.states[line] & LSTAT_EXPRESSION_ERROR) != 0);
    }
    *body.lines_list[0] = "?:0";
    update_autopick_expression_states(nullptr, &body);
    CHECK((body.states[1] & LSTAT_BYPASS) != 0);
    CHECK((body.states[3] & LSTAT_BYPASS) == 0);
    for (auto line = 0; line < 4; ++line) {
        CHECK((body.states[line] & LSTAT_EXPRESSION_ERROR) == 0);
    }
}

TEST_CASE("autopick editor checks included conditions without changing settings")
{
    test::TemporaryJsonFiles files;
    const auto restore = util::make_finalizer([directory = ANGBAND_DIR_USER, history = histpref_buf] {
        ANGBAND_DIR_USER = directory;
        histpref_buf = history;
    });
    ANGBAND_DIR_USER = files.directory;
    histpref_buf = "before";
    files.write_raw("child.prf", "%:nested.prf\nH:side effect\n");
    files.write_raw("nested.prf", "?:[FOO a]\n");
    text_body_type body(0, 0);
    for (const auto line : { "%:child.prf", "apple", "?:1", "banana" }) {
        body.lines_list.push_back(std::make_unique<std::string>(line));
    }
    body.lines_list.push_back(nullptr);
    body.dirty_flags = DIRTY_EXPRESSION;
    update_autopick_expression_states(nullptr, &body);
    for (auto line = 0; line < 4; ++line) {
        CHECK((body.states[line] & LSTAT_EXPRESSION_ERROR) != 0);
        CHECK((body.states[line] & LSTAT_BYPASS) != 0);
    }
    files.write_raw("nested.prf", "?:1\n");
    update_autopick_expression_states(nullptr, &body);
    for (auto line = 0; line < 4; ++line) {
        CHECK((body.states[line] & (LSTAT_EXPRESSION_ERROR | LSTAT_BYPASS)) == 0);
    }
    REQUIRE(histpref_buf.has_value());
    CHECK(*histpref_buf == "before");
    *body.lines_list[0] = "%:missing.prf";
    update_autopick_expression_states(nullptr, &body);
    CHECK((body.states[3] & (LSTAT_EXPRESSION_ERROR | LSTAT_BYPASS)) == 0);
    files.write_raw("child.prf", "?:0\n%:bad.prf\n");
    files.write_raw("bad.prf", "?:[FOO a]\n");
    CHECK(check_autopick_file_conditions(nullptr, "child.prf") == 0);
    // 通常の文字挿入で有効な取り込み先をエラーのあるファイルへ変えても再評価する。
    files.write_raw("bad.prf", "?:[FOO a]\n");
    *body.lines_list[0] = "%:bad.pr";
    body.dirty_flags = DIRTY_EXPRESSION;
    update_autopick_expression_states(nullptr, &body);
    CHECK((body.states[3] & LSTAT_BYPASS) == 0);
    body.dirty_flags = 0;
    body.dirty_line = -1;
    body.cx = static_cast<int>(body.lines_list[0]->size());
    insert_single_letter(&body, 'f');
    update_autopick_expression_states(nullptr, &body);
    CHECK((body.states[3] & LSTAT_EXPRESSION_ERROR) != 0);
    CHECK((body.states[3] & LSTAT_BYPASS) != 0);
    *body.lines_list[0] = "!%:bad.prf";
    body.dirty_flags = DIRTY_EXPRESSION;
    update_autopick_expression_states(nullptr, &body);
    CHECK((body.states[3] & LSTAT_BYPASS) == 0);
    body.dirty_flags = 0;
    body.dirty_line = -1;
    toggle_command_letter(&body, AutopickMethod::AUTOPICK);
    REQUIRE(*body.lines_list[0] == "%:bad.prf");
    update_autopick_expression_states(nullptr, &body);
    CHECK((body.states[3] & LSTAT_EXPRESSION_ERROR) != 0);
    CHECK((body.states[3] & LSTAT_BYPASS) != 0);
    // 入れ子の読めないパスがあっても、親ファイルの後続の条件式を検証する。
    for (const auto length : { 300, 2000 }) {
        files.write_raw("child.prf", "%:" + std::string(length, 'x') + "\n?:[FOO a]\n");
        CHECK(check_autopick_file_conditions(nullptr, "child.prf") == PREF_EXPRESSION_ERROR);
        *body.lines_list[0] = "%:child.prf";
        body.dirty_flags = DIRTY_EXPRESSION;
        update_autopick_expression_states(nullptr, &body);
        CHECK((body.states[3] & LSTAT_EXPRESSION_ERROR) != 0);
    }
#ifdef SET_UID
    // ログイン名が実行ユーザーと異なるCIでも、アクセス可能なホームからチルダ展開を確認する。
    // ファイルはホームへ書かず、一時ディレクトリに置く。
    const auto *account = getpwuid(getuid());
    REQUIRE(account != nullptr);
    const auto tilde = "~" + std::string(account->pw_name) + "/";
    const auto home = std::filesystem::canonical(path_parse(tilde));
    const auto target = std::filesystem::canonical(files.directory / "bad.prf");
    const auto include = tilde + target.lexically_relative(home).generic_string();
    CAPTURE(include);
    REQUIRE(std::filesystem::equivalent(path_parse(include), target));
    files.write_raw("root.prf", "%:" + include + "\nH:after\n");
    test::ScopedReaderState messages;
    CHECK(process_autopick_file(nullptr, "root.prf") == PREF_EXPRESSION_ERROR);
    CHECK(check_autopick_file_conditions(nullptr, include) == PREF_EXPRESSION_ERROR);
    *body.lines_list[0] = "%:" + include;
    body.dirty_flags = DIRTY_EXPRESSION;
    update_autopick_expression_states(nullptr, &body);
    CHECK((body.states[3] & LSTAT_EXPRESSION_ERROR) != 0);
#endif
    CHECK(check_autopick_file_conditions(nullptr, std::string(2000, 'x')) < 0);
    std::filesystem::create_directory(files.directory / "directory.prf");
    CHECK(check_autopick_file_conditions(nullptr, "directory.prf") < 0);
#ifndef _WIN32
    CHECK(check_autopick_file_conditions(nullptr, "/dev/zero") < 0);
    CHECK(check_autopick_file_conditions(nullptr, "/dev/tty") < 0);
#endif
}

TEST_CASE("autopick editor and reader use the same include recursion boundary")
{
    test::TemporaryJsonFiles files;
    test::ScopedReaderState messages;
    const auto restore = util::make_finalizer([directory = ANGBAND_DIR_USER, history = histpref_buf] {
        ANGBAND_DIR_USER = directory;
        histpref_buf = history;
    });
    ANGBAND_DIR_USER = files.directory;
    for (auto level = 1; level <= 21; ++level) {
        files.write_raw("f" + std::to_string(level) + ".prf", "%:f" + std::to_string(level + 1) + ".prf\n");
    }
    files.write_raw("f22.prf", "?:[FOO a]\n");
    files.write_raw("root.prf", "%:f1.prf\nH:after\n");
    histpref_buf = "";
    CHECK(process_autopick_file(nullptr, "root.prf") == 0);
    REQUIRE(histpref_buf.has_value());
    CHECK(*histpref_buf == "after");
    CHECK(check_autopick_file_conditions(nullptr, "f1.prf") == 0);
    files.write_raw("f21.prf", "?:[FOO a]\n");
    CHECK(process_autopick_file(nullptr, "root.prf") == PREF_EXPRESSION_ERROR);
    CHECK(check_autopick_file_conditions(nullptr, "f1.prf") == PREF_EXPRESSION_ERROR);
}
