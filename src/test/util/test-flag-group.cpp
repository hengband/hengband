/*!
 * @brief FlagGroupクラステンプレートのテスト
 *
 * モンスター・アイテム・地形などのフラグ集合を扱う土台であり、
 * セーブファイルの読み書きも担うため、以下を重点的に検証する。
 *
 * - 左辺値版と右辺値版で参照修飾されたメンバ関数が、どちらも意図どおり働くこと
 *   (呼び分けを誤ると変更が黙って捨てられるが、コンパイルは通ってしまう)
 * - セーブファイルとの相互変換が、フラグ数が8の倍数でない場合や
 *   バイト数が食い違う場合でも壊れないこと
 *
 * フラグ数が unsigned long long のビット数以下かどうかでコンストラクタの
 * 実装が分かれるため、7個 (constexpr版) と100個 (非constexpr版) の
 * 2種類のフラグ集合で確かめる。
 */

#include "util/flag-group.h"

#include "util/enum-range.h"

#include <doctest/doctest.h>

#include <cstdint>
#include <iterator>
#include <map>
#include <string>
#include <vector>

namespace {

// 別名の EnumClassFlagGroup を使う。実コードでは FlagGroup を直接書く箇所は少なく、
// ほとんどがこの別名経由で使われるため

//! フラグ数が unsigned long long のビット数以下のフラグ集合。
//! フラグ数を8の倍数から1つ減らし、セーブファイルの端数を検証できるようにしている
enum class SmallFlag {
    ZERO = 0,
    ONE = 1,
    TWO = 2,
    THREE = 3,
    FOUR = 4,
    FIVE = 5,
    SIX = 6,
    MAX = 7,
};
using SmallFlagGroup = EnumClassFlagGroup<SmallFlag>;

//! フラグ数が unsigned long long のビット数より多いフラグ集合
enum class LargeFlag {
    FIRST = 0,
    BEFORE_BOUNDARY = 63,
    AFTER_BOUNDARY = 64,
    LAST = 99,
    MAX = 100,
};
using LargeFlagGroup = EnumClassFlagGroup<LargeFlag>;

//! セーブファイルのバイト数が1バイトに収まらないフラグ集合。
//! 2057フラグは258バイト (0x0102) になり、バイト数の上位バイトが非0になる。
//! 実際のフラグ集合は最大でも164個 (21バイト) だが、書き込み側が16ビットで
//! 出力していることを確かめるために用意する
enum class HugeFlag {
    FIRST = 0,
    MAX = 2057,
};
using HugeFlagGroup = EnumClassFlagGroup<HugeFlag>;

//! セーブファイルの代わりにバイト列へ読み書きするための入れ物
class ByteBuffer {
public:
    //! 1バイト書き込む。wr_byte_func として渡す。
    //! 呼び出し側が渡す型が int だったり unsigned long だったりするため auto で受ける
    auto writer()
    {
        return [this](auto value) { this->bytes.push_back(static_cast<uint8_t>(value)); };
    }

    //! 1バイト読み出す。rd_byte_func として渡す。
    //! 末尾を越えても read_position は進める。頭打ちにすると、
    //! 読み過ぎ (セーブファイルの以降がずれる不具合) を検出できなくなる
    auto reader()
    {
        return [this]() {
            const auto pos = this->read_position++;
            return pos < this->bytes.size() ? this->bytes[pos] : uint8_t{ 0 };
        };
    }

    std::vector<uint8_t> bytes;
    size_t read_position = 0;
};

}

TEST_CASE("FlagGroup has no flag just after default construction")
{
    const SmallFlagGroup flags;

    CHECK(flags.size() == 7);
    CHECK(flags.count() == 0);
    CHECK(flags.none());
    CHECK_FALSE(flags.any());
    CHECK_FALSE(flags.first().has_value());
}

