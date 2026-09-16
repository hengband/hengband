/*!
 * @brief マクロ表記とキーコード列の相互変換のテスト
 *
 * io/macro-configurations-store.h の text_to_ascii() と ascii_to_text() を検証する。
 * どちらも pref ファイルやユーザーの入力をそのまま受け取るため、
 * 通常の変換に加えて、表記が途中で途切れている場合などに文字列やバッファの範囲外を扱わないことを確かめる。
 */

#include "io/macro-configurations-store.h"

#include <doctest/doctest.h>

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace {

/*!
 * @brief マクロトリガーの定義を差し替え、スコープを抜けるときに元へ戻す
 */
class ScopedMacroTriggers {
public:
    enum class Kind {
        NONE, //!< マクロテンプレートを持たない (GCU 版などの状態)
        X11_LIKE, //!< X11 版の pref ファイルと同じ形式のテンプレートとトリガーを持つ
    };

    /*!
     * @brief 現在の定義を退避し、指定した種類の定義に差し替える
     * @param kind 差し替える定義の種類
     * @details
     * X11_LIKE では T:&_#:NSOM:control-:shift-:alt-:mod2- 相当のテンプレートと、以下のトリガーを定義する。
     * - F1: 通常時 FFBE、Shift 時 SFFBE (Shift の有無でキーコードが変わることを確かめるため、X11 版とは変えている)
     * - Short: FF1 (Escape のキーコードの先頭部分と一致する)
     * - Escape: FF1B
     */
    explicit ScopedMacroTriggers(Kind kind)
        : template_backup(macro_template)
        , modifier_chr_backup(macro_modifier_chr)
        , modifier_names_backup(macro_modifier_names)
        , trigger_names_backup(macro_trigger_names)
        , keycodes_backup(macro_trigger_keycodes)
        , max_backup(max_macrotrigger)
    {
        if (kind == Kind::NONE) {
            macro_template.reset();
            macro_modifier_chr.reset();
            max_macrotrigger = 0;
            return;
        }

        macro_template = "&_#";
        macro_modifier_chr = "NSOM";
        macro_modifier_names.at(0) = "control-";
        macro_modifier_names.at(1) = "shift-";
        macro_modifier_names.at(2) = "alt-";
        macro_modifier_names.at(3) = "mod2-";
        set_trigger(0, "F1", "FFBE", "SFFBE");
        set_trigger(1, "Short", "FF1", "FF1");
        set_trigger(2, "Escape", "FF1B", "FF1B");
        max_macrotrigger = 3;
    }

    ~ScopedMacroTriggers()
    {
        macro_template = this->template_backup;
        macro_modifier_chr = this->modifier_chr_backup;
        macro_modifier_names = this->modifier_names_backup;
        macro_trigger_names = this->trigger_names_backup;
        macro_trigger_keycodes = this->keycodes_backup;
        max_macrotrigger = this->max_backup;
    }

    ScopedMacroTriggers(const ScopedMacroTriggers &) = delete;
    ScopedMacroTriggers &operator=(const ScopedMacroTriggers &) = delete;

private:
    tl::optional<std::string> template_backup;
    tl::optional<std::string> modifier_chr_backup;
    std::vector<std::string> modifier_names_backup;
    std::vector<std::string> trigger_names_backup;
    std::map<ShiftStatus, std::vector<std::string>> keycodes_backup;
    size_t max_backup;

    static void set_trigger(size_t index, std::string_view name, std::string_view keycode, std::string_view shift_keycode)
    {
        macro_trigger_names.at(index) = name;
        macro_trigger_keycodes.at(ShiftStatus::OFF).at(index) = keycode;
        macro_trigger_keycodes.at(ShiftStatus::ON).at(index) = shift_keycode;
    }
};

/*!
 * @brief text_to_ascii() の結果を文字列で受け取る
 * @param text 変換元の文字列
 * @param bufsize 変換先バッファの大きさ
 * @return 変換結果 (NUL終端までの文字列)
 */
std::string to_ascii(std::string_view text, size_t bufsize = 1024)
{
    std::vector<char> buf(bufsize, 'X');
    text_to_ascii(buf.data(), text, bufsize);
    return std::string(buf.data());
}

/*!
 * @brief ascii_to_text() の結果を文字列で受け取る
 * @param keys 変換元のキーコード列
 * @param bufsize 変換先バッファの大きさ
 * @return 変換結果 (NUL終端までの文字列)
 */
std::string to_text(std::string_view keys, size_t bufsize = 1024)
{
    std::vector<char> buf(bufsize, 'X');
    ascii_to_text(buf.data(), keys, bufsize);
    return std::string(buf.data());
}

/*!
 * @brief 後ろに別の文字が続く領域の一部を切り出す
 * @param backing 切り出し元の文字列
 * @param length 切り出す長さ
 * @return backing の先頭 length バイトを指すビュー
 * @details
 * ビューの直後がNULではないため、ビューの長さを無視して読み進める実装では結果が変わる。
 */
std::string_view head(const std::string &backing, size_t length)
{
    return std::string_view(backing).substr(0, length);
}

}

