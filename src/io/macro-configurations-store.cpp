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

bool streq_case_insensitive(std::string_view a, std::string_view b)
{
    const auto length = std::min(a.length(), b.length());
    for (size_t i = 0; i < length; i++) {
        const auto a_up = std::toupper(a.at(i));
        const auto b_up = std::toupper(b.at(i));
        if (a_up != b_up) {
            return false;
        }
    }

    return true;
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

bool trigger_ascii_to_text(char **bufptr, concptr *strptr)
{
    char *s = *bufptr;
    concptr str = *strptr;
    char key_code[100]{};
    if (!macro_template) {
        return false;
    }

    *s++ = '\\';
    *s++ = '[';

    concptr tmp;
    for (auto i = 0; (*macro_template)[i] != '\0'; i++) {
        const auto ch = (*macro_template)[i];
        switch (ch) {
        case '&':
            while ((tmp = angband_strchr(macro_modifier_chr->data(), *str)) != 0) {
                const auto j = tmp - macro_modifier_chr->data();
                tmp = macro_modifier_names[j].data();
                while (*tmp) {
                    *s++ = *tmp++;
                }
                str++;
            }

            break;
        case '#': {
            int j;
            for (j = 0; *str && *str != '\r'; j++) {
                key_code[j] = *str++;
            }
            key_code[j] = '\0';
            break;
        }
        default:
            if (ch != *str) {
                return false;
            }
            str++;
        }
    }

    if (*str++ != '\r') {
        return false;
    }

    size_t i = 0;
    for (; i < max_macrotrigger; i++) {
        auto is_string_same = streq_case_insensitive(key_code, macro_trigger_keycodes.at(ShiftStatus::OFF).at(i));
        is_string_same |= streq_case_insensitive(key_code, macro_trigger_keycodes.at(ShiftStatus::ON).at(i));
        if (is_string_same) {
            break;
        }
    }

    if (i == max_macrotrigger) {
        return false;
    }

    tmp = macro_trigger_names[i].data();
    while (*tmp) {
        *s++ = *tmp++;
    }

    *s++ = ']';

    *bufptr = s;
    *strptr = str;
    return true;
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

/*
 * Hack -- convert a string into a printable form
 */
void ascii_to_text(char *buf, std::string_view sv, size_t bufsize)
{
    char *s = buf;
    auto buffer_end = s + bufsize;
    auto str = sv.data();
    constexpr auto step_size = 4;
    while (*str && (s + step_size < buffer_end)) {
        uint8_t i = *str++;
        if (i == 31) {
            if (!trigger_ascii_to_text(&s, &str)) {
                *s++ = '^';
                *s++ = '_';
            }
        } else {
            if (i == ESCAPE) {
                *s++ = '\\';
                *s++ = 'e';
            } else if (i == ' ') {
                *s++ = '\\';
                *s++ = 's';
            } else if (i == '\b') {
                *s++ = '\\';
                *s++ = 'b';
            } else if (i == '\t') {
                *s++ = '\\';
                *s++ = 't';
            } else if (i == '\n') {
                *s++ = '\\';
                *s++ = 'n';
            } else if (i == '\r') {
                *s++ = '\\';
                *s++ = 'r';
            } else if (i == '^') {
                *s++ = '\\';
                *s++ = '^';
            } else if (i == '\\') {
                *s++ = '\\';
                *s++ = '\\';
            } else if (i < 32) {
                *s++ = '^';
                *s++ = i + 64;
            } else if (i < 127) {
                *s++ = i;
            } else if (i < 64) {
                *s++ = '\\';
                *s++ = '0';
                *s++ = octify(i / 8);
                *s++ = octify(i % 8);
            } else {
                *s++ = '\\';
                *s++ = 'x';
                *s++ = hexify_upper(i);
                *s++ = hexify_lower(i);
            }
        }
    }

    *s = '\0';
}