TEST_CASE("FlagGroup can be constructed from a list of flags")
{
    SUBCASE("initializer list")
    {
        const SmallFlagGroup flags = { SmallFlag::ONE, SmallFlag::THREE };
        CHECK(flags.count() == 2);
        CHECK(flags.has(SmallFlag::ONE));
        CHECK(flags.has(SmallFlag::THREE));
        CHECK(flags.has_not(SmallFlag::ZERO));
    }

    SUBCASE("input iterators")
    {
        const std::vector<SmallFlag> list = { SmallFlag::ZERO, SmallFlag::SIX };
        const SmallFlagGroup flags(list.begin(), list.end());
        CHECK(flags.count() == 2);
        CHECK(flags.has(SmallFlag::ZERO));
        CHECK(flags.has(SmallFlag::SIX));
    }

    SUBCASE("enum range")
    {
        // EnumRange は終端を含まない
        const SmallFlagGroup flags(EnumRange(SmallFlag::ONE, SmallFlag::FOUR));
        CHECK(flags.count() == 3);
        CHECK(flags.has(SmallFlag::ONE));
        CHECK(flags.has(SmallFlag::THREE));
        CHECK(flags.has_not(SmallFlag::FOUR));
    }

    SUBCASE("inclusive enum range")
    {
        const SmallFlagGroup flags(EnumRangeInclusive(SmallFlag::ONE, SmallFlag::FOUR));
        CHECK(flags.count() == 4);
        CHECK(flags.has(SmallFlag::FOUR));
    }
}

TEST_CASE("FlagGroup which has more flags than the bits of a long long can be constructed as well")
{
    // フラグ数が unsigned long long のビット数を超えると、constexprでない方の
    // コンストラクタが選択される
    SUBCASE("initializer list")
    {
        const LargeFlagGroup flags = { LargeFlag::FIRST, LargeFlag::AFTER_BOUNDARY, LargeFlag::LAST };
        CHECK(flags.size() == 100);
        CHECK(flags.count() == 3);
        CHECK(flags.has(LargeFlag::FIRST));
        CHECK(flags.has(LargeFlag::AFTER_BOUNDARY));
        CHECK(flags.has(LargeFlag::LAST));
        CHECK(flags.has_not(LargeFlag::BEFORE_BOUNDARY));
    }

    SUBCASE("input iterators")
    {
        const std::vector<LargeFlag> list = { LargeFlag::BEFORE_BOUNDARY, LargeFlag::AFTER_BOUNDARY };
        const LargeFlagGroup flags(list.begin(), list.end());
        CHECK(flags.count() == 2);
        CHECK(flags.has(LargeFlag::BEFORE_BOUNDARY));
        CHECK(flags.has(LargeFlag::AFTER_BOUNDARY));
    }
}

TEST_CASE("FlagGroup treats the MAX flag as not being set")
{
    // どのフラグにも該当しない値としてMAXを渡すことがあるため、
    // 範囲外アクセスにせず常に偽を返す
    SmallFlagGroup flags;
    flags.set(SmallFlag::ZERO);

    CHECK_FALSE(flags.has(SmallFlag::MAX));
    CHECK(flags.has_not(SmallFlag::MAX));
}

TEST_CASE("FlagGroup sets and resets a flag")
{
    SmallFlagGroup flags;

    flags.set(SmallFlag::TWO);
    CHECK(flags.has(SmallFlag::TWO));
    CHECK(flags.count() == 1);

    // 第2引数で値を指定できる
    flags.set(SmallFlag::TWO, false);
    CHECK(flags.has_not(SmallFlag::TWO));
    CHECK(flags.none());

    flags.set(SmallFlag::TWO, true);
    flags.reset(SmallFlag::TWO);
    CHECK(flags.none());
}

TEST_CASE("FlagGroup sets and resets flags by another group")
{
    SmallFlagGroup flags = { SmallFlag::ZERO, SmallFlag::ONE };
    const SmallFlagGroup others = { SmallFlag::ONE, SmallFlag::TWO };

    SUBCASE("set merges the flags of the other group")
    {
        flags.set(others);
        CHECK(flags.count() == 3);
        CHECK(flags.has(SmallFlag::ZERO));
        CHECK(flags.has(SmallFlag::TWO));
    }

    SUBCASE("reset removes the flags of the other group")
    {
        flags.reset(others);
        CHECK(flags.count() == 1);
        CHECK(flags.has(SmallFlag::ZERO));
        CHECK(flags.has_not(SmallFlag::ONE));
    }

    SUBCASE("set and reset by iterators work the same way")
    {
        const std::vector<SmallFlag> list = { SmallFlag::ONE, SmallFlag::TWO };

        flags.set(list.begin(), list.end());
        CHECK(flags.count() == 3);

        flags.reset(list.begin(), list.end());
        CHECK(flags.count() == 1);
        CHECK(flags.has(SmallFlag::ZERO));
    }
}

