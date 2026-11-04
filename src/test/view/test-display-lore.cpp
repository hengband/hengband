/*!
 * @brief モンスターの思い出の表示のテスト
 */

#include "lore/lore-util.h"
#include "monster-race/race-kind-flags.h"
#include "monster-race/race-sex.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "test/scoped-restore.h"
#include "test/string-helpers.h"
#include "test/system/monrace-list-test-access.h"
#include "test/system/monrace-records-test-access.h"
#include "util/enum-converter.h"
#include "view/display-lore.h"
#include <cstdint>
#include <doctest/doctest.h>
#include <string>
#include <string_view>

namespace {

std::string captured_text;

void capture_roff(TERM_COLOR, std::string_view str)
{
    captured_text.append(str);
}

#if defined(JP) && defined(SJIS)
const auto TAG_UNIQUE = test::cat("[", "\x83\x86\x83\x6a\x81\x5b\x83\x4e", "]"); //!< [ユニーク]
const auto TAG_MALE = test::cat("[", "\x92\x6a\x90\xab", "]"); //!< [男性]
const auto TAG_FEMALE = test::cat("[", "\x8f\x97\x90\xab", "]"); //!< [女性]
#elif defined(JP)
const auto TAG_UNIQUE = test::cat("[", "\xa5\xe6\xa5\xcb\xa1\xbc\xa5\xaf", "]"); //!< [ユニーク]
const auto TAG_MALE = test::cat("[", "\xc3\xcb\xc0\xad", "]"); //!< [男性]
const auto TAG_FEMALE = test::cat("[", "\xbd\xf7\xc0\xad", "]"); //!< [女性]
#else
const std::string TAG_UNIQUE = "[UNIQ]";
const std::string TAG_MALE = "[male]";
const std::string TAG_FEMALE = "[female]";
#endif

// 種族の性別から思い出の性別を決める処理を通して、表示までを確かめる
std::string kind_tags_of(MonsterSex sex, bool is_unique = false)
{
    MonraceList::get_instance().get_monrace(static_cast<MonraceId>(1)).init_sex(static_cast<uint32_t>(enum2i(sex)));
    lore_type lore(static_cast<MonraceId>(1), MONSTER_LORE_NORMAL);
    REQUIRE(lore.msex == sex);
    if (is_unique) {
        lore.kind_flags.set(MonsterKindType::UNIQUE);
    }

    captured_text.clear();
    display_monster_kind_tags(&lore);
    return captured_text;
}

}

TEST_CASE("display_monster_kind_tags shows the sex of the monster")
{
    test::MonraceListTestAccess monraces;
    test::MonraceRecordsTestAccess records;
    MonraceList::get_instance().emplace(static_cast<MonraceId>(1));
    const auto restore_hook = test::scoped_restore(hook_c_roff);
    hook_c_roff = capture_roff;

    CHECK(kind_tags_of(MonsterSex::MALE) == test::cat(TAG_MALE, "\n"));
    CHECK(kind_tags_of(MonsterSex::FEMALE) == test::cat(TAG_FEMALE, "\n"));
    CHECK(kind_tags_of(MonsterSex::NONE) == "\n");

    // ユニークでは性別をユニークのタグの直後に表示する
    CHECK(kind_tags_of(MonsterSex::MALE, true) == test::cat(TAG_UNIQUE, TAG_MALE, "\n"));
    CHECK(kind_tags_of(MonsterSex::FEMALE, true) == test::cat(TAG_UNIQUE, TAG_FEMALE, "\n"));
    CHECK(kind_tags_of(MonsterSex::NONE, true) == test::cat(TAG_UNIQUE, "\n"));
}