TEST_CASE("text_to_ascii converts key notations")
{
    const ScopedMacroTriggers triggers(ScopedMacroTriggers::Kind::NONE);

    CHECK(to_ascii("abc") == "abc");
    CHECK(to_ascii("a^Ab") == "a\001b");
    CHECK(to_ascii("\\e\\s\\b\\t\\n\\r") == "\x1b \b\t\n\r");
    CHECK(to_ascii("\\\\\\^") == "\\^");
    CHECK(to_ascii("\\x41\\x6a") == "Aj");
    CHECK(to_ascii("\\101\\060") == "A0");
    CHECK(to_ascii("a\\qb") == "ab");
}

TEST_CASE("text_to_ascii drops an incomplete notation at the end of the string")
{
    const ScopedMacroTriggers triggers(ScopedMacroTriggers::Kind::NONE);

    // 表記の続きに当たる文字をビューの外に置き、範囲外を読むと結果が変わるようにする
    for (const auto *text : { "abc^A", "abc\\e", "abc\\x41", "abc\\101" }) {
        const std::string backing(text);
        for (auto length = 3U; length < backing.length(); length++) {
            CAPTURE(text);
            CAPTURE(length);
            CHECK(to_ascii(head(backing, length)) == "abc");
        }
    }
}

TEST_CASE("text_to_ascii stops at a NUL character")
{
    const ScopedMacroTriggers triggers(ScopedMacroTriggers::Kind::NONE);

    const std::string text("ab\0cd", 5);
    CHECK(to_ascii(text) == "ab");
}

TEST_CASE("text_to_ascii truncates the result to fit the buffer")
{
    const ScopedMacroTriggers triggers(ScopedMacroTriggers::Kind::NONE);

    CHECK(to_ascii("abcdef", 4) == "abc");
    CHECK(to_ascii("abcdef", 1) == "");

    // バッファの大きさが0の場合は何も書き込まない
    char untouched = 'X';
    text_to_ascii(&untouched, "abc", 0);
    CHECK(untouched == 'X');
}

TEST_CASE("text_to_ascii converts macro trigger notations")
{
    const ScopedMacroTriggers triggers(ScopedMacroTriggers::Kind::X11_LIKE);

    CHECK(to_ascii("\\[F1]") == "\x1f_FFBE\r");
    CHECK(to_ascii("\\[shift-F1]") == "\x1fS_SFFBE\r");
    CHECK(to_ascii("\\[shift-control-F1]x") == "\x1fNS_SFFBE\rx");
    CHECK(to_ascii("\\[SHIFT-f1]") == "\x1fS_SFFBE\r");
    CHECK(to_ascii("\\[Escape]") == "\x1f_FF1B\r");

    // 定義されていないトリガー名は「]」まで読み飛ばし、テンプレートを使わない形にする
    CHECK(to_ascii("\\[shift-NoSuchKey]x") == "\x1f\rx");
}

TEST_CASE("text_to_ascii does not read beyond an incomplete macro trigger notation")
{
    const ScopedMacroTriggers triggers(ScopedMacroTriggers::Kind::X11_LIKE);

    // 「]」が無い場合は「\[」だけを読み飛ばし、残りは通常の文字として扱う
    const std::string backing("\\[shift-F1]");
    CHECK(to_ascii(head(backing, 2)) == "");
    CHECK(to_ascii(head(backing, 3)) == "s");
    CHECK(to_ascii(head(backing, 8)) == "shift-");
    CHECK(to_ascii(head(backing, 10)) == "shift-F1");
}

