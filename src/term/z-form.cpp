/* File: z-form.c */

/*
 * Copyright (c) 1997 Ben Harrison
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.
 */

/* Purpose: Low level text formatting -BEN- */

#include "term/z-form.h"
#include "term/z-util.h"
#include "term/z-virt.h"
#include <span>
#include <vector>

/*
 * Here is some information about the routines in this file.
 *
 * In general, the following routines take a "buffer", a "max length",
 * a "format string", and some "arguments", and use the format string
 * and the arguments to create a (terminated) string in the buffer
 * (using only the first "max length" bytes), and return the "length"
 * of the resulting string, not including the (mandatory) terminator.
 *
 * The format strings allow the basic "sprintf()" format sequences, though
 * some of them are processed slightly more carefully or portably.
 *
 * Note that some "limitations" are enforced by the current implementation,
 * for example, no "format sequence" can exceed 100 characters, including any
 * "length" restrictions, and the result of combining and "format sequence"
 * with the relevent "arguments" must not exceed 1000 characters.
 *
 * These limitations could be fixed by stealing some of the code from,
 * say, "vsprintf()" and placing it into my "vstrnfmt()" function.
 *
 * Note that a "^" inside a "format sequence" causes the first non-space
 * character in the string resulting from the combination of the format
 * sequence and the argument(s) to be "capitalized" if possible.  Note
 * that the "^" character is removed before the "standard" formatting
 * routines are called.  Likewise, a "*" inside a "format sequence" is
 * removed from the "format sequence", and replaced by the textual form
 * of the next argument in the argument list.  See examples below.
 *
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
 * Format("%s", concptr s)
 *   Append the string "s".
 *   Do not use the "+" or "0" flags.
 *   Note that a "nullptr" value of "s" is converted to the empty string.
 *
 *
 * For examples below, assume "int n = 0; int m = 100; char buf[100];",
 * plus "char *s = nullptr;", and unknown values "char *txt; int i;".
 *
 * For example: "n = strnfmt(buf, -1, "(Max %d)", i);" will have a
 * similar effect as "sprintf(buf, "(Max %d)", i); n = strlen(buf);".
 *
 * For example: "(void)strnfmt(buf, 16, "%s", txt);" will have a similar
 * effect as "strncpy(buf, txt, 16); buf[15] = '\0';".
 *
 * For example: "if (strnfmt(buf, 16, "%s", txt) < 16) ..." will have
 * a similar effect as "strcpy(buf, txt)" but with bounds checking.
 *
 * For example: "s = buf; s += vstrnfmt(s, -1, ...); ..." will allow
 * multiple "appends" to "buf" (at the cost of losing the max-length info).
 *
 * For example: "s = buf; n = vstrnfmt(s+n, 100-n, ...); ..." will allow
 * multiple bounded "appends" to "buf", with constant access to "strlen(buf)".
 *
 * For example: "format("%^-.*s", i, txt)" will produce a string containing
 * the first "i" characters of "txt", left justified, with the first non-space
 * character capitilized, if reasonable.
 */

/*
 * Basic "vararg" format function.
 *
 * This function takes a buffer, a max byte count, a format string, and
 * a va_list of arguments to the format string, and uses the format string
 * and the arguments to create a string to the buffer.  The string is
 * derived from the format string and the arguments in the manner of the
 * "sprintf()" function, but with some extra "format" commands.  Note that
 * this function will never use more than the given number of bytes in the
 * buffer, preventing messy invalid memory references.  This function then
 * returns the total number of non-null bytes written into the buffer.
 *
 * Method: Let "str" be the (unlimited) created string, and let "len" be the
 * smaller of "max-1" and "strlen(str)".  We copy the first "len" chars of
 * "str" into "buf", place "\0" into buf[len], and return "len".
 *
 * In English, we do a sprintf() into "buf", a buffer with size "max",
 * and we return the resulting value of "strlen(buf)", but we allow some
 * special format commands, and we are more careful than "sprintf()".
 *
 * Typically, "max" is in fact the "size" of "buf", and thus represents
 * the "number" of chars in "buf" which are ALLOWED to be used.  An
 * alternative definition would have required "buf" to hold at least
 * "max+1" characters, and would have used that extra character only
 * in the case where "buf" was too short for the result.  This would
 * give an easy test for "overflow", but a less "obvious" semantics.
 *
 * Note that if the buffer was "too short" to hold the result, we will
 * always return "max-1", but we also return "max-1" if the buffer was
 * "just long enough".  We could have returned "max" if the buffer was
 * too short, not written a null, and forced the programmer to deal with
 * this special case, but I felt that it is better to at least give a
 * "usable" result when the buffer was too long instead of either giving
 * a memory overwrite like "sprintf()" or a non-terminted string like
 * "strncpy()".  Note that "strncpy()" also "null-pads" the result.
 *
 * Note that in most cases "just long enough" is probably "too short".
 *
 * We should also consider extracting and processing the "width" and other
 * "flags" by hand, it might be more "accurate", and it would allow us to
 * remove the limit (1000 chars) on the result of format sequences.
 *
 * Also, some sequences, such as "%+d" by hand, do not work on all machines,
 * and could thus be correctly handled here.
 *
 * Error detection in this routine is not very graceful, in particular,
 * if an error is detected in the format string, we simply "pre-terminate"
 * the given buffer to a length of zero, and return a "length" of zero.
 * The contents of "buf", except for "buf[0]", may then be undefined.
 */
