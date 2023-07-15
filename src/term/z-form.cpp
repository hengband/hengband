/*!
 * @file z-form.cpp
 * @brief Low level text formatting
 * @date 2023/04/30
 * @author Ben Harrison, 1997
 * @detail
 * Legal format characters: %,n,p,c,s,d,i,o,u,X,x,E,e,F,f,G,g.
 *
 * Format("%%")
 *   Append the literal "%".
 *   No legal modifiers.
 *
 * Format("%n", int *np)
 *   Save the current length into (*np).
 *   No legal modifiers.
 *
 * Format("%p", vptr v)
 *   Append the pointer "v" (implementation varies).
 *   No legal modifiers.
 *
 * Format("%E", double r)
 * Format("%F", double r)
 * Format("%G", double r)
 * Format("%e", double r)
 * Format("%f", double r)
 * Format("%g", double r)
 *   Append the double "r", in various formats.
 *
 * Format("%LE", long double r)
 * Format("%LF", long double r)
 * Format("%LG", long double r)
 * Format("%Le", long double r)
 * Format("%Lf", long double r)
 * Format("%Lg", long double r)
 *   Append the long double "r", in various formats.
 *
 * Format("%ld", long int i)
 *   Append the long integer "i".
 *
 * Format("%lld", long long int i)
 *   Append the long long integer "i".
 *
 * Format("%d", int i)
 *   Append the integer "i".
 *
 * Format("%lu", unsigned long int i)
 *   Append the unsigned long integer "i".
 *
 * Format("%llu", unsigned long long int i)
 *   Append the unsigned long long integer "i".
 *
 * Format("%u", unsigned int i)
 *   Append the unsigned integer "i".
 *
 * Format("%lo", unsigned long int i)
 *   Append the unsigned long integer "i", in octal.
 *
 * Format("%o", unsigned int i)
 *   Append the unsigned integer "i", in octal.
 *
 * Format("%lX", unsigned long int i)
 *   Note -- use all capital letters
 * Format("%lx", unsigned long int i)
 *   Append the unsigned long integer "i", in hexidecimal.
 *
 * Format("%X", unsigned int i)
 *   Note -- use all capital letters
 * Format("%x", unsigned int i)
 *   Append the unsigned integer "i", in hexidecimal.
 *
 * Format("%c", char c)
 *   Append the character "c".
 *   Do not use the "+" or "0" flags.
 *
 * Format("%s", const char *s)
 *   Append the string "s".
 *   Do not use the "+" or "0" flags.
 *   Note that a "nullptr" value of "s" is converted to the empty string.
 *
 */

#include "term/z-form.h"
#include "term/z-util.h"
#include "term/z-virt.h"
#include <span>
#include <vector>

namespace {
std::string vformat(const char *fmt, va_list vp)
{
    std::vector<char> format_buf(1024);
    while (true) {
        const auto len = vstrnfmt(format_buf.data(), format_buf.size(), fmt, vp);
        if (len < format_buf.size() - 1) {
            return std::string(format_buf.data());
        }

        format_buf.resize(format_buf.size() * 2);
    }
}
}

/*!
 * @brief 2バイト文字、及び文頭の大文字小文字を考慮しつつ、文字列のフォーマットを行う
 * @details 文頭を大文字にするには'%s^'とする.
 */
