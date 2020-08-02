#include "wizard/spoiler-util.h"

const char item_separator = ',';
const char list_separator = _(',', ';');
const int max_evolution_depth = 64;
concptr spoiler_indent = "    ";

/* The spoiler file being created */
FILE *spoiler_file = NULL;

/*!
 * @brief ファイルポインタ先に同じ文字を複数出力する /
 * Write out `n' of the character `c' to the spoiler file
 * @param n 出力する数
 * @param c 出力するキャラクタ
 * @return なし
 */
static void spoiler_out_n_chars(int n, char c)
{
    while (--n >= 0)
        fputc(c, spoiler_file);
}

/*!
 * @brief ファイルポインタ先に改行を複数出力する /
 * Write out `n' blank lines to the spoiler file
 * @param n 改行を出力する数
 * @return なし
 */
void spoiler_blanklines(int n) { spoiler_out_n_chars(n, '\n'); }

/*!
 * @brief ファイルポインタ先に複数のハイフンで装飾した文字列を出力する /
 * Write a line to the spoiler file and then "underline" it with hypens
 * @param str 出力したい文字列
 * @return なし
 */
void spoiler_underline(concptr str)
{
    fprintf(spoiler_file, "%s\n", str);
    spoiler_out_n_chars(strlen(str), '-');
    fprintf(spoiler_file, "\n");
}

/*!
 * @brief 文字列をファイルポインタに出力する /
 * Buffer text to the given file. (-SHAWN-)
 * This is basically c_roff() from mon-desc.c with a few changes.
 * @param str 文字列参照ポインタ
 * @return なし
 */
void spoil_out(concptr str)
{
    concptr r;
    static char roff_buf[256];
    static char roff_waiting_buf[256];

#ifdef JP
    bool iskanji_flag = FALSE;
#endif

    static char *roff_p = roff_buf;
    static char *roff_s = NULL;
    static bool waiting_output = FALSE;
    if (!str) {
        if (waiting_output) {
            fputs(roff_waiting_buf, spoiler_file);
            waiting_output = FALSE;
        }

        if (roff_p != roff_buf)
            roff_p--;
        while (*roff_p == ' ' && roff_p != roff_buf)
            roff_p--;

        if (roff_p == roff_buf)
            fprintf(spoiler_file, "\n");
        else {
            *(roff_p + 1) = '\0';
            fprintf(spoiler_file, "%s\n\n", roff_buf);
        }

        roff_p = roff_buf;
        roff_s = NULL;
        roff_buf[0] = '\0';
        return;
    }

    for (; *str; str++) {
#ifdef JP
        char cbak;
        bool k_flag = iskanji((unsigned char)(*str));
#endif
        char ch = *str;
        bool wrap = (ch == '\n');

#ifdef JP
        if (!isprint((unsigned char)ch) && !k_flag && !iskanji_flag)
            ch = ' ';

        iskanji_flag = k_flag && !iskanji_flag;
#else
        if (!isprint(ch))
            ch = ' ';
#endif

        if (waiting_output) {
            fputs(roff_waiting_buf, spoiler_file);
            if (!wrap)
                fputc('\n', spoiler_file);

            waiting_output = FALSE;
        }

        if (!wrap) {
#ifdef JP
            if (roff_p >= roff_buf + (k_flag ? 74 : 75))
                wrap = TRUE;
            else if ((ch == ' ') && (roff_p >= roff_buf + (k_flag ? 72 : 73)))
                wrap = TRUE;
#else
            if (roff_p >= roff_buf + 75)
                wrap = TRUE;
            else if ((ch == ' ') && (roff_p >= roff_buf + 73))
                wrap = TRUE;
#endif

            if (wrap) {
#ifdef JP
                bool k_flag_local;
                bool iskanji_flag_local = FALSE;
                concptr tail = str + (k_flag ? 2 : 1);
#else
                concptr tail = str + 1;
#endif

                for (; *tail; tail++) {
                    if (*tail == ' ')
                        continue;

#ifdef JP
                    k_flag_local = iskanji((unsigned char)(*tail));
                    if (isprint((unsigned char)*tail) || k_flag_local || iskanji_flag_local)
                        break;

                    iskanji_flag_local = k_flag_local && !iskanji_flag_local;
#else
                    if (isprint(*tail))
                        break;
#endif
                }

                if (!*tail)
                    waiting_output = TRUE;
            }
        }

        if (wrap) {
            *roff_p = '\0';
            r = roff_p;
#ifdef JP
            cbak = ' ';
#endif
            if (roff_s && (ch != ' ')) {
#ifdef JP
                cbak = *roff_s;
#endif
                *roff_s = '\0';
                r = roff_s + 1;
            }

            if (!waiting_output)
                fprintf(spoiler_file, "%s\n", roff_buf);
            else
                strcpy(roff_waiting_buf, roff_buf);

            roff_s = NULL;
            roff_p = roff_buf;
#ifdef JP
            if (cbak != ' ')
                *roff_p++ = cbak;
#endif
            while (*r)
                *roff_p++ = *r++;
        }

        if ((roff_p <= roff_buf) && (ch == ' '))
            continue;

#ifdef JP
        if (!k_flag) {
            if ((ch == ' ') || (ch == '('))
                roff_s = roff_p;
        } else {
            if (iskanji_flag && strncmp(str, "。", 2) != 0 && strncmp(str, "、", 2) != 0 && strncmp(str, "ィ", 2) != 0 && strncmp(str, "ー", 2) != 0)
                roff_s = roff_p;
        }
#else
        if (ch == ' ')
            roff_s = roff_p;
#endif

        *roff_p++ = ch;
    }
}