uint vstrnfmt(char *buf, uint max, concptr fmt, va_list vp)
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
        case 'i': {
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
        }

        /* Unsigned Integers -- various formats */
        case 'u':
        case 'o':
        case 'x':
        case 'X': {
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
        }

        /* Floating Point -- various formats */
        case 'f':
        case 'F':
        case 'e':
        case 'E':
        case 'g':
        case 'G': {
            if (do_long_double) {
                auto arg = va_arg(vp, long double);
                snprintf(tmp, sizeof(tmp), aux.data(), arg);
            } else {
                auto arg = va_arg(vp, double);
                snprintf(tmp, sizeof(tmp), aux.data(), arg);
            }
            break;
        }

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

std::string vformat(concptr fmt, va_list vp)
{
    /* Initial allocation */
    std::vector<char> format_buf(1024);

    /* Keep going until successful */
    while (true) {
        uint len;

        /* Build the string */
        len = vstrnfmt(format_buf.data(), format_buf.size(), fmt, vp);

        /* Success */
        if (len < format_buf.size() - 1) {
            break;
        }

        /* Grow the buffer */
        format_buf.resize(format_buf.size() * 2);
    }

    /* Return the new string */
    return std::string(format_buf.data());
}

/*
 * Do a vstrnfmt (see above) into a buffer of a given size.
 */
uint strnfmt(char *buf, uint max, concptr fmt, ...)
{
    uint len;

    va_list vp;

    /* Begin the Varargs Stuff */
    va_start(vp, fmt);

    /* Do a virtual fprintf to stderr */
    len = vstrnfmt(buf, max, fmt, vp);

    /* End the Varargs Stuff */
    va_end(vp);

    /* Return the number of bytes written */
    return len;
}

/*
 * Do a vstrnfmt() into (see above) into a (growable) static buffer.
 * This buffer is usable for very short term formatting of results.
 * Note that the buffer is (technically) writable, but only up to
 * the length of the string contained inside it.
 */
std::string format(concptr fmt, ...)
{
    va_list vp;

    /* Begin the Varargs Stuff */
    va_start(vp, fmt);

    /* Format the args */
    auto res = vformat(fmt, vp);

    /* End the Varargs Stuff */
    va_end(vp);

    /* Return the result */
    return res;
}

/*
 * Vararg interface to plog()
 */
void plog_fmt(concptr fmt, ...)
{
    va_list vp;

    /* Begin the Varargs Stuff */
    va_start(vp, fmt);

    /* Format the args */
    auto res = vformat(fmt, vp);

    /* End the Varargs Stuff */
    va_end(vp);

    /* Call plog */
    plog(res.data());
}

/*
 * Vararg interface to quit()
 */
void quit_fmt(concptr fmt, ...)
{
    va_list vp;

    /* Begin the Varargs Stuff */
    va_start(vp, fmt);

    /* Format */
    auto res = vformat(fmt, vp);

    /* End the Varargs Stuff */
    va_end(vp);

    /* Call quit() */
    quit(res.data());
}

/*
 * Vararg interface to core()
 */
void core_fmt(concptr fmt, ...)
{
    va_list vp;

    /* Begin the Varargs Stuff */
    va_start(vp, fmt);

    /* If requested, Do a virtual fprintf to stderr */
    auto res = vformat(fmt, vp);

    /* End the Varargs Stuff */
    va_end(vp);

    /* Call core() */
    core(res.data());
}
