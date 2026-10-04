#include "wizard/spoiler-util.h"
#include "system/item/item-entity.h"
#include <algorithm>
#include <fmt/format.h>
#include <fstream>
#include <string>
#include <tl/optional.hpp>

const char item_separator = ',';
const char list_separator = _(',', ';');
const int max_evolution_depth = 64;
const std::string spoiler_indent = "    ";

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
static void spoiler_out_n_chars(int n, char c, std::ofstream &ofs)
{
    for (auto i = 0; i < n; i++) {
        ofs << c;
    }
}

/*!
 * @brief ファイルポインタ先に改行を複数出力する /
 * Write out `n' blank lines to the spoiler file
 * @param n 改行を出力する数
 */
void spoiler_blanklines(int n, std::ofstream &ofs)
{
    spoiler_out_n_chars(n, '\n', ofs);
}

/*!
 * @brief ファイルポインタ先に複数のハイフンで装飾した文字列を出力する /
 * Write a line to the spoiler file and then "underline" it with hypens
 * @param str 出力したい文字列
 */
void spoiler_underline(std::string_view str, std::ofstream &ofs)
{
    ofs << str << '\n';
    spoiler_out_n_chars(str.length(), '-', ofs);
    ofs << '\n';
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
    static std::string line; //!< 書き出し待ちの行
    static auto break_pos = std::string::npos; //!< 行の中で折り返せる位置
    static tl::optional<std::string> waiting_line; //!< 後ろに空白しか続かないため、書き出しを保留している行

#ifdef JP
    bool iskanji_flag = false;
#endif

    if (flush_buffer) {
        if (waiting_line) {
            fmt::print(spoiler_file, "{}", *waiting_line);
            waiting_line.reset();
        }

        // 行が空か、空白だけなら空の行として扱う
        const auto last_pos = line.find_last_not_of(' ');
        if (last_pos == std::string::npos) {
            fmt::print(spoiler_file, "\n");
        } else {
            fmt::print(spoiler_file, "{}\n\n", std::string_view(line).substr(0, last_pos + 1));
        }

        line.clear();
        break_pos = std::string::npos;
        return;
    }

    for (size_t i = 0; i < sv.length(); ++i) {
        char ch = sv[i];
#ifdef JP
        const auto k_flag = iskanji(static_cast<unsigned char>(ch));
#endif
        bool wrap = (ch == '\n');
        bool defer = false; //!< 折り返した行の後ろに空白しか続かず、書き出しを保留するか

#ifdef JP
        if (!isprint(static_cast<unsigned char>(ch)) && !k_flag && !iskanji_flag) {
            ch = ' ';
        }

        iskanji_flag = k_flag && !iskanji_flag;
#else
        if (!isprint(ch)) {
            ch = ' ';
        }
#endif

        if (waiting_line) {
            fmt::print(spoiler_file, "{}", *waiting_line);
            if (!wrap) {
                fmt::print(spoiler_file, "\n");
            }

            waiting_line.reset();
        }

        if (!wrap) {
#ifdef JP
            if (line.length() >= (iskanji_flag ? 74U : 75U)) {
                wrap = true;
            } else if ((ch == ' ') && (line.length() >= (iskanji_flag ? 72U : 73U))) {
                wrap = true;
            }
#else
            if (line.length() >= 75U) {
                wrap = true;
            } else if ((ch == ' ') && (line.length() >= 73U)) {
                wrap = true;
            }
#endif

            if (wrap) {
#ifdef JP
                bool k_flag_local;
                bool iskanji_flag_local = false;
                auto tail = std::min(i + (iskanji_flag ? 2 : 1), sv.length());
#else
                auto tail = i + 1;
#endif

                for (; tail < sv.length(); tail++) {
                    if (sv[tail] == ' ') {
                        continue;
                    }

#ifdef JP
                    k_flag_local = iskanji(static_cast<unsigned char>(sv[tail]));
                    if (isprint(static_cast<unsigned char>(sv[tail])) || k_flag_local || iskanji_flag_local) {
                        break;
                    }

                    iskanji_flag_local = k_flag_local && !iskanji_flag_local;
#else
                    if (isprint(sv[tail])) {
                        break;
                    }
#endif
                }

                defer = (tail >= sv.length());
            }
        }

        if (wrap) {
            // 折り返せる位置があれば、その位置で切って後ろを次の行へ送る (今の文字が空白なら行全体を書き出す)。
            // 折り返せる位置の文字は、空白なら捨て、空白でなければ (「(」や2バイト文字の前半バイト) 次の行の先頭に置く
            std::string next_line;
            if ((break_pos != std::string::npos) && (ch != ' ')) {
                next_line = line.substr(break_pos + ((line[break_pos] == ' ') ? 1 : 0));
                line.erase(break_pos);
            }

            if (!defer) {
                fmt::print(spoiler_file, "{}\n", line);
            } else {
                waiting_line = std::move(line);
            }

            break_pos = std::string::npos;
            line = std::move(next_line);
        }

        if (line.empty() && (ch == ' ')) {
            continue;
        }

#ifdef JP
        if (!k_flag) {
            if ((ch == ' ') || (ch == '(')) {
                break_pos = line.length();
            }
        } else {
            const auto rest = sv.substr(i);
            if (iskanji_flag && !rest.starts_with("。") && !rest.starts_with("、") && !rest.starts_with("ィ") && !rest.starts_with("ー")) {
                break_pos = line.length();
            }
        }
#else
        if (ch == ' ') {
            break_pos = line.length();
        }
#endif

        line.push_back(ch);
    }
}

void ParameterValueInfo::analyze(const ItemEntity &item)
{
    if (item.pval == 0) {
        return;
    }

    const auto flags = item.get_flags();
    this->pval_desc = format("%+d", item.pval);
    if (flags.has_all_of(EnumRangeInclusive(TR_STR, TR_CHR))) {
        this->pval_affects.push_back(_("全能力", "All stats"));
    } else if (flags.has_any_of(EnumRangeInclusive(TR_STR, TR_CHR))) {
        const auto descriptions_stat = extract_spoiler_flags(flags, stat_flags_desc);
        this->pval_affects.insert(this->pval_affects.end(), descriptions_stat.begin(), descriptions_stat.end());
    }

    const auto descriptions_pval1 = extract_spoiler_flags(flags, pval_flags1_desc);
    this->pval_affects.insert(this->pval_affects.end(), descriptions_pval1.begin(), descriptions_pval1.end());
}
