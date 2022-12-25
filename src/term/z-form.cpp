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
 * some of them are processed slightly more carefully or portably, as well
 * as a few "special" sequences, including the "%r" and "%v" sequences, and
 * the "capilitization" sequences of "%C", "%S", and "%V".
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
 * Legal format characters: %,n,p,c,s,d,i,o,u,X,x,E,e,F,f,G,g,r,v.
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
 * Format("%ld", long int i)
 *   Append the long integer "i".
 *
 * Format("%Ld", long long int i)
 *   Append the long long integer "i".
 *
 * Format("%d", int i)
 *   Append the integer "i".
 *
 * Format("%lu", unsigned long int i)
 *   Append the unsigned long integer "i".
 *
 * Format("%Lu", unsigned long long int i)
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
 * Format("%V", vptr v)
 *   Note -- possibly significant mode flag
 * Format("%v", vptr v)
 *   Append the object "v", using the current "user defined print routine".
 *   User specified modifiers, often ignored.
 *
 * Format("%r", vstrnfmt_aux_func *fp)
 *   Set the "user defined print routine" (vstrnfmt_aux) to "fp".
 *   No legal modifiers.
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
 * For example: "format("The %r%v was destroyed!", obj_desc, obj);"
 * (where "obj_desc(buf, max, fmt, obj)" will "append" a "description"
 * of the given object to the given buffer, and return the total length)
 * will return a "useful message" about the object "obj", for example,
 * "The Large Shield was destroyed!".
 *
 * For example: "format("%^-.*s", i, txt)" will produce a string containing
 * the first "i" characters of "txt", left justified, with the first non-space
 * character capitilized, if reasonable.
 */

/*
 * The "type" of the "user defined print routine" pointer
 */
typedef uint (*vstrnfmt_aux_func)(char *buf, uint max, concptr fmt, vptr arg);

/*
 * The "default" user defined print routine.  Ignore the "fmt" string.
 */
static uint vstrnfmt_aux_dflt(char *buf, uint max, concptr fmt, vptr arg)
{
    uint len;
    char tmp[32];

    /* XXX XXX */
    fmt = fmt ? fmt : 0;

    /* Pointer display */
    snprintf(tmp, sizeof(tmp), "<<%p>>", arg);
    len = strlen(tmp);
    if (len >= max) {
        len = max - 1;
    }
    tmp[len] = '\0';
    strcpy(buf, tmp);
    return len;
}

/*
 * The "current" user defined print routine.  It can be changed
 * dynamically by sending the proper "%r" sequence to "vstrnfmt()"
 */