TEST_CASE("FlagGroup clears all the flags")
{
    SmallFlagGroup flags = { SmallFlag::ZERO, SmallFlag::SIX };

    flags.clear();
    CHECK(flags.none());
    CHECK(flags.count() == 0);
}

TEST_CASE("FlagGroup modifies itself when the modifier is called on an lvalue")
{
    // 左辺値版は自身への参照を返すので、戻り値を経由した変更も同じオブジェクトに効く
    SmallFlagGroup flags;

    auto &returned = flags.set(SmallFlag::ZERO);
    returned.set(SmallFlag::ONE);

    CHECK(flags.has(SmallFlag::ZERO));
    CHECK(flags.has(SmallFlag::ONE));
    CHECK(&returned == &flags);
}

TEST_CASE("FlagGroup returns the modified value when the modifier is called on an rvalue")
{
    // 右辺値版は値を返す。一時オブジェクトに対して連ねた変更が
    // すべて結果に含まれている必要がある
    SUBCASE("chained set")
    {
        const auto flags = SmallFlagGroup().set(SmallFlag::ZERO).set(SmallFlag::TWO);
        CHECK(flags.count() == 2);
        CHECK(flags.has(SmallFlag::ZERO));
        CHECK(flags.has(SmallFlag::TWO));
    }

    SUBCASE("chained reset")
    {
        const auto flags = SmallFlagGroup({ SmallFlag::ZERO, SmallFlag::ONE }).reset(SmallFlag::ZERO);
        CHECK(flags.count() == 1);
        CHECK(flags.has(SmallFlag::ONE));
    }

    SUBCASE("chained clear")
    {
        const auto flags = SmallFlagGroup({ SmallFlag::ZERO }).clear();
        CHECK(flags.none());
    }

    // 以下はフラグ1つを指定する版とは別の関数本体を持つため、個別に確かめる
    SUBCASE("chained set and reset by another group")
    {
        const SmallFlagGroup others = { SmallFlag::ONE, SmallFlag::TWO };

        const auto set_flags = SmallFlagGroup().set(others);
        CHECK(set_flags == others);

        const auto reset_flags = SmallFlagGroup({ SmallFlag::ZERO, SmallFlag::ONE }).reset(others);
        CHECK(reset_flags == SmallFlagGroup({ SmallFlag::ZERO }));
    }

    SUBCASE("chained set and reset by iterators")
    {
        const std::vector<SmallFlag> list = { SmallFlag::ONE, SmallFlag::TWO };

        const auto set_flags = SmallFlagGroup().set(list.begin(), list.end());
        CHECK(set_flags == SmallFlagGroup({ SmallFlag::ONE, SmallFlag::TWO }));

        const auto reset_flags = SmallFlagGroup({ SmallFlag::ZERO, SmallFlag::ONE }).reset(list.begin(), list.end());
        CHECK(reset_flags == SmallFlagGroup({ SmallFlag::ZERO }));
    }
}

TEST_CASE("FlagGroup can be constructed in a constant expression")
{
    // フラグ数が unsigned long long のビット数以下の場合、
    // コンストラクタは constexpr として使える
    constexpr SmallFlagGroup flags = { SmallFlag::ONE, SmallFlag::THREE };
    static_assert(flags.size() == 7);

    CHECK(flags.has(SmallFlag::ONE));
}

