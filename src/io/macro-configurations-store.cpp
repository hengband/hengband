/*
 * @brief マクロ設定実装
 * @author Hourier
 * @date 2024/02/19
 */

#include "io/macro-configurations-store.h"
#include "system/angband.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include <cctype>
#include <cstdint>
#include <utility>

const char *keymap_act[KEYMAP_MODES][256];
size_t max_macrotrigger = 0;
std::optional<std::string> macro_template;
std::optional<std::string> macro_modifier_chr;
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

void trigger_text_to_ascii(char **bufptr, concptr *strptr)
{
    char *s = *bufptr;
    concptr str = *strptr;
    bool mod_status[MAX_MACRO_MOD]{};

    if (!macro_template) {
        return;
    }

    for (auto i = 0; (*macro_modifier_chr)[i] != '\0'; i++) {
        mod_status[i] = false;
    }
    str++;

    /* Examine modifier keys */
    auto shiftstatus = ShiftStatus::OFF;
    while (true) {
        auto i = 0;
        size_t len = 0;
        for (; (*macro_modifier_chr)[i] != '\0'; i++) {
            len = macro_modifier_names[i].length();
            if (streq_case_insensitive(str, macro_modifier_names[i])) {
                break;
            }
        }

        if ((*macro_modifier_chr)[i] == '\0') {
            break;
        }
        str += len;
        mod_status[i] = true;
        if ('S' == (*macro_modifier_chr)[i]) {
            shiftstatus = ShiftStatus::ON;
        }
    }

    size_t len = 0;
    size_t i = 0;
    for (; i < max_macrotrigger; i++) {
        len = macro_trigger_names[i].length();
        if (streq_case_insensitive(str, macro_trigger_names[i]) && str[len] == ']') {
            break;
        }
    }

    if (i == max_macrotrigger) {
        str = angband_strchr(str, ']');
        if (str) {
            *s++ = (char)31;
            *s++ = '\r';
            *bufptr = s;
            *strptr = str; /* where **strptr == ']' */
        }

        return;
    }

    const auto &key_code = macro_trigger_keycodes.at(shiftstatus).at(i);
    str += len;

    *s++ = (char)31;
    for (i = 0; (*macro_template)[i]; i++) {
        const auto ch = (*macro_template)[i];
        switch (ch) {
        case '&':
            for (auto j = 0; (*macro_modifier_chr)[j] != '\0'; j++) {
                if (mod_status[j]) {
                    *s++ = (*macro_modifier_chr)[j];
                }
            }

            break;
        case '#':
            strcpy(s, key_code.data());
            s += key_code.length();
            break;
        default:
            *s++ = ch;
            break;
        }
    }

    *s++ = '\r';

    *bufptr = s;
    *strptr = str; /* where **strptr == ']' */
    return;
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
}

/*
 * Hack -- convert a printable string into real ascii
 *
 * I have no clue if this function correctly handles, for example,
 * parsing "\xFF" into a (signed) char.  Whoever thought of making
 * the "sign" of a "char" undefined is a complete moron.  Oh well.
 */
void text_to_ascii(char *buf, std::string_view sv, size_t bufsize)
{
    char *s = buf;
    auto buffer_end = s + bufsize;
    auto str = sv.data();
    constexpr auto step_size = 1;
    while (*str && (s + step_size < buffer_end)) {
        if (*str == '\\') {
            str++;
            if (!(*str)) {
                break;
            }

            switch (*str) {
            case '[':
                trigger_text_to_ascii(&s, &str);
                break;
            case 'x':
                *s = 16 * dehex(*++str);
                *s++ += dehex(*++str);
                break;
            case '\\':
                *s++ = '\\';
                break;
            case '^':
                *s++ = '^';
                break;
            case 's':
                *s++ = ' ';
                break;
            case 'e':
                *s++ = ESCAPE;
                break;
            case 'b':
                *s++ = '\b';
                break;
            case 'n':
                *s++ = '\n';
                break;
            case 'r':
                *s++ = '\r';
                break;
            case 't':
                *s++ = '\t';
                break;
            case '0':
                *s = 8 * deoct(*++str);
                *s++ += deoct(*++str);
                break;
            case '1':
                *s = 64 + 8 * deoct(*++str);
                *s++ += deoct(*++str);
                break;
            case '2':
                *s = 64 * 2 + 8 * deoct(*++str);
                *s++ += deoct(*++str);
                break;
            case '3':
                *s = 64 * 3 + 8 * deoct(*++str);
                *s++ += deoct(*++str);
                break;
            default:
                break;
            }

            str++;
        } else if (*str == '^') {
            str++;
            *s++ = (*str++ & 037);
        } else {
            *s++ = *str++;
        }
    }

    *s = '\0';
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