uint32_t vstrnfmt(char *buf, uint32_t max, const char *fmt, va_list vp)
{
    /* treat "no format" or "illegal" length as "empty string" */
    if (!fmt || (max == 0)) {
        buf[0] = '\0';
        return 0;
    }

    /* Output length to buffer */
    auto n = 0U;

    for (auto s = fmt; *s != '\0';) {
        /* Normal character */
        if (*s != '%') {
            if (n == max - 1) {
                break;
            }

            buf[n++] = *s++;
            continue;
        }

        s++;

        /* Pre-process "%%" */
        if (*s == '%') {
            if (n == max - 1) {
                break;
            }

            buf[n++] = '%';
            s++;
            continue;
        }

        /* Pre-process "%n" */
        if (*s == 'n') {
            /* Save the current length */
            auto *arg = va_arg(vp, int *);
            (*arg) = n;

            s++;
            continue;
        }

        auto do_long = false;
        auto do_long_long = false;
        auto do_long_double = false;
        auto do_capitalize = false;

        /* Format sequence */
        std::string aux;
        aux.reserve(128);
        aux.push_back('%');

        /* Build the format sequence string */
        while (true) {
            /* Error -- format sequence is not terminated */
            if (*s == '\0') {
                buf[0] = '\0';
                return 0;
            }

            /* Error -- format sequence may be too long */
            if (aux.length() > 100) {
                buf[0] = '\0';
                return 0;
            }

            if (isalpha(*s)) {
                /* handle "long" or "long long" request */
                if (*s == 'l') {
                    aux.push_back(*s++);
                    if (*s == 'l') {
                        aux.push_back(*s++);
                        do_long_long = true;
                    } else {
                        do_long = true;
                    }
                }

                /* handle "long double" request */
                else if (*s == 'L') {
                    aux.push_back(*s++);
                    do_long_double = true;
                }

                /* Handle normal end of format sequence */
                else {
                    aux.push_back(*s++);
                    break;
                }
            } else {
                /* Handle 'star' (for "variable length" argument) */
                if (*s == '*') {
                    auto arg = va_arg(vp, int);
                    aux.append(std::to_string(arg));
                    s++;
                }

                /* Collect "normal" characters (digits, "-", "+", ".", etc) */
                else {
                    aux.push_back(*s++);
                }
            }
        }

        /* Resulting string */
        char tmp[1024]{};

        /* Process the "format" char */
        switch (aux.back()) {
        /* Simple Character -- standard format */
        case 'c': {
            auto arg = va_arg(vp, int);
            snprintf(tmp, sizeof(tmp), "%c", arg);
            break;
        }

        /* Signed Integers -- standard format */
        case 'd':
        case 'i':
            if (do_long) {
                auto arg = va_arg(vp, long);
                snprintf(tmp, sizeof(tmp), aux.data(), arg);
            } else if (do_long_long) {
                auto arg = va_arg(vp, long long);
                snprintf(tmp, sizeof(tmp), aux.data(), arg);
            } else {
                auto arg = va_arg(vp, int);
                snprintf(tmp, sizeof(tmp), aux.data(), arg);
            }

            break;

        /* Unsigned Integers -- various formats */
        case 'u':
        case 'o':
        case 'x':
        case 'X':
            if (do_long) {
                auto arg = va_arg(vp, unsigned long);
                snprintf(tmp, sizeof(tmp), aux.data(), arg);
            } else if (do_long_long) {
                auto arg = va_arg(vp, unsigned long long);
                snprintf(tmp, sizeof(tmp), aux.data(), arg);
            } else {
                auto arg = va_arg(vp, unsigned int);
                snprintf(tmp, sizeof(tmp), aux.data(), arg);
            }

            break;

        /* Floating Point -- various formats */
        case 'f':
        case 'F':
        case 'e':
        case 'E':
        case 'g':
        case 'G':
            if (do_long_double) {
                auto arg = va_arg(vp, long double);
                snprintf(tmp, sizeof(tmp), aux.data(), arg);
            } else {
                auto arg = va_arg(vp, double);
                snprintf(tmp, sizeof(tmp), aux.data(), arg);
            }

            break;

        /* Pointer -- implementation varies */
        case 'p': {
            auto arg = va_arg(vp, void *);
            snprintf(tmp, sizeof(tmp), aux.data(), arg);
            break;
        }

        /* String */
        case 's': {
            if (*s == '^') {
                do_capitalize = true;
                ++s;
            }

            auto arg = va_arg(vp, const char *);
            if (arg == nullptr) {
                arg = "";
            }

            snprintf(tmp, sizeof(tmp), aux.data(), arg);
            break;
        }
        default: {
            /* Error -- illegal format char */
            buf[0] = '\0';
            return 0;
        }
        }

        std::span<char> formatted_str(std::begin(tmp), strlen(tmp));
#ifdef JP
        for (auto ch : formatted_str) {
            if (iskanji(ch)) {
                do_capitalize = false;
                break;
            }
        }
#endif
        if (do_capitalize) {
            for (auto &ch : formatted_str) {
                if (!iswspace(ch)) {
                    if (islower(ch)) {
                        ch = static_cast<char>(toupper(ch));
                    }

                    break;
                }
            }
        }

        /* Now append formatted_str to "buf" */
        for (auto it = formatted_str.begin(), it_end = formatted_str.end(); it != it_end;) {
            if (n == max - 1) {
                break;
            }
#ifdef JP
            if (iskanji(*it)) {
                if ((n < max - 2) && ((it + 1) != it_end)) {
                    buf[n++] = *it++;
                } else {
                    // 最後の文字が2バイト文字の前半で終わる場合は空白で置き換えて終了する
                    buf[n++] = ' ';
                    break;
                }
            }
#endif
            buf[n++] = *it++;
        }
    }

    buf[n] = '\0';
    return n;
}

/*
 * Do a vstrnfmt (see above) into a buffer of a given size.
 */
uint32_t strnfmt(char *buf, uint32_t max, const char *fmt, ...)
{
    va_list vp;
    va_start(vp, fmt);
    auto len = vstrnfmt(buf, max, fmt, vp);
    va_end(vp);
    return len;
}

/*
 * Do a vstrnfmt() into (see above) into a (growable) static buffer.
 * This buffer is usable for very short term formatting of results.
 * Note that the buffer is (technically) writable, but only up to
 * the length of the string contained inside it.
 */
std::string format(const char *fmt, ...)
{
    va_list vp;
    va_start(vp, fmt);
    auto res = vformat(fmt, vp);
    va_end(vp);
    return res;
}

/*
 * Vararg interface to plog()
 */
void plog_fmt(const char *fmt, ...)
{
    va_list vp;
    va_start(vp, fmt);
    auto res = vformat(fmt, vp);
    va_end(vp);
    plog(res.data());
}

/*
 * Vararg interface to quit()
 */
void quit_fmt(const char *fmt, ...)
{
    va_list vp;
    va_start(vp, fmt);
    auto res = vformat(fmt, vp);
    va_end(vp);
    quit(res.data());
}

/*
 * Vararg interface to core()
 */
void core_fmt(const char *fmt, ...)
{
    va_list vp;
    va_start(vp, fmt);
    auto res = vformat(fmt, vp);
    va_end(vp);
    core(res.data());
}
