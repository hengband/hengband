#include "wizard/spoiler-util.h"
#include "object/object-flags.h"
#include "system/item-entity.h"

const char item_separator = ',';
const char list_separator = _(',', ';');
const int max_evolution_depth = 64;
concptr spoiler_indent = "    ";

/* The spoiler file being created */
FILE *spoiler_file = nullptr;

/*!
 * @brief 特性フラグ定義から表記すべき特性を抽出する
 * @param art_flags 出力するアーティファクトの特性一覧
 * @param definitions 表記対象の特性一覧
 * @return 表記すべき特性一覧
 */
std::vector<std::string> extract_spoiler_flags(const TrFlags &art_flags, const std::vector<flag_desc> &definitions)
{
    std::vector<std::string> descriptions{};
    for (const auto &definition : definitions) {
        if (art_flags.has(definition.flag)) {
            descriptions.push_back(definition.desc);
        }
    }

    return descriptions;
}

/*!
 * @brief ファイルポインタ先に同じ文字を複数出力する /
 * Write out `n' of the character `c' to the spoiler file
 * @param n 出力する数
 * @param c 出力するキャラクタ
 */
static void spoiler_out_n_chars(int n, char c)
{
    while (--n >= 0) {
        fputc(c, spoiler_file);
    }
}

/*!
 * @brief ファイルポインタ先に改行を複数出力する /
 * Write out `n' blank lines to the spoiler file
 * @param n 改行を出力する数
 */
void spoiler_blanklines(int n)
{
    spoiler_out_n_chars(n, '\n');
}

/*!
 * @brief ファイルポインタ先に複数のハイフンで装飾した文字列を出力する /
 * Write a line to the spoiler file and then "underline" it with hypens
 * @param str 出力したい文字列
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
 * @param sv 文字列
 * @param flush_buffer trueならバッファの内容をフラッシュし、改行を書き込む。strは無視される。
 */
void spoil_out(std::string_view sv, bool flush_buffer)
{
    concptr r;
    static char roff_buf[256]{};
    static char roff_waiting_buf[256]{};

#ifdef JP
    bool iskanji_flag = false;
#endif

    static char *roff_p = roff_buf;
    static char *roff_s = nullptr;
    static bool waiting_output = false;
    if (flush_buffer) {
        if (waiting_output) {
            fputs(roff_waiting_buf, spoiler_file);
            waiting_output = false;
        }

        if (roff_p != roff_buf) {
            roff_p--;
        }
        while (*roff_p == ' ' && roff_p != roff_buf) {
            roff_p--;
        }

        if (roff_p == roff_buf) {
            fprintf(spoiler_file, "\n");
        } else {
            *(roff_p + 1) = '\0';
            fprintf(spoiler_file, "%s\n\n", roff_buf);
        }

        roff_p = roff_buf;
        roff_s = nullptr;
        roff_buf[0] = '\0';
        return;
    }

    for (auto str = sv.data(); *str != '\0'; ++str) {
#ifdef JP
        char cbak;
        bool k_flag = iskanji((unsigned char)(*str));
#endif
        char ch = *str;
        bool wrap = (ch == '\n');

#ifdef JP
        if (!isprint((unsigned char)ch) && !k_flag && !iskanji_flag) {
            ch = ' ';
        }

        iskanji_flag = k_flag && !iskanji_flag;
#else
        if (!isprint(ch)) {
            ch = ' ';
        }
#endif

        if (waiting_output) {
            fputs(roff_waiting_buf, spoiler_file);
            if (!wrap) {
                fputc('\n', spoiler_file);
            }

            waiting_output = false;
        }

        if (!wrap) {
#ifdef JP
            if (roff_p >= roff_buf + (iskanji_flag ? 74 : 75)) {
                wrap = true;
            } else if ((ch == ' ') && (roff_p >= roff_buf + (iskanji_flag ? 72 : 73))) {
                wrap = true;
            }
#else
            if (roff_p >= roff_buf + 75) {
                wrap = true;
            } else if ((ch == ' ') && (roff_p >= roff_buf + 73)) {
                wrap = true;
            }
#endif

            if (wrap) {
#ifdef JP
                bool k_flag_local;
                bool iskanji_flag_local = false;
                concptr tail = str + (iskanji_flag ? 2 : 1);
#else
                concptr tail = str + 1;
#endif

                for (; *tail; tail++) {
                    if (*tail == ' ') {
                        continue;
                    }

#ifdef JP
                    k_flag_local = iskanji((unsigned char)(*tail));
                    if (isprint((unsigned char)*tail) || k_flag_local || iskanji_flag_local) {
                        break;
                    }

                    iskanji_flag_local = k_flag_local && !iskanji_flag_local;
#else
                    if (isprint(*tail)) {
                        break;
                    }
#endif
                }

                if (!*tail) {
                    waiting_output = true;
                }
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

            if (!waiting_output) {
                fprintf(spoiler_file, "%s\n", roff_buf);
            } else {
                strcpy(roff_waiting_buf, roff_buf);
            }

            roff_s = nullptr;
            roff_p = roff_buf;
#ifdef JP
            if (cbak != ' ') {
                *roff_p++ = cbak;
            }
#endif
            while (*r) {
                *roff_p++ = *r++;
            }
        }

        if ((roff_p <= roff_buf) && (ch == ' ')) {
            continue;
        }

#ifdef JP
        if (!k_flag) {
            if ((ch == ' ') || (ch == '(')) {
                roff_s = roff_p;
            }
        } else {
            if (iskanji_flag && strncmp(str, "。", 2) != 0 && strncmp(str, "、", 2) != 0 && strncmp(str, "ィ", 2) != 0 && strncmp(str, "ー", 2) != 0) {
                roff_s = roff_p;
            }
        }
#else
        if (ch == ' ') {
            roff_s = roff_p;
        }
#endif

        *roff_p++ = ch;
    }
}

void ParameterValueInfo::analyze(const ItemEntity &item)
{
    if (item.pval == 0) {
        return;
    }

    auto flags = object_flags(&item);
    this->pval_desc = format("%+d", item.pval);
    if (flags.has_all_of(EnumRange(TR_STR, TR_CHR))) {
        this->pval_affects.push_back(_("全能力", "All stats"));
    } else if (flags.has_any_of(EnumRange(TR_STR, TR_CHR))) {
        const auto descriptions_stat = extract_spoiler_flags(flags, stat_flags_desc);
        this->pval_affects.insert(this->pval_affects.end(), descriptions_stat.begin(), descriptions_stat.end());
    }

    const auto descriptions_pval1 = extract_spoiler_flags(flags, pval_flags1_desc);
    this->pval_affects.insert(this->pval_affects.end(), descriptions_pval1.begin(), descriptions_pval1.end());
}