TEST_CASE("FlagGroup tells whether it has all/any/none of the given flags")
{
    const SmallFlagGroup flags = { SmallFlag::ZERO, SmallFlag::ONE, SmallFlag::TWO };

    SUBCASE("against another group")
    {
        const SmallFlagGroup subset = { SmallFlag::ZERO, SmallFlag::ONE };
        CHECK(flags.has_all_of(subset));
        CHECK(flags.has_any_of(subset));
        CHECK_FALSE(flags.has_none_of(subset));

        const SmallFlagGroup partial = { SmallFlag::TWO, SmallFlag::THREE };
        CHECK_FALSE(flags.has_all_of(partial));
        CHECK(flags.has_any_of(partial));
        CHECK_FALSE(flags.has_none_of(partial));

        const SmallFlagGroup disjoint = { SmallFlag::THREE, SmallFlag::FOUR };
        CHECK_FALSE(flags.has_all_of(disjoint));
        CHECK_FALSE(flags.has_any_of(disjoint));
        CHECK(flags.has_none_of(disjoint));
    }

    SUBCASE("against a list of flags")
    {
        const std::vector<SmallFlag> subset = { SmallFlag::ZERO, SmallFlag::TWO };

        // イテレータ版は内部で一時的なFlagGroupを構築するため、
        // CHECK に直接書くとMSVCが評価順序の警告 (C4866) を出す
        const auto has_all = flags.has_all_of(subset.begin(), subset.end());
        CHECK(has_all);

        const auto has_any = flags.has_any_of(subset.begin(), subset.end());
        CHECK(has_any);

        const auto has_none = flags.has_none_of(subset.begin(), subset.end());
        CHECK_FALSE(has_none);
    }

    SUBCASE("an empty group is contained by anything")
    {
        const SmallFlagGroup empty;
        CHECK(flags.has_all_of(empty));
        CHECK_FALSE(flags.has_any_of(empty));
        CHECK(flags.has_none_of(empty));
    }
}

TEST_CASE("FlagGroup returns the flag of the smallest number as the first one")
{
    SmallFlagGroup flags = { SmallFlag::TWO, SmallFlag::FIVE };
    CHECK(flags.first() == SmallFlag::TWO);

    flags.reset(SmallFlag::TWO);
    CHECK(flags.first() == SmallFlag::FIVE);

    flags.clear();
    CHECK_FALSE(flags.first().has_value());
}

TEST_CASE("FlagGroup converts itself into a string of 0 and 1")
{
    // 上位番号のフラグから順に並べる
    const SmallFlagGroup flags = { SmallFlag::ZERO, SmallFlag::THREE };

    const auto str = flags.str();
    CHECK(str == "0001001");
    CHECK(str.length() == flags.size());
}

TEST_CASE("FlagGroup converts itself into an integer")
{
    const SmallFlagGroup flags = { SmallFlag::ZERO, SmallFlag::TWO };

    CHECK(flags.to_ulong() == 0b101);
    CHECK(flags.to_ullong() == 0b101);
}

TEST_CASE("FlagGroup compares equal only when the same flags are set")
{
    const SmallFlagGroup flags = { SmallFlag::ZERO, SmallFlag::ONE };

    CHECK(flags == SmallFlagGroup({ SmallFlag::ONE, SmallFlag::ZERO }));
    CHECK(flags != SmallFlagGroup({ SmallFlag::ZERO }));
    CHECK(flags != SmallFlagGroup());
}

TEST_CASE("FlagGroup takes the intersection and the union by the operators")
{
    const SmallFlagGroup lhs = { SmallFlag::ZERO, SmallFlag::ONE };
    const SmallFlagGroup rhs = { SmallFlag::ONE, SmallFlag::TWO };

    SUBCASE("intersection")
    {
        auto flags = lhs;
        flags &= rhs;
        CHECK(flags == SmallFlagGroup({ SmallFlag::ONE }));

        // 元を書き換えない自由関数版もある
        const auto combined = lhs & rhs;
        CHECK(combined == SmallFlagGroup({ SmallFlag::ONE }));
        CHECK(lhs == SmallFlagGroup({ SmallFlag::ZERO, SmallFlag::ONE }));
    }

    SUBCASE("union")
    {
        auto flags = lhs;
        flags |= rhs;
        CHECK(flags == SmallFlagGroup({ SmallFlag::ZERO, SmallFlag::ONE, SmallFlag::TWO }));

        const auto combined = lhs | rhs;
        CHECK(combined == SmallFlagGroup({ SmallFlag::ZERO, SmallFlag::ONE, SmallFlag::TWO }));
        CHECK(lhs == SmallFlagGroup({ SmallFlag::ZERO, SmallFlag::ONE }));
    }
}

TEST_CASE("FlagGroup can be accessed by the subscript operator")
{
    SmallFlagGroup flags;

    flags[SmallFlag::ONE] = true;
    CHECK(flags.has(SmallFlag::ONE));
    CHECK(static_cast<bool>(flags[SmallFlag::ONE]));

    flags[SmallFlag::ONE] = false;
    CHECK(flags.none());
    CHECK_FALSE(static_cast<bool>(flags[SmallFlag::ONE]));
}

