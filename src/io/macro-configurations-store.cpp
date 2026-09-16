/*
 * @brief マクロ設定実装
 * @author Hourier
 * @date 2024/02/19
 */

#include "io/macro-configurations-store.h"
#include "system/angband.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <utility>

std::map<KeymapMode, std::vector<tl::optional<std::string>>> keymap_actions_map = {
    { KeymapMode::ORIGINAL, std::vector<tl::optional<std::string>>(256) },
    { KeymapMode::ROGUE, std::vector<tl::optional<std::string>>(256) },
};
size_t max_macrotrigger = 0;
tl::optional<std::string> macro_template;
tl::optional<std::string> macro_modifier_chr;
std::vector<std::string> macro_modifier_names = std::vector<std::string>(MAX_MACRO_MOD);
std::vector<std::string> macro_trigger_names = std::vector<std::string>(MAX_MACRO_TRIG);
std::map<ShiftStatus, std::vector<std::string>> macro_trigger_keycodes = {
    { ShiftStatus::OFF, std::vector<std::string>(MAX_MACRO_TRIG) },
    { ShiftStatus::ON, std::vector<std::string>(MAX_MACRO_TRIG) },
};

namespace {
char deoct(char c)
{
    if (isdigit(c)) {
        return static_cast<char>(D2I(c));
    }

    return '\0';
}

/*
 * Convert a hexidecimal-digit into a decimal
 */
char dehex(char c)
{
    if (isdigit(c)) {
        return static_cast<char>(D2I(c));
    }

    if (islower(c)) {
        return static_cast<char>(A2I(c) + 10);
    }

    if (isupper(c)) {
        return static_cast<char>(A2I(tolower(c)) + 10);
    }

    return '\0';
}

/*!
 * @brief 大文字小文字を区別せずに、文字列が指定した文字列で始まるかを調べる
 * @param str 調べる文字列
 * @param prefix 先頭にあるか調べる文字列
 * @return str が prefix で始まるならtrue
 */
bool starts_with_case_insensitive(std::string_view str, std::string_view prefix)
{
    if (str.length() < prefix.length()) {
        return false;
    }

    const auto to_upper = [](char c) { return std::toupper(static_cast<unsigned char>(c)); };
    return std::equal(prefix.begin(), prefix.end(), str.begin(), [&to_upper](char a, char b) { return to_upper(a) == to_upper(b); });
}

/*!
 * @brief 大文字小文字を区別せずに、2つの文字列が等しいかを調べる
 * @param a 比べる文字列
 * @param b 比べる文字列
 * @return 等しいならtrue
 */
bool streq_case_insensitive(std::string_view a, std::string_view b)
{
    return (a.length() == b.length()) && starts_with_case_insensitive(a, b);
}

/*!
 * @brief マクロトリガー表記「\[修飾キー名…トリガー名]」をキーコード列に変換して追加する
 * @param result 変換結果を追加する文字列
 * @param sv 変換元の文字列
 * @param pos 変換元の文字列中の「[」の位置
 * @return 変換元の文字列で続きを読む位置
 * @details
 * マクロテンプレート (pref ファイルの T: 行) が定義されていない場合は何も追加せず、「[」の次から読ませる。
 * トリガー名が見つからない場合は「]」まで読み飛ばし、テンプレートを使わない最小限のキーコード列を追加する。
 * 「]」も無い場合は何も追加せず、「[」の次から読ませる。
 */
size_t trigger_text_to_ascii(std::string &result, std::string_view sv, size_t pos)
{
    if (!macro_template) {
        return pos + 1;
    }

    const auto &modifier_chars = *macro_modifier_chr;
    const auto num_modifiers = std::min(modifier_chars.length(), macro_modifier_names.size());
    std::vector<bool> mod_status(num_modifiers);
    auto shift_status = ShiftStatus::OFF;
    auto cur = pos + 1;
    while (true) {
        const auto rest = sv.substr(cur);
        size_t m = 0;
        for (; m < num_modifiers; m++) {
            // 空の名前は常に一致して先へ進まないため、判定しない
            const auto &name = macro_modifier_names[m];
            if (!name.empty() && starts_with_case_insensitive(rest, name)) {
                break;
            }
        }

        if (m == num_modifiers) {
            break;
        }

        cur += macro_modifier_names[m].length();
        mod_status[m] = true;
        if (modifier_chars[m] == 'S') {
            shift_status = ShiftStatus::ON;
        }
    }

    const auto rest = sv.substr(cur);
    size_t trigger = 0;
    for (; trigger < max_macrotrigger; trigger++) {
        const auto &name = macro_trigger_names[trigger];
        if (starts_with_case_insensitive(rest, name) && (rest.length() > name.length()) && (rest[name.length()] == ']')) {
            break;
        }
    }

    if (trigger == max_macrotrigger) {
        // 2バイト文字の後半バイトを「]」と誤認しないよう、angband_strchr() で探す
        const std::string rest_str(rest);
        const auto *close = angband_strchr(rest_str.data(), ']');
        if (close == nullptr) {
            return pos + 1;
        }

        result.push_back(static_cast<char>(31));
        result.push_back('\r');
        return cur + (close - rest_str.data()) + 1;
    }

    result.push_back(static_cast<char>(31));
    for (const auto ch : *macro_template) {
        switch (ch) {
        case '&':
            for (size_t j = 0; j < num_modifiers; j++) {
                if (mod_status[j]) {
                    result.push_back(modifier_chars[j]);
                }
            }

            break;
        case '#':
            result.append(macro_trigger_keycodes.at(shift_status).at(trigger));
            break;
        default:
            result.push_back(ch);
            break;
        }
    }

    result.push_back('\r');
    return cur + macro_trigger_names[trigger].length() + 1;
}

/*!
 * @brief キーコード列中のマクロトリガーを表記「\[修飾キー名…トリガー名]」に変換する
 * @param sv 変換元のキーコード列
 * @param pos 変換元のキーコード列中の、マクロトリガーの開始を表す 0x1F の次の位置
 * @return 変換した表記と、変換元のキーコード列で続きを読む位置の組。マクロトリガーとして解釈できない場合はnullopt
 */
tl::optional<std::pair<std::string, size_t>> trigger_ascii_to_text(std::string_view sv, size_t pos)
{
    if (!macro_template) {
        return tl::nullopt;
    }

    const auto &modifier_chars = *macro_modifier_chr;
    const auto num_modifiers = std::min(modifier_chars.length(), macro_modifier_names.size());
    std::string text("\\[");
    std::string_view key_code;
    auto cur = pos;
    for (const auto ch : *macro_template) {
        switch (ch) {
        case '&':
            while (cur < sv.length()) {
                const auto modifier = modifier_chars.find(sv[cur]);
                if ((modifier == std::string::npos) || (modifier >= num_modifiers)) {
                    break;
                }

                text.append(macro_modifier_names[modifier]);
                cur++;
            }

            break;
        case '#': {
            const auto end = std::min(sv.find('\r', cur), sv.length());
            key_code = sv.substr(cur, end - cur);
            cur = end;
            break;
        }
        default:
            if ((cur >= sv.length()) || (sv[cur] != ch)) {
                return tl::nullopt;
            }

            cur++;
            break;
        }
    }

    if ((cur >= sv.length()) || (sv[cur] != '\r')) {
        return tl::nullopt;
    }

    for (size_t trigger = 0; trigger < max_macrotrigger; trigger++) {
        const auto matches_off = streq_case_insensitive(key_code, macro_trigger_keycodes.at(ShiftStatus::OFF).at(trigger));
        const auto matches_on = streq_case_insensitive(key_code, macro_trigger_keycodes.at(ShiftStatus::ON).at(trigger));
        if (matches_off || matches_on) {
            text.append(macro_trigger_names[trigger]).push_back(']');
            return std::make_pair(std::move(text), cur + 1);
        }
    }

    return tl::nullopt;
}

/*!
 * @brief キーコード1バイトを表記に変換する
 * @param ch 変換するキーコード
 * @return 表記
 */
std::string char_to_text(uint8_t ch)
{
    switch (ch) {
    case ESCAPE:
        return "\\e";
    case ' ':
        return "\\s";
    case '\b':
        return "\\b";
    case '\t':
        return "\\t";
    case '\n':
        return "\\n";
    case '\r':
        return "\\r";
    case '^':
        return "\\^";
    case '\\':
        return "\\\\";
    default:
        break;
    }

    if (ch < 32) {
        return { '^', static_cast<char>(ch + 64) };
    }

    if (ch < 127) {
        return std::string(1, static_cast<char>(ch));
    }

    return { '\\', 'x', hexify_upper(ch), hexify_lower(ch) };
}

/*!
 * @brief エスケープ表記 (「\」で始まる表記) をキーコードに変換して追加する
 * @param result 変換結果を追加する文字列
 * @param sv 変換元の文字列
 * @param pos 変換元の文字列中の「\」の位置
 * @return 変換元の文字列で続きを読む位置。表記に必要な文字が足りない場合はnullopt
 */
tl::optional<size_t> escape_text_to_ascii(std::string &result, std::string_view sv, size_t pos)
{
    if (pos + 1 >= sv.length()) {
        return tl::nullopt;
    }

    const auto ch = sv[pos + 1];
    switch (ch) {
    case '[':
        return trigger_text_to_ascii(result, sv, pos + 1);
    case 'x':
        // 「\xNN」は続く2文字を16進数として読む
        if (pos + 3 >= sv.length()) {
            return tl::nullopt;
        }

        result.push_back(static_cast<char>(16 * dehex(sv[pos + 2]) + dehex(sv[pos + 3])));
        return pos + 4;
    case '0':
    case '1':
    case '2':
    case '3':
        // 「\0NN」～「\3NN」は続く2文字と合わせて8進数として読む
        if (pos + 3 >= sv.length()) {
            return tl::nullopt;
        }

        result.push_back(static_cast<char>(64 * D2I(ch) + 8 * deoct(sv[pos + 2]) + deoct(sv[pos + 3])));
        return pos + 4;
    case '\\':
        result.push_back('\\');
        break;
    case '^':
        result.push_back('^');
        break;
    case 's':
        result.push_back(' ');
        break;
    case 'e':
        result.push_back(ESCAPE);
        break;
    case 'b':
        result.push_back('\b');
        break;
    case 'n':
        result.push_back('\n');
        break;
    case 'r':
        result.push_back('\r');
        break;
    case 't':
        result.push_back('\t');
        break;
    default:
        break;
    }

    return pos + 2;
}
}

