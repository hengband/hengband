/*!
 * @brief スコアファイル (scores.json) の中身の変換と、記録の順位のテスト
 */

#include "core/score-file.h"
#include "io/files-util.h"
#include "test/scoped-restore.h"
#include "test/string-helpers.h"
#include "test/temporary-json-files.h"
#include <algorithm>
#include <doctest/doctest.h>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <iterator>
#include <limits>
#include <string>
#include <string_view>
#include <vector>
#ifdef SET_UID
#include "main-unix/unix-user-ids.h"
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace {
HighScore make_score(int64_t points, std::string_view name = "Henkma")
{
    HighScore score;
    score.version = "3.1.0";
    score.points = points;
    score.gold = 11823427;
    score.turns = 6496289;
    score.date = "2026-11-17";
    score.name = name;
    score.uid = 508;
    score.sex = "f";
    score.race = 8;
    score.pclass = 12;
    score.personality = 3;
    score.level = 50;
    score.dungeon_level = 0;
    score.max_level = 50;
    score.max_dungeon_level = 100;
    score.died_from = "ripe";
    return score;
}

std::vector<int64_t> points_of(const std::vector<HighScore> &scores)
{
    std::vector<int64_t> points;
    for (const auto &score : scores) {
        points.push_back(score.points);
    }

    return points;
}

template <size_t N>
void set_field(char (&field)[N], std::string_view value)
{
    std::copy_n(value.begin(), std::min(value.length(), N), field);
}

/*!
 * @brief テスト用の一時ディレクトリをスコアファイルの置き場にする
 * @details ファイルを書くときの safe_setuid_grab() が今のユーザーとグループに切り替わるよう、起動時と同じく ID を設定する
 */
class ScopedScoreDirectory {
public:
    ScopedScoreDirectory()
        : restore(test::scoped_restore(ANGBAND_DIR_APEX))
    {
        ANGBAND_DIR_APEX = this->files.directory;
#ifdef SET_UID
        auto &ids = UnixUserIds::get_instance();
        this->saved_effective_user_id = ids.get_effective_user_id();
        this->saved_effective_group_id = ids.get_effective_group_id();
        ids.set_effective_user_id(static_cast<int>(geteuid()));
        ids.set_effective_group_id(static_cast<int>(getegid()));
#endif
    }

    ~ScopedScoreDirectory()
    {
#ifdef SET_UID
        auto &ids = UnixUserIds::get_instance();
        ids.set_effective_user_id(this->saved_effective_user_id);
        ids.set_effective_group_id(this->saved_effective_group_id);
#endif
    }

    ScopedScoreDirectory(const ScopedScoreDirectory &) = delete;
    ScopedScoreDirectory(ScopedScoreDirectory &&) = delete;
    ScopedScoreDirectory &operator=(const ScopedScoreDirectory &) = delete;
    ScopedScoreDirectory &operator=(ScopedScoreDirectory &&) = delete;

    test::TemporaryJsonFiles files;

private:
    decltype(test::scoped_restore(ANGBAND_DIR_APEX)) restore;
#ifdef SET_UID
    int saved_effective_user_id = 0;
    int saved_effective_group_id = 0;
#endif
};
}

TEST_CASE("scores_from_json reads back what scores_to_json wrote")
{
    const std::vector<HighScore> scores{ make_score(200), make_score(100, "hoge") };
    const auto read = scores_from_json(scores_to_json(scores));
    REQUIRE(read.has_value());
    REQUIRE(read->size() == 2);
    const auto &score = read->front();
    CHECK(score.version == "3.1.0");
    CHECK(score.points == 200);
    CHECK(score.gold == 11823427);
    CHECK(score.turns == 6496289);
    CHECK(score.date == "2026-11-17");
    CHECK(score.name == "Henkma");
    CHECK(score.uid == 508);
    CHECK(score.sex == "f");
    CHECK(score.race == 8);
    CHECK(score.pclass == 12);
    CHECK(score.personality == 3);
    CHECK(score.level == 50);
    CHECK(score.dungeon_level == 0);
    CHECK(score.max_level == 50);
    CHECK(score.max_dungeon_level == 100);
    CHECK(score.died_from == "ripe");
    CHECK(read->back().name == "hoge");
}

