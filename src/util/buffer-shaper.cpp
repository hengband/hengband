#include "util/buffer-shaper.h"
#include "locale/japanese.h"
#include <algorithm>

/*!
 * @brief 文字列を指定した最大長を目安として分割する
 *
 * 与えられた文字列 sv を指定した最大長 maxlen ごとに分割する。
 * 分割する際には以下の配慮を行う。
 *
 * - なるべく空白(' ')の部分で分割しようとする。具体的には表示可能なASCII文字が連続している途中で
 *   最大長に達した場合、最後に空白が出現した位置で分割する。
 *   但し、最大長に達した時点で連続している部分の長さが maxlen の約 1/3 以上になる場合は連続の途中でも分割する。
 * - 分割後の先頭の文字が空白である場合は残さず削除する。
 * - 改行文字('\\n')が検出された場合はその位置で強制的に分割する。
 * - 日本語（全角文字）の場合は基本的にどこでも分割するが、分割後の先頭の文字が kinsoku_list
 *   に含まれる場合はその1文字前で分割する。（いわゆる行頭禁則処理）
 * - 終端文字('\\0')の領域を残すため、分割後の文字列の長さは最大 maxlen - 1 バイトになるように分割する。
 *
 * @param sv 分割する文字列
 * @param maxlen 分割する最大長
 * @return std::vector<std::string> 分割後の文字列を要素とする配列
 */
std::vector<std::string> shape_buffer(std::string_view sv, size_t maxlen)
{
    std::vector<std::string> result;
    std::string line;
    auto separate_pos = 0U;

    while (line.length() < sv.length()) {
        const auto is_kanji = _(iskanji(sv[line.length()]), false);
        auto ch = sv.substr(line.length(), is_kanji ? 2 : 1);
        const auto is_newline = ch[0] == '\n';

        if ((ch.length() == 1) && !isprint(ch[0])) {
            ch = " ";
        }

        if ((line.length() + ch.length() >= maxlen) || is_newline) {
            if ((ch[0] == ' ') || (line.length() >= separate_pos * 2) || (is_kanji && !is_kinsoku(ch))) {
                sv.remove_prefix(line.length());
            } else {
                line.erase(separate_pos);
                sv.remove_prefix(separate_pos);
            }
            if ((sv.front() == ' ') || is_newline) {
                sv.remove_prefix(1);
            }

            result.push_back(std::move(line));
            line.clear();
            separate_pos = 0;
            continue;
        }

        if ((ch[0] == ' ') || is_kanji) {
            separate_pos = line.length();
        }

        line.append(ch);
    }

    if (!line.empty()) {
        result.push_back(std::move(line));
    }

    return result;
}