/*!
 * @brief マクロ表記の文字列をキーコード列に変換する
 * @param buf 変換結果を書き込むバッファ
 * @param sv 変換元の文字列 (「^X」「\e」「\[shift-F1]」等のマクロ表記を使用できる)
 * @param bufsize buf の大きさ。変換結果は最大 bufsize - 1 バイトに切り詰め、必ずNUL終端する
 * @details
 * 変換元の文字列は、長さの範囲内かつ最初のNULまでを変換する。
 * 「^」「\x」等の表記が文字列の末尾で途切れている場合、その表記は捨ててそこで変換を終える。
 */
void text_to_ascii(char *buf, std::string_view sv, size_t bufsize)
{
    if (bufsize == 0) {
        return;
    }

    sv = sv.substr(0, sv.find('\0'));
    std::string result;
    for (size_t pos = 0; (pos < sv.length()) && (result.length() < bufsize - 1);) {
        const auto ch = sv[pos];
        if (ch == '\\') {
            const auto next = escape_text_to_ascii(result, sv, pos);
            if (!next) {
                break;
            }

            pos = *next;
            continue;
        }

        if (ch == '^') {
            if (pos + 1 >= sv.length()) {
                break;
            }

            result.push_back(sv[pos + 1] & 037);
            pos += 2;
            continue;
        }

        result.push_back(ch);
        pos++;
    }

    const auto length = std::min(result.length(), bufsize - 1);
    std::copy_n(result.data(), length, buf);
    buf[length] = '\0';
}