TEST_CASE("HighScore converts strings between the system encoding and UTF-8")
{
    // 内部の文字コードの「漢」は、UTF-8 では E6 BC A2 になる
#ifdef JP
    constexpr auto kanji = test::KANJI_KAN;
#else
    constexpr std::string_view kanji = "\xe6\xbc\xa2";
#endif
    CHECK(HighScore::from_sys(kanji) == "\xe6\xbc\xa2");
    CHECK(HighScore::to_sys("\xe6\xbc\xa2") == kanji);
}

TEST_CASE("scores_to_json keeps UTF-8 strings that the system encoding cannot represent")
{
    // 内部の文字コードに無い文字 (U+1F600) も、読み直して書き直したときに失われない
    constexpr std::string_view grinning_face = "\xf0\x9f\x98\x80";
    auto score = make_score(100, grinning_face);
    score.died_from = grinning_face;
    const auto json = scores_to_json({ score });
    CHECK(json.find(fmt::format("\"name\": \"{}\"", grinning_face)) != std::string::npos);

    const auto read = scores_from_json(json);
    REQUIRE(read.has_value());
    REQUIRE(read->size() == 1);
    CHECK(read->front().name == grinning_face);
    CHECK(read->front().died_from == grinning_face);
    CHECK(scores_to_json(*read) == json);
}

TEST_CASE("scores_from_json treats blank text as no records")
{
    for (const auto text : { "", " \n" }) {
        const auto read = scores_from_json(text);
        REQUIRE(read.has_value());
        CHECK(read->empty());
    }
}

TEST_CASE("scores_from_json ignores unknown fields and fills missing fields with defaults")
{
    const auto read = scores_from_json(R"({"version": 1, "scores": [{"points": 10, "unknown": true}]})");
    REQUIRE(read.has_value());
    REQUIRE(read->size() == 1);
    CHECK(read->front().points == 10);
    CHECK(read->front().name.empty());
    CHECK(read->front().level == 0);
}

TEST_CASE("scores_from_json rejects broken text, newer versions and wrong field types")
{
    CHECK_FALSE(scores_from_json("{").has_value());
    CHECK_FALSE(scores_from_json("[]").has_value());
    CHECK_FALSE(scores_from_json(R"({"version": 1})").has_value());
    CHECK_FALSE(scores_from_json(R"({"version": 1, "scores": null})").has_value());
    CHECK_FALSE(scores_from_json(R"({"version": 1, "scores": {}})").has_value());
    CHECK_FALSE(scores_from_json(R"({"version": 2, "scores": []})").has_value());
    CHECK_FALSE(scores_from_json(R"({"version": 1.5, "scores": []})").has_value());
    CHECK_FALSE(scores_from_json(R"({"version": "1", "scores": []})").has_value());
    CHECK_FALSE(scores_from_json(R"({"version": 1, "scores": [{"points": "10"}]})").has_value());
}

TEST_CASE("scores_from_json rejects races, classes and personalities out of range")
{
    CHECK(scores_from_json(R"({"version": 1, "scores": [{"race": 0, "class": 0, "personality": 0}]})").has_value());
    for (const auto record : { R"({"race": -1})", R"({"race": 999})", R"({"class": 999})", R"({"personality": 999})" }) {
        CHECK_FALSE(scores_from_json(fmt::format(R"({{"version": 1, "scores": [{}]}})", record)).has_value());
    }
}

