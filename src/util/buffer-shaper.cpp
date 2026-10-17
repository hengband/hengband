#include "util/buffer-shaper.h"
#include "locale/japanese.h"
#include "util/string-processor.h"
#include <algorithm>
#include <array>

namespace {
/*!
 * @brief 文字列を分割せずにそのまま 1 行にできるか
 * @param sv 調べる文字列
 * @return 改行も表示できない文字 (空白に置き換える文字) も無ければ true
 * @details shape_buffer() と同じく全角文字は 2 バイトをまとめて見る。前半バイトで終わっている文字列は、
 * その前半バイトを 1 バイトの文字として表示できるかで判断する (shape_buffer() と同じ)
 */
bool is_printable_as_is(std::string_view sv)
{
    for (auto i = 0U; i < sv.length(); i++) {
        if (is_multibyte_char_at(sv, i)) {
            i++;
            continue;
        }

        if (!isprint(sv[i])) {
            return false;
        }
    }

    return true;
}
}

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
 *   但し、1文字も収まらない場合 (maxlen が 1 以下、日本語の全角文字では 2 以下) でも、各行に少なくとも1文字は入れる。
 *   1文字も入れずに分割すると、文字列を読み進められず空の行を際限なく追加してしまうため。
 *
 * @param sv 分割する文字列
 * @param maxlen 分割する最大長
 * @return std::vector<std::string> 分割後の文字列を要素とする配列
 */
std::vector<std::string> shape_buffer(std::string_view sv, size_t maxlen)
{
    // 1 行に収まる文字列 (メッセージの多く) は、1 文字ずつ組み立てずにそのまま返す
    if (!sv.empty() && (sv.length() < maxlen) && is_printable_as_is(sv)) {
        std::vector<std::string> one_line;
        one_line.emplace_back(sv);
        return one_line;
    }

    std::vector<std::string> result;
    std::string line;
    auto separate_pos = 0U;
    std::array<bool, 2> is_kanji{}; // [0]:現在の文字 [1]:一つ前の文字

    while (line.length() < sv.length()) {
        is_kanji[1] = is_kanji[0];
        is_kanji[0] = _(iskanji(sv[line.length()]), false);
        auto ch = sv.substr(line.length(), is_kanji[0] ? 2 : 1);
        const auto is_newline = ch[0] == '\n';

        if ((ch.length() == 1) && !isprint(ch[0])) {
            ch = " ";
        }

        if ((!line.empty() && (line.length() + ch.length() >= maxlen)) || is_newline) {
            if ((ch[0] == ' ') || (line.length() >= separate_pos * 2) || (is_kanji[0] && !is_kinsoku(ch))) {
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
            is_kanji.fill(false);
            continue;
        }

        if ((ch[0] == ' ') || is_kanji[0] || is_kanji[1]) {
            separate_pos = line.length();
        }

        line.append(ch);
    }

    if (!line.empty()) {
        result.push_back(std::move(line));
    }

    return result;
}
