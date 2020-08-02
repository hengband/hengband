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