TEST_CASE("scores_from_json rejects numbers that are not integers or do not fit the fields")
{
    // int に丸めると 8 (範囲内の種族) になる値
    CHECK_FALSE(scores_from_json(R"({"version": 1, "scores": [{"race": 4294967304}]})").has_value());
    CHECK_FALSE(scores_from_json(R"({"version": 1, "scores": [{"points": 1e30}]})").has_value());
    CHECK_FALSE(scores_from_json(R"({"version": 1, "scores": [{"points": 18446744073709551615}]})").has_value());
    CHECK_FALSE(scores_from_json(R"({"version": 1, "scores": [{"level": 1.5}]})").has_value());
    CHECK_FALSE(scores_from_json(R"({"version": 1, "scores": [1]})").has_value());

    const auto read = scores_from_json(R"({"version": 1, "scores": [{"points": 9223372036854775807, "level": -1}]})");
    REQUIRE(read.has_value());
    CHECK((*read)[0].points == std::numeric_limits<int64_t>::max());
    CHECK((*read)[0].level == -1);
}

TEST_CASE("scores_from_json sorts records by points keeping the order of ties")
{
    const auto read = scores_from_json(R"({"version": 1, "scores": [{"points": 100, "name": "a"}, {"points": 300}, {"points": 100, "name": "b"}]})");
    REQUIRE(read.has_value());
    CHECK(points_of(*read) == std::vector<int64_t>{ 300, 100, 100 });
    CHECK((*read)[1].name == "a");
    CHECK((*read)[2].name == "b");
}

TEST_CASE("find_score_rank places a new score after the scores that are not lower")
{
    const std::vector<HighScore> scores{ make_score(300), make_score(200), make_score(200), make_score(100) };
    CHECK(find_score_rank(scores, 400) == 0);
    CHECK(find_score_rank(scores, 200) == 3);
    CHECK(find_score_rank(scores, 150) == 3);
    CHECK(find_score_rank(scores, 50) == 4);
    CHECK(find_score_rank({}, 50) == 0);
}

TEST_CASE("insert_score keeps the records sorted and within MAX_HISCORES")
{
    std::vector<HighScore> scores{ make_score(300), make_score(100) };
    CHECK(insert_score(scores, make_score(200)) == 1);
    CHECK(points_of(scores) == std::vector<int64_t>{ 300, 200, 100 });

    std::vector<HighScore> full(MAX_HISCORES, make_score(100));
    CHECK(insert_score(full, make_score(200)) == 0);
    CHECK(std::ssize(full) == MAX_HISCORES);
    CHECK(full.front().points == 200);

    // 上限まであり、どれよりも低い記録は最下位と入れ替わる
    std::vector<HighScore> lowest(MAX_HISCORES, make_score(100));
    CHECK(insert_score(lowest, make_score(50)) == MAX_HISCORES - 1);
    CHECK(std::ssize(lowest) == MAX_HISCORES);
    CHECK(lowest.back().points == 50);
}

TEST_CASE("migrate_raw_score converts the date of scores.raw")
{
    high_score raw{};
    set_field(raw.day, "@20261117");
    CHECK(migrate_raw_score(raw).date == "2026-11-17");

    // 古い版の日付はそのまま残し、表示のときに読む
    raw = {};
    set_field(raw.day, "06/03/00");
    CHECK(migrate_raw_score(raw).date == "06/03/00");

    raw = {};
    CHECK(migrate_raw_score(raw).date.empty());

    // 前の版の表示と同じく、先頭の空白を除く
    set_field(raw.day, " @20261117");
    CHECK(migrate_raw_score(raw).date == "2026-11-17");
}

TEST_CASE("prepare_score_file migrates scores.raw and keeps it")
{
    ScopedScoreDirectory directory;
    const auto &files = directory.files;
    high_score raws[2]{};
    set_field(raws[0].pts, "      200");
    set_field(raws[0].who, "first");
    set_field(raws[1].pts, "      100");
    set_field(raws[1].who, "second");
    files.write_raw("scores.raw", std::string_view(reinterpret_cast<const char *>(raws), sizeof(raws)));

    REQUIRE(prepare_score_file());
    CHECK(std::filesystem::exists(files.directory / "scores.raw"));
    const auto scores = read_score_file();
    REQUIRE(scores.has_value());
    CHECK(points_of(*scores) == std::vector<int64_t>{ 200, 100 });
    CHECK(scores->front().name == "first");

    // scores.json があれば、scores.raw を読み直さない
    files.write_raw("scores.raw", "");
    REQUIRE(prepare_score_file());
    CHECK(read_score_file()->size() == 2);
}