TEST_CASE("FlagGroup writes the flags which are set into an output iterator")
{
    const SmallFlagGroup flags = { SmallFlag::FIVE, SmallFlag::ONE };

    std::vector<SmallFlag> result;
    SmallFlagGroup::get_flags(flags, std::back_inserter(result));

    // 番号の小さい順に書き込まれる
    REQUIRE(result.size() == 2);
    CHECK(result[0] == SmallFlag::ONE);
    CHECK(result[1] == SmallFlag::FIVE);
}

TEST_CASE("FlagGroup sets a flag found in a dictionary by its key")
{
    const std::map<std::string, SmallFlag> dict = {
        { "ZERO", SmallFlag::ZERO },
        { "THREE", SmallFlag::THREE },
    };

    SmallFlagGroup flags;

    CHECK(SmallFlagGroup::grab_one_flag(flags, dict, "THREE"));
    CHECK(flags.has(SmallFlag::THREE));

    // 見つからなければ何も変更しない
    CHECK_FALSE(SmallFlagGroup::grab_one_flag(flags, dict, "UNKNOWN"));
    CHECK(flags.count() == 1);
}

TEST_CASE("FlagGroup survives a round trip through the save file format")
{
    // 復元した内容だけでなく、書き込んだバイト数をちょうど消費したことも確かめる。
    // 消費量がずれるとセーブファイルの以降の読み込みがすべて狂う
    SUBCASE("flags which fit in a long long")
    {
        const SmallFlagGroup original = { SmallFlag::ZERO, SmallFlag::THREE, SmallFlag::SIX };

        ByteBuffer buffer;
        wr_FlagGroup(original, buffer.writer());

        SmallFlagGroup restored;
        rd_FlagGroup(restored, buffer.reader());

        CHECK(restored == original);
        CHECK(buffer.read_position == buffer.bytes.size());
    }

    SUBCASE("flags which cross the boundary of a long long")
    {
        const LargeFlagGroup original = { LargeFlag::FIRST, LargeFlag::BEFORE_BOUNDARY, LargeFlag::AFTER_BOUNDARY, LargeFlag::LAST };

        ByteBuffer buffer;
        wr_FlagGroup(original, buffer.writer());

        LargeFlagGroup restored;
        rd_FlagGroup(restored, buffer.reader());

        CHECK(restored == original);
        CHECK(buffer.read_position == buffer.bytes.size());
    }

    SUBCASE("no flag is set")
    {
        const SmallFlagGroup original;

        ByteBuffer buffer;
        wr_FlagGroup(original, buffer.writer());

        SmallFlagGroup restored = { SmallFlag::ONE };
        rd_FlagGroup(restored, buffer.reader());

        CHECK(restored == original);
        CHECK(buffer.read_position == buffer.bytes.size());
    }
}

TEST_CASE("FlagGroup writes the size header as a little endian value")
{
    // バイト数が1バイトに収まらない場合、上位バイトも出力する必要がある。
    // uint8_t に切り詰める回帰が入ると、258バイトが2バイトとして書かれてしまう
    constexpr size_t expected_bytes = 0x0102;

    const HugeFlagGroup flags = { HugeFlag::FIRST };

    ByteBuffer buffer;
    wr_FlagGroup(flags, buffer.writer());

    REQUIRE(buffer.bytes.size() == 2 + expected_bytes);
    CHECK(buffer.bytes[0] == 0x02);
    CHECK(buffer.bytes[1] == 0x01);

    // 書いたものをそのまま読み戻せること
    HugeFlagGroup restored;
    rd_FlagGroup(restored, buffer.reader());

    CHECK(restored == flags);
    CHECK(buffer.read_position == buffer.bytes.size());
}

TEST_CASE("FlagGroup reads the size header as a little endian value")
{
    // 先頭2バイトは下位バイトが先。上下を取り違えると読み出すバイト数が変わり、
    // セーブファイルの以降がずれる。上位バイトが実際に使われることを確かめるため、
    // 2バイトでしか表せないサイズを与える
    constexpr size_t declared_bytes = 0x0102;

    ByteBuffer buffer;
    buffer.bytes = { 0x02, 0x01 };
    buffer.bytes.resize(2 + declared_bytes);

    // フラグ64はフラグ列の9バイト目のビット0
    buffer.bytes[2 + 8] = 0b0000'0001;

    LargeFlagGroup flags;
    rd_FlagGroup(flags, buffer.reader());

    CHECK(buffer.read_position == 2 + declared_bytes);
    CHECK(flags == LargeFlagGroup({ LargeFlag::AFTER_BOUNDARY }));
}

