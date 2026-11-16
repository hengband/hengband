#include "wizard/spoiler-util.h"
#include "locale/multibyte-char.h"
#include "system/item/item-entity.h"
#include "util/buffer-shaper.h"
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
    ofs << std::string(str_display_width(str), '-');
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
    constexpr size_t line_width_max = 75; //!< 1 行の表示幅の上限
    constexpr size_t space_wrap_width = 73; //!< 空白を置く前にこの表示幅に達していたら折り返す
    static std::string line; //!< 書き出し待ちの行
    static auto break_pos = std::string::npos; //!< 行の中で折り返せる位置
    static tl::optional<std::string> waiting_line; //!< 後ろに空白しか続かないため、書き出しを保留している行
#ifdef JP
    static std::string prev_char; //!< 行に最後に足した文字 (行末禁則の判定に使う)
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
#ifdef JP
        prev_char.clear();
#endif
        return;
    }

    // 全角文字と表示できる1バイトの文字のほかは空白とみなす (後続のバイトが欠けた多バイト文字の先頭バイトも空白にする)。
    // Shift_JIS の半角カナは、これまでの出力に合わせて空白にする (is_printable_single_byte_char() とは扱いが違う)
    const auto is_visible = [](std::string_view ch) { return is_multibyte_char_at(ch) || isprint(static_cast<unsigned char>(ch.front())); };
    for (auto rest = sv; !rest.empty();) {
        auto ch = mb_first_char(rest);
        rest.remove_prefix(ch.length());
        auto wrap = ch.front() == '\n';
        auto defer = false; //!< 折り返した行の後ろに空白しか続かず、書き出しを保留するか
        if (!is_visible(ch)) {
            ch = " ";
        }

        const auto is_wide = is_multibyte_char_at(ch);

        if (waiting_line) {
            fmt::print(spoiler_file, "{}", *waiting_line);
            if (!wrap) {
                fmt::print(spoiler_file, "\n");
            }

            waiting_line.reset();
        }

        const auto char_width = str_display_width(ch);
        if (!wrap) {
            const auto line_width = str_display_width(line);
            wrap = (line_width + char_width > line_width_max) || ((ch == " ") && (line_width >= space_wrap_width));
            if (wrap) {
                // 後ろに空白と表示できない文字しか続かなければ、書き出しを保留する。
                // ただし今の文字が全角文字なら保留しない (これまで1バイトずつ処理していたとき、後半バイトで保留をすぐに解いていたのと同じ)
                auto tail = rest;
                while (!tail.empty()) {
                    const auto next = mb_first_char(tail);
                    if ((next != " ") && is_visible(next)) {
                        break;
                    }

                    tail.remove_prefix(next.length());
                }

                defer = tail.empty() && !is_wide;
            }
        }

        if (wrap) {
            // 折り返せる位置があれば、その位置で切って後ろを次の行へ送る (今の文字が空白なら行全体を書き出す)。
            // 折り返せる位置の文字は、空白なら捨て、空白でなければ (「(」や全角文字) 次の行の先頭に置く
            std::string next_line;
            if ((break_pos != std::string::npos) && (ch != " ")) {
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

            // 次の行へ送った部分と今の文字が幅に収まらなければ (送った部分に折り返す位置が無い)、送った部分もそのまま書き出す
            if (str_display_width(line) + char_width > line_width_max) {
                if (waiting_line) {
                    fmt::print(spoiler_file, "{}\n", *waiting_line);
                    waiting_line.reset();
                }

                fmt::print(spoiler_file, "{}\n", line);
                line.clear();
            }
        }

        if (line.empty() && (ch == " ")) {
            continue;
        }

#ifdef JP
        // 「(」と全角文字の前で折り返してよいかは、画面の折り返しと同じ classify_line_break() で決める
        // (行頭禁則文字の前、行末禁則文字の後ろでは折り返さない)。折り返す位置が無いときに語の途中までさかのぼる処理は持たない。
        // 行の先頭は、そこで折り返すと空の行を書き出すので、折り返す位置にしない
        if (!is_wide) {
            if ((ch == " ") || ((ch == "(") && !line.empty() && (classify_line_break(prev_char, "(") != LineBreakKind::NONE))) {
                break_pos = line.length();
            }
        } else if (!line.empty() && (classify_line_break(prev_char, ch) != LineBreakKind::NONE)) {
            break_pos = line.length();
        }

        prev_char = ch;
#else
        if (ch == " ") {
            break_pos = line.length();
        }
#endif

        line.append(ch);
    }
}

void ParameterValueInfo::analyze(const ItemEntity &item)
{
    if (item.pval == 0) {
        return;
    }

    const auto flags = item.get_flags();
    this->pval_desc = fmt::format("{:+}", item.pval);
    if (flags.has_all_of(EnumRangeInclusive(TR_STR, TR_CHR))) {
        this->pval_affects.push_back(_("全能力", "All stats"));
    } else if (flags.has_any_of(EnumRangeInclusive(TR_STR, TR_CHR))) {
        const auto descriptions_stat = extract_spoiler_flags(flags, stat_flags_desc);
        this->pval_affects.insert(this->pval_affects.end(), descriptions_stat.begin(), descriptions_stat.end());
    }

    const auto descriptions_pval1 = extract_spoiler_flags(flags, pval_flags1_desc);
    this->pval_affects.insert(this->pval_affects.end(), descriptions_pval1.begin(), descriptions_pval1.end());
}
