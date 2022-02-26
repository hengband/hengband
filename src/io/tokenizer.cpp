#include "io/tokenizer.h"

/*!
 * @brief 各種データテキストをトークン単位に分解する / Extract the first few "tokens" from a buffer
 * @param buf データテキストの参照ポインタ
 * @param num トークンの数
 * @param tokens トークンを保管する文字列参照ポインタ配列
 * @param mode オプション
 * @return 解釈した文字列数
 * @details
 * <pre>
 * This function uses "colon" and "slash" as the delimeter characters.
 * We never extract more than "num" tokens.  The "last" token may include
 * "delimeter" characters, allowing the buffer to include a "string" token.
 * We save pointers to the tokens in "tokens", and return the number found.
 * Hack -- Attempt to handle the 'c' character formalism
 * Hack -- An empty buffer, or a final delimeter, yields an "empty" token.
 * Hack -- We will always extract at least one token
 * </pre>
 */
int16_t tokenize(char *buf, int16_t num, char **tokens, BIT_FLAGS mode)
{
    int16_t i = 0;
    char *s = buf;
    while (i < num - 1) {
        char *t;
        for (t = s; *t; t++) {
            if ((*t == ':') || (*t == '/')) {
                break;
            }

            if ((mode & TOKENIZE_CHECKQUOTE) && (*t == '\'')) {
                t++;
                if (*t == '\\') {
                    t++;
                }
                if (!*t) {
                    break;
                }

                t++;
                if (*t != '\'') {
                    *t = '\'';
                }
            }

            if (*t == '\\') {
                t++;
            }
        }

        if (!*t) {
            break;
        }

        *t++ = '\0';
        tokens[i++] = s;
        s = t;
    }

    tokens[i++] = s;
    return i;
}