TEST_CASE("prepare_score_file ignores the bytes after the last whole record of scores.raw")
{
    ScopedScoreDirectory directory;
    const auto &files = directory.files;
    high_score raw{};
    set_field(raw.pts, "      200");
    set_field(raw.who, "first");
    // 前の版と同じく、末尾の 1 件に満たないバイトは記録として読まない
    files.write_raw("scores.raw", std::string(reinterpret_cast<const char *>(&raw), sizeof(raw)).append(10, 'x'));

    REQUIRE(prepare_score_file());
    const auto scores = read_score_file();
    REQUIRE(scores.has_value());
    CHECK(points_of(*scores) == std::vector<int64_t>{ 200 });
}

TEST_CASE("prepare_score_file skips records of scores.raw out of range")
{
    ScopedScoreDirectory directory;
    const auto &files = directory.files;
    high_score raws[2]{};
    set_field(raws[0].pts, "      200");
    set_field(raws[0].p_r, "99");
    set_field(raws[1].pts, "      100");
    files.write_raw("scores.raw", std::string_view(reinterpret_cast<const char *>(raws), sizeof(raws)));

    REQUIRE(prepare_score_file());
    const auto scores = read_score_file();
    REQUIRE(scores.has_value());
    CHECK(points_of(*scores) == std::vector<int64_t>{ 100 });
}

#ifdef SET_UID
TEST_CASE("prepare_score_file does not migrate an empty record set when scores.raw cannot be read")
{
    // root は権限に関わらず読めるので確かめられない
    if (geteuid() == 0) {
        return;
    }

    ScopedScoreDirectory directory;
    const auto &files = directory.files;
    files.write_raw("scores.raw", std::string(sizeof(high_score), '\0'));
    std::filesystem::permissions(files.directory / "scores.raw", std::filesystem::perms::none);

    CHECK_FALSE(prepare_score_file());
    CHECK(std::filesystem::file_size(files.directory / "scores.json") == 0);

    // 読めるようになれば、空の scores.json に記録を移す
    std::filesystem::permissions(files.directory / "scores.raw", std::filesystem::perms::owner_read);
    CHECK(prepare_score_file());
    CHECK(read_score_file()->size() == 1);
}

TEST_CASE("prepare_score_file accepts a score file that can be read but not written")
{
    // root は権限に関わらず書けるので確かめられない
    if (geteuid() == 0) {
        return;
    }

    ScopedScoreDirectory directory;
    const auto &files = directory.files;
    files.write_raw("scores.json", scores_to_json({ make_score(100) }));
    std::filesystem::permissions(files.directory / "scores.json", std::filesystem::perms::owner_read);

    CHECK(prepare_score_file());
    CHECK(points_of(*read_score_file()) == std::vector<int64_t>{ 100 });
    CHECK_FALSE(can_add_score_to_file());
    auto rank = -1;
    CHECK_FALSE(add_score_to_file(make_score(200), rank).has_value());
}

TEST_CASE("prepare_score_file accepts a blank score file that can be read but not written")
{
    // root は権限に関わらず書けるので確かめられない
    if (geteuid() == 0) {
        return;
    }

    ScopedScoreDirectory directory;
    const auto &files = directory.files;
    files.write_raw("scores.json", "");
    std::filesystem::permissions(files.directory / "scores.json", std::filesystem::perms::owner_read);

    CHECK(prepare_score_file());
    CHECK(read_score_file()->empty());
}

