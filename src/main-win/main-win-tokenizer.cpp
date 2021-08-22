/*!
 * @file main-win-tokenizer.cpp
 * @brief Windows版固有実装(トークン分割)
 */

#include "main-win/main-win-tokenizer.h"

#include <ctype.h>

/*
 * - Taken from files.c.
 *
 * Extract "tokens" from a buffer
 *
 * This function uses "whitespace" as delimiters, and treats any amount of
 * whitespace as a single delimiter.  We will never return any empty tokens.
 * When given an empty buffer, or a buffer containing only "whitespace", we
 * will return no tokens.  We will never extract more than "num" tokens.
 *
 * By running a token through the "text_to_ascii()" function, you can allow
 * that token to include (encoded) whitespace, using "\s" to encode spaces.
 *
 * We save pointers to the tokens in "tokens", and return the number found.
 */
int16_t tokenize_whitespace(char *buf, int16_t num, char **tokens)
{
    int16_t k = 0;
    char *s = buf;

    while (k < num) {
        char *t;
        for (; *s && iswspace(*s); ++s) /* loop */
            ;

        if (!*s)
            break;

        for (t = s; *t && !iswspace(*t); ++t) /* loop */
            ;

        if (*t)
            *t++ = '\0';

        tokens[k++] = s;
        s = t;
    }

    return k;
}
