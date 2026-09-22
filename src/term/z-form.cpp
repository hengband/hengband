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
#include <fmt/printf.h>
#include <span>
#include <string_view>
#include <vector>

namespace {
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
        auto left_align = false;
        auto seen_dot = false;
        auto width = 0;
        auto precision = -1;

        std::string aux;
        aux.reserve(128);
        aux.push_back('%');

        while (true) {
            if (*s == '\0') {
                buf[0] = '\0';
                return 0;
            }

            if (aux.length() > 100) {
                buf[0] = '\0';
                return 0;
            }

            if (isalpha(*s)) {
                if (*s == 'l') {
                    aux.push_back(*s++);
                    if (*s == 'l') {
                        aux.push_back(*s++);
                        do_long_long = true;
                    } else {
                        do_long = true;
                    }
                } else if (*s == 'L') {
                    aux.push_back(*s++);
                    do_long_double = true;
                } else {
                    aux.push_back(*s++);
                    break;
                }
            } else if (*s == '*') {
                auto arg = va_arg(vp, int);
                if (seen_dot) {
                    if (arg < 0) {
                        precision = -1;
                        aux.pop_back();
                    } else {
                        precision = arg;
                        aux.append(std::to_string(arg));
                    }
                } else if (arg < 0) {
                    if (!left_align) {
                        aux.insert(1, 1, '-');
                    }

                    left_align = true;
                    width = -arg;
                    aux.append(std::to_string(width));
                } else {
                    width = arg;
                    aux.append(std::to_string(arg));
                }

                ++s;
            } else {
                const auto ch = *s;
                if (ch == '-') {
                    left_align = true;
                } else if (ch == '.') {
                    seen_dot = true;
                    precision = 0;
                } else if (isdigit(static_cast<unsigned char>(ch))) {
                    const auto digit = ch - '0';
                    if (seen_dot) {
                        precision = precision * 10 + digit;
                    } else {
                        width = width * 10 + digit;
                    }
                }

                aux.push_back(*s++);
            }
        }

        std::string tmp;
        switch (aux.back()) {
        case 'c': {
            const auto arg = va_arg(vp, int);
            tmp = fmt::sprintf(std::string_view(aux), arg);
            break;
        }
        case 'd':
        case 'i': {
            if (do_long) {
                const auto arg = va_arg(vp, long);
                tmp = fmt::sprintf(std::string_view(aux), arg);
                break;
            }

            if (do_long_long) {
                const auto arg = va_arg(vp, long long);
                tmp = fmt::sprintf(std::string_view(aux), arg);
                break;
            }

            const auto arg = va_arg(vp, int);
            tmp = fmt::sprintf(std::string_view(aux), arg);
            break;
        }
        case 'u':
        case 'o':
        case 'x':
        case 'X': {
            if (do_long) {
                const auto arg = va_arg(vp, unsigned long);
                tmp = fmt::sprintf(std::string_view(aux), arg);
                break;
            }

            if (do_long_long) {
                const auto arg = va_arg(vp, unsigned long long);
                tmp = fmt::sprintf(std::string_view(aux), arg);
                break;
            }

            const auto arg = va_arg(vp, unsigned int);
            tmp = fmt::sprintf(std::string_view(aux), arg);
            break;
        }
        case 'f':
        case 'F':
        case 'e':
        case 'E':
        case 'g':
        case 'G': {
            if (do_long_double) {
                const auto arg = va_arg(vp, long double);
                tmp = fmt::sprintf(std::string_view(aux), arg);
                break;
            }

            const auto arg = va_arg(vp, double);
            tmp = fmt::sprintf(std::string_view(aux), arg);
            break;
        }
        case 'p': {
            const auto arg = va_arg(vp, void *);
            tmp = fmt::sprintf(std::string_view(aux), arg);
            break;
        }
        case 's': {
            if (*s == '^') {
                do_capitalize = true;
                ++s;
            }

            auto arg = va_arg(vp, const char *);
            if (arg == nullptr) {
                arg = "";
            }

            std::string_view sv(arg);
            if (precision >= 0) {
                sv = sv.substr(0, static_cast<size_t>(precision));
            }

            tmp.assign(sv);
            if (sv.length() >= static_cast<size_t>(width)) {
                break;
            }

            const auto pad = static_cast<size_t>(width) - sv.length();
            if (left_align) {
                tmp.append(pad, ' ');
            } else {
                tmp.insert(tmp.begin(), pad, ' ');
            }

            break;
        }
        default:
            buf[0] = '\0';
            return 0;
        }

        /* strlen だと %c の NUL で切れる。録画の TERM_DARK(0) を壊すので size() を使う */
        const auto formatted_str = std::span(tmp.data(), tmp.size());
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
}

std::string vformat(const char *fmt, va_list vp)
{
    std::vector<char> format_buf(1024);
    while (true) {
        // バッファが足りなければ引数を最初から読み直すので、vpを消費しないようにコピーを渡す
        va_list vp_copy;
        va_copy(vp_copy, vp);
        const auto len = vstrnfmt(format_buf.data(), format_buf.size(), fmt, vp_copy);
        va_end(vp_copy);
        if (len < format_buf.size() - 1) {
            return std::string(format_buf.data(), len);
        }

        format_buf.resize(format_buf.size() * 2);
    }
}

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