TEST_CASE("text_to_ascii ignores empty modifier names")
{
    const ScopedMacroTriggers triggers(ScopedMacroTriggers::Kind::X11_LIKE);

    // 空の修飾キー名は常に一致するため、判定に含めると先へ進まなくなる
    macro_modifier_names.at(1) = "";
    CHECK(to_ascii("\\[F1]") == "\x1f_FFBE\r");
}

TEST_CASE("text_to_ascii skips a macro trigger notation without a template")
{
    const ScopedMacroTriggers triggers(ScopedMacroTriggers::Kind::NONE);

    CHECK(to_ascii("\\[shift-F1]") == "shift-F1]");
}

TEST_CASE("ascii_to_text converts key codes to notations")
{
    const ScopedMacroTriggers triggers(ScopedMacroTriggers::Kind::NONE);

    CHECK(to_text("abc") == "abc");
    CHECK(to_text("\x1b \b\t\n\r") == "\\e\\s\\b\\t\\n\\r");
    CHECK(to_text("^\\") == "\\^\\\\");
    CHECK(to_text("\001\x1f\x7f\xff") == "^A^_\\x7F\\xFF");
}

TEST_CASE("ascii_to_text round-trips with text_to_ascii")
{
    const ScopedMacroTriggers triggers(ScopedMacroTriggers::Kind::X11_LIKE);

    for (const auto *text : { "abc", "\\e\\s^A\\^\\\\\\xFF", "\\[F1]", "\\[shift-F1]", "x\\[control-shift-Escape]y" }) {
        CAPTURE(text);
        CHECK(to_text(to_ascii(text)) == text);
    }
}

TEST_CASE("ascii_to_text converts macro triggers")
{
    const ScopedMacroTriggers triggers(ScopedMacroTriggers::Kind::X11_LIKE);

    CHECK(to_text("\x1fS_SFFBE\r") == "\\[shift-F1]");

    // キーコードは完全一致で照合する (FF1B は、その先頭部分の FF1 を持つ Short ではなく Escape)
    CHECK(to_text("\x1f_FF1B\r") == "\\[Escape]");

    // 定義されていないキーコードや、テンプレートに合わない並びはトリガーとして扱わない
    CHECK(to_text("\x1f_FF\r") == "^__FF\\r");
    CHECK(to_text("\x1fX") == "^_X");
}

TEST_CASE("ascii_to_text does not overflow with a long key code")
{
    const ScopedMacroTriggers triggers(ScopedMacroTriggers::Kind::X11_LIKE);

    const std::string long_code(200, 'A');
    CHECK(to_text("\x1f_" + long_code + "\r") == "^__" + long_code + "\\r");
    CHECK(to_text("\x1f_" + long_code) == "^__" + long_code);
}

TEST_CASE("ascii_to_text does not read beyond the string view")
{
    const ScopedMacroTriggers triggers(ScopedMacroTriggers::Kind::X11_LIKE);

    // 「\r」をビューの外に置き、範囲外を読むとトリガーとして変換されてしまうようにする
    const std::string backing("\x1f_FFBE\r");
    CHECK(to_text(head(backing, backing.length() - 1)) == "^__FFBE");
}

TEST_CASE("ascii_to_text truncates without splitting a notation")
{
    const ScopedMacroTriggers triggers(ScopedMacroTriggers::Kind::X11_LIKE);

    CHECK(to_text("\x1b\x1b\x1b", 5) == "\\e\\e");
    CHECK(to_text("\x1b\x1b\x1b", 4) == "\\e");
    CHECK(to_text("\xff", 4) == "");
    CHECK(to_text("a\x1fS_SFFBE\r", 12) == "a");
    CHECK(to_text("a\x1fS_SFFBE\r", 13) == "a\\[shift-F1]");

    // バッファの大きさが0の場合は何も書き込まない
    char untouched = 'X';
    ascii_to_text(&untouched, "abc", 0);
    CHECK(untouched == 'X');
}