static vstrnfmt_aux_func vstrnfmt_aux = vstrnfmt_aux_dflt;

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
    concptr s;

    /* The argument is "long" */
    bool do_long;

    /* The argument is "long long" */
    bool do_long_long;

    /* The argument needs "processing" */
    bool do_xtra;

    /* Bytes used in buffer */
    uint n;

    /* Bytes used in format sequence */
    uint q;

    /* Format sequence */
    char aux[128];

    /* Resulting string */
    char tmp[1024];

    /* Mega-Hack -- treat "illegal" length as "infinite" */
    if (!max) {
        max = 32767;
    }

    /* Mega-Hack -- treat "no format" as "empty string" */
    if (!fmt) {
        fmt = "";
    }

    /* Begin the buffer */
    n = 0;

    /* Begin the format string */
    s = fmt;

    /* Scan the format string */
    while (true) {
        /* All done */
        if (!*s) {
            break;
        }

        /* Normal character */
        if (*s != '%') {
            /* Check total length */
            if (n == max - 1) {
                break;
            }

            /* Save the character */
            buf[n++] = *s++;
            continue;
        }

        /* Skip the "percent" */
        s++;

        /* Pre-process "%%" */
        if (*s == '%') {
            /* Check total length */
            if (n == max - 1) {
                break;
            }

            /* Save the percent */
            buf[n++] = '%';

            /* Skip the "%" */
            s++;
            continue;
        }

        /* Pre-process "%n" */
        if (*s == 'n') {
            int *arg;

            /* Access the next argument */
            arg = va_arg(vp, int *);

            /* Save the current length */
            (*arg) = n;

            /* Skip the "n" */
            s++;
            continue;
        }

        /* Hack -- Pre-process "%r" */
        if (*s == 'r') {
            /* Extract the next argument, and save it (globally) */
            vstrnfmt_aux = va_arg(vp, vstrnfmt_aux_func);

            /* Skip the "r" */
            s++;
            continue;
        }

        /* Begin the "aux" string */
        q = 0;

        /* Save the "percent" */
        aux[q++] = '%';

        /* Assume no "long" argument */
        do_long = false;

        /* Assume no "long long" argument */
        do_long_long = false;

        /* Assume no "xtra" processing */
        do_xtra = false;

        /* Build the "aux" string */
        while (true) {
            /* Error -- format sequence is not terminated */
            if (!*s) {
                /* Terminate the buffer */
                buf[0] = '\0';

                /* Return "error" */
                return 0;
            }

            /* Error -- format sequence may be too long */
            if (q > 100) {
                /* Terminate the buffer */
                buf[0] = '\0';

                /* Return "error" */
                return 0;
            }

            /* Handle "alphabetic" chars */
            if (isalpha(*s)) {
                /* Hack -- handle "long" request */
                if (*s == 'l') {
                    /* Save the character */
                    aux[q++] = *s++;

                    /* Note the "long" flag */
                    do_long = true;
                }

                /* Mega-Hack -- handle "extra-long" request */
                else if (*s == 'L') {
                    /* Save the character */
                    aux[q++] = 'l';
                    aux[q++] = 'l';
                    s++;

                    /* Note the "long long" flag */
                    do_long_long = true;
                }

                /* Handle normal end of format sequence */
                else {
                    /* Save the character */
                    aux[q++] = *s++;

                    /* Stop processing the format sequence */
                    break;
                }
            }

            /* Handle "non-alphabetic" chars */
            else {
                /* Hack -- Handle 'star' (for "variable length" argument) */
                if (*s == '*') {
                    int arg;

                    /* Access the next argument */
                    arg = va_arg(vp, int);

                    /* Hack -- append the "length" */
                    snprintf(aux + q, sizeof(aux) - q, "%d", arg);

                    /* Hack -- accept the "length" */
                    while (aux[q]) {
                        q++;
                    }

                    /* Skip the "*" */
                    s++;
                }

                /* Mega-Hack -- Handle 'caret' (for "uppercase" request) */
                else if (*s == '^') {
                    /* Note the "xtra" flag */
                    do_xtra = true;

                    /* Skip the "^" */
                    s++;
                }

                /* Collect "normal" characters (digits, "-", "+", ".", etc) */
                else {
                    /* Save the character */
                    aux[q++] = *s++;
                }
            }
        }

        /* Terminate "aux" */
        aux[q] = '\0';

        /* Clear "tmp" */
        tmp[0] = '\0';

        /* Process the "format" char */
        switch (aux[q - 1]) {
        /* Simple Character -- standard format */
        case 'c': {
            int arg;

            /* Access next argument */
            arg = va_arg(vp, int);

            /* Format the argument */
            snprintf(tmp, sizeof(tmp), "%c", arg);

            break;
        }

        /* Signed Integers -- standard format */
        case 'd':
        case 'i': {
            if (do_long) {
                long arg;

                /* Access next argument */
                arg = va_arg(vp, long);

                /* Format the argument */
                snprintf(tmp, sizeof(tmp), aux, arg);
            } else if (do_long_long) {
                long long arg;

                /* Access next argument */
                arg = va_arg(vp, long long);

                /* Format the argument */
                snprintf(tmp, sizeof(tmp), aux, arg);
            } else {
                int arg;

                /* Access next argument */
                arg = va_arg(vp, int);

                /* Format the argument */
                snprintf(tmp, sizeof(tmp), aux, arg);
            }

            break;
        }

        /* Unsigned Integers -- various formats */
        case 'u':
        case 'o':
        case 'x':
        case 'X': {
            if (do_long) {
                ulong arg;

                /* Access next argument */
                arg = va_arg(vp, ulong);

                snprintf(tmp, sizeof(tmp), aux, arg);
            } else if (do_long_long) {
                unsigned long long arg;

                /* Access next argument */
                arg = va_arg(vp, unsigned long long);

                snprintf(tmp, sizeof(tmp), aux, arg);
            } else {
                uint arg;

                /* Access next argument */
                arg = va_arg(vp, uint);
                snprintf(tmp, sizeof(tmp), aux, arg);
            }

            break;
        }

        /* Floating Point -- various formats */
        case 'f':
        case 'e':
        case 'E':
        case 'g':
        case 'G': {
            double arg;

            /* Access next argument */
            arg = va_arg(vp, double);

            /* Format the argument */
            snprintf(tmp, sizeof(tmp), aux, arg);

            break;
        }

        /* Pointer -- implementation varies */
        case 'p': {
            vptr arg;

            /* Access next argument */
            arg = va_arg(vp, vptr);

            /* Format the argument */
            snprintf(tmp, sizeof(tmp), aux, arg);

            break;
        }

        /* String */
        case 's': {
            concptr arg;
            char arg2[1024];

            /* Access next argument */
            arg = va_arg(vp, concptr);

            /* Hack -- convert nullptr to EMPTY */
            if (!arg) {
                arg = "";
            }

            /* Prevent buffer overflows */
            strncpy(arg2, arg, 1024);
            arg2[1023] = '\0';

            /* Format the argument */
            snprintf(tmp, sizeof(tmp), aux, arg);

            break;
        }

        /* User defined data */
        case 'V':
        case 'v': {
            vptr arg;

            /* Access next argument */
            arg = va_arg(vp, vptr);

            /* Format the "user data" */
            snprintf(tmp, sizeof(tmp), aux, arg);

            break;
        }

        default: {
            /* Error -- illegal format char */
            buf[0] = '\0';

            /* Return "error" */
            return 0;
        }
        }

#ifdef JP
        for (q = 0; tmp[q]; q++) {
            if (iskanji(tmp[q])) {
                do_xtra = false;
                break;
            }
        }
#endif
        /* Mega-Hack -- handle "capitilization" */
        if (do_xtra) {
            /* Now append "tmp" to "buf" */
            for (q = 0; tmp[q]; q++) {
                /* Notice first non-space */
                if (!iswspace(tmp[q])) {
                    /* Capitalize if possible */
                    if (islower(tmp[q])) {
                        tmp[q] = (char)toupper(tmp[q]);
                    }

                    break;
                }
            }
        }

        /* Now append "tmp" to "buf" */
        for (q = 0; tmp[q]; q++) {
            /* Check total length */
            if (n == max - 1) {
                break;
            }

            /* Save the character */
#ifdef JP
            if (iskanji(tmp[q])) {
                if (tmp[q + 1]) {
                    buf[n++] = tmp[q++];
                } else {
                    // 最後の文字が2バイト文字の前半で終わる場合は空白で置き換えて終了する
                    buf[n++] = ' ';
                    break;
                }
            }
#endif
            buf[n++] = tmp[q];
        }
    }

    /* Terminate buffer */
    buf[n] = '\0';

    /* Return length */
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