TEST_CASE("FlagGroup writes the number of the bytes of the flags before the flags themselves")
{
    // フラグ数が8の倍数でない場合は切り上げる
    const SmallFlagGroup small;
    ByteBuffer small_buffer;
    wr_FlagGroup(small, small_buffer.writer());

    // 先頭2バイトがバイト数 (下位バイトが先)、続いてフラグのバイト列
    REQUIRE(small_buffer.bytes.size() == 2 + 1);
    CHECK(small_buffer.bytes[0] == 1);
    CHECK(small_buffer.bytes[1] == 0);

    const LargeFlagGroup large;
    ByteBuffer large_buffer;
    wr_FlagGroup(large, large_buffer.writer());

    // 100フラグは13バイト
    REQUIRE(large_buffer.bytes.size() == 2 + 13);
    CHECK(large_buffer.bytes[0] == 13);
    CHECK(large_buffer.bytes[1] == 0);
}

TEST_CASE("FlagGroup leaves the padding bits of the last byte off")
{
    // 7フラグをすべてONにしても、8ビット目は0のまま
    const SmallFlagGroup flags(EnumRange(SmallFlag::ZERO, SmallFlag::MAX));
    REQUIRE(flags.count() == 7);

    ByteBuffer buffer;
    wr_FlagGroup(flags, buffer.writer());

    REQUIRE(buffer.bytes.size() == 3);
    CHECK(buffer.bytes[2] == 0b0111'1111);
}

TEST_CASE("FlagGroup reads and writes a fixed number of bytes")
{
    const SmallFlagGroup original = { SmallFlag::ONE, SmallFlag::FOUR };

    ByteBuffer buffer;
    wr_FlagGroup_bytes(original, buffer.writer(), 1);

    // バイト数は書き込まれず、指定したバイト数だけが並ぶ
    CHECK(buffer.bytes.size() == 1);

    SmallFlagGroup restored;
    rd_FlagGroup_bytes(restored, buffer.reader(), 1);
    CHECK(restored == original);
}

TEST_CASE("FlagGroup writes the specified bytes even if the flags do not fill them")
{
    // 指定バイト数がフラグ集合に必要な数を超えていても、余りを0で埋めて
    // 必ず指定した数だけ書き込む。実際に SubWindowRedrawingFlag (2バイトで足りる) を
    // 4バイト固定で書いている箇所があり、数が減るとセーブファイルがずれる
    const SmallFlagGroup flags = { SmallFlag::ONE };

    ByteBuffer buffer;
    wr_FlagGroup_bytes(flags, buffer.writer(), 3);

    REQUIRE(buffer.bytes.size() == 3);
    CHECK(buffer.bytes[0] == 0b0000'0010);
    CHECK(buffer.bytes[1] == 0);
    CHECK(buffer.bytes[2] == 0);
}

TEST_CASE("FlagGroup consumes the specified bytes even if they do not fit in the flags")
{
    // フラグ集合に収まらないバイトも読み飛ばさずに消費する。
    // 消費しそこねると、セーブファイルの以降の読み込みがすべてずれる
    ByteBuffer buffer;
    buffer.bytes = { 0b0000'0010, 0xff, 0xff };

    SmallFlagGroup flags;
    rd_FlagGroup_bytes(flags, buffer.reader(), 3);

    CHECK(buffer.read_position == 3);
    CHECK(flags == SmallFlagGroup({ SmallFlag::ONE }));
}

TEST_CASE("FlagGroup keeps the flags which are not covered by the bytes read")
{
    // 読み込むバイト数がフラグ集合に足りない場合、残りのフラグは変更されない。
    // 読み込み先をクリアしてから呼ぶ必要がある
    LargeFlagGroup flags = { LargeFlag::LAST };

    ByteBuffer buffer;
    buffer.bytes = { 0b0000'0001 };
    rd_FlagGroup_bytes(flags, buffer.reader(), 1);

    CHECK(flags.has(LargeFlag::FIRST));
    CHECK(flags.has(LargeFlag::LAST));
    CHECK(flags.count() == 2);
}