/*!
 * @brief キーコード列をマクロ表記の文字列に変換する
 * @param buf 変換結果を書き込むバッファ
 * @param sv 変換元のキーコード列
 * @param bufsize buf の大きさ。変換結果は最大 bufsize - 1 バイトに切り詰め、必ずNUL終端する
 * @details
 * 変換元のキーコード列は、長さの範囲内かつ最初のNULまでを変換する。
 * 1つのキーコードやマクロトリガーの表記が途中で切れないよう、収まらない表記の手前で切り詰める。
 */
void ascii_to_text(char *buf, std::string_view sv, size_t bufsize)
{
    if (bufsize == 0) {
        return;
    }

    sv = sv.substr(0, sv.find('\0'));
    std::string result;
    for (size_t pos = 0; pos < sv.length();) {
        const auto ch = static_cast<uint8_t>(sv[pos]);
        std::string text;
        auto next = pos + 1;
        if (const auto trigger = (ch == 31) ? trigger_ascii_to_text(sv, pos + 1) : tl::nullopt; trigger) {
            text = trigger->first;
            next = trigger->second;
        } else {
            text = char_to_text(ch);
        }

        if (result.length() + text.length() > bufsize - 1) {
            break;
        }

        result.append(text);
        pos = next;
    }

    std::copy(result.begin(), result.end(), buf);
    buf[result.length()] = '\0';
}