TEST_CASE("prepare_score_file shares the score file regardless of umask")
{
    ScopedScoreDirectory directory;
    const auto &files = directory.files;
    const auto old_umask = umask(077);
    const auto prepared = prepare_score_file();
    umask(old_umask);
    REQUIRE(prepared);

    using std::filesystem::perms;
    const auto permission = std::filesystem::status(files.directory / "scores.json").permissions();
    CHECK((permission & (perms::group_read | perms::group_write)) == (perms::group_read | perms::group_write));
}
#endif

TEST_CASE("prepare_score_file writes an empty record set without scores.raw")
{
    ScopedScoreDirectory directory;
    REQUIRE(prepare_score_file());
    const auto scores = read_score_file();
    REQUIRE(scores.has_value());
    CHECK(scores->empty());

    // 起動するたびに移し替えを確かめ直さないよう、記録 0 件を書いておく
    std::ifstream ifs(directory.files.directory / "scores.json", std::ios::binary);
    const std::string text(std::istreambuf_iterator<char>(ifs), {});
    CHECK(text == scores_to_json({}));
}

TEST_CASE("add_score_to_file inserts the score and rewrites the score file")
{
    ScopedScoreDirectory directory;
    const auto &files = directory.files;
    files.write_raw("scores.json", scores_to_json({ make_score(300), make_score(100) }));

    CHECK(can_add_score_to_file());
    auto rank = -1;
    const auto added = add_score_to_file(make_score(200, "new"), rank);
    REQUIRE(added.has_value());
    CHECK(rank == 1);
    CHECK(points_of(*added) == std::vector<int64_t>{ 300, 200, 100 });

    const auto scores = read_score_file();
    REQUIRE(scores.has_value());
    CHECK(points_of(*scores) == std::vector<int64_t>{ 300, 200, 100 });
    CHECK((*scores)[1].name == "new");
}

TEST_CASE("add_score_to_file pads what remains of a longer score file with spaces")
{
    ScopedScoreDirectory directory;
    const auto &files = directory.files;
    // 知らない欄は書き直すときに落ちるので、書き直した中身は前より短くなる
    files.write_raw("scores.json", fmt::format(R"({{"version": 1, "scores": [{{"points": 100, "note": "{}"}}]}})", std::string(1000, 'x')));

    auto rank = -1;
    REQUIRE(add_score_to_file(make_score(200), rank).has_value());
    const auto scores = read_score_file();
    REQUIRE(scores.has_value());
    CHECK(points_of(*scores) == std::vector<int64_t>{ 200, 100 });

    // 古い中身は残らず、末尾の改行の前が空白で埋まる
    std::ifstream ifs(files.directory / "scores.json", std::ios::binary);
    const std::string text(std::istreambuf_iterator<char>(ifs), {});
    CHECK(text.find("xxxx") == std::string::npos);
    CHECK(text.ends_with(" \n"));
    CHECK(text.starts_with(scores_to_json(*scores).substr(0, scores_to_json(*scores).length() - 1)));
}

TEST_CASE("prepare_score_file migrates scores.raw into an empty score file made by the installer")
{
    ScopedScoreDirectory directory;
    const auto &files = directory.files;
    files.write_raw("scores.raw", std::string(sizeof(high_score), '\0'));
    files.write_raw("scores.json", "");

    REQUIRE(prepare_score_file());
    const auto scores = read_score_file();
    REQUIRE(scores.has_value());
    CHECK(scores->size() == 1);
}

TEST_CASE("prepare_score_file writes an empty record set so that scores.raw is migrated only once")
{
    ScopedScoreDirectory directory;
    const auto &files = directory.files;
    files.write_raw("scores.raw", "");

    REQUIRE(prepare_score_file());
    std::ifstream ifs(files.directory / "scores.json", std::ios::binary);
    const std::string text(std::istreambuf_iterator<char>(ifs), {});
    CHECK(text == scores_to_json({}));
}

TEST_CASE("prepare_score_file keeps a score file that has records")
{
    ScopedScoreDirectory directory;
    const auto &files = directory.files;
    files.write_raw("scores.raw", std::string(sizeof(high_score), '\0'));
    files.write_raw("scores.json", scores_to_json({ make_score(300), make_score(100) }));

    REQUIRE(prepare_score_file());
    CHECK(points_of(*read_score_file()) == std::vector<int64_t>{ 300, 100 });
}

TEST_CASE("add_score_to_file does not overwrite a score file that cannot be read")
{
    ScopedScoreDirectory directory;
    const auto &files = directory.files;
    files.write_raw("scores.json", "{");

    auto rank = -1;
    CHECK_FALSE(add_score_to_file(make_score(200), rank).has_value());
    std::ifstream ifs(files.directory / "scores.json", std::ios::binary);
    CHECK(std::string(std::istreambuf_iterator<char>(ifs), {}) == "{");
}

#ifdef JP
namespace {
constexpr std::string_view UTF8_KAN = "\xe6\xbc\xa2"; //!< 漢
#ifdef SJIS
constexpr std::string_view OTHER_KAN = "\xb4\xc1"; //!< EUC-JP の「漢」
constexpr std::string_view HANKAKU_AI = "\xb1\xb2"; //!< ｱｲ
#else
constexpr std::string_view OTHER_KAN = "\x8a\xbf"; //!< Shift_JIS の「漢」
constexpr std::string_view HANKAKU_AI = "\x8e\xb1\x8e\xb2"; //!< ｱｲ
#endif
}

TEST_CASE("migrate_raw_score converts names and causes of death to UTF-8")
{
    // 「漢」の Shift_JIS (8A BF) と EUC-JP (B4 C1) は、どちらも UTF-8 の「漢」(E6 BC A2) になる
    for (const auto bytes : { test::KANJI_KAN, OTHER_KAN }) {
        high_score raw{};
        set_field(raw.how, bytes);
        CHECK(migrate_raw_score(raw).died_from == UTF8_KAN);
    }

    high_score raw{};
    set_field(raw.who, test::KANJI_KAN);
    CHECK(migrate_raw_score(raw).name == UTF8_KAN);
}

#ifdef EUC
TEST_CASE("migrate_raw_score converts a name that cannot be read in the system encoding")
{
    high_score raw{};
    set_field(raw.who, OTHER_KAN);
    CHECK(migrate_raw_score(raw).name == UTF8_KAN);
}
#endif

TEST_CASE("migrate_raw_score keeps a name that can be read in the system encoding")
{
    // 半角カナの並びは、codeconv() が別の文字コードの漢字と取り違えることがある
    high_score raw{};
    set_field(raw.who, HANKAKU_AI);
    CHECK(migrate_raw_score(raw).name == "\xef\xbd\xb1\xef\xbd\xb2");
}

TEST_CASE("migrate_raw_score drops a character cut in the middle at the end")
{
    // 前の版はバイト数で切ったので、末尾に「漢」の先頭のバイトだけが残ることがある
    const auto lead = test::KANJI_KAN.substr(0, 1);
    const auto utf8_kan_kan = test::cat(UTF8_KAN, UTF8_KAN);
    for (const auto bytes : { test::KANJI_KAN, OTHER_KAN }) {
        high_score raw{};
        set_field(raw.how, test::cat(bytes, bytes, bytes.substr(0, 1)));
        CHECK(migrate_raw_score(raw).died_from == utf8_kan_kan);
    }

    high_score raw{};
    set_field(raw.who, test::cat(test::KANJI_KAN, test::KANJI_KAN, lead));
    CHECK(migrate_raw_score(raw).name == utf8_kan_kan);

    // 文字コードを推定できない文字列でも、欠けた文字は落とす
    set_field(raw.how, test::cat("abc", lead));
    CHECK(migrate_raw_score(raw).died_from == "abc");
}
#endif
