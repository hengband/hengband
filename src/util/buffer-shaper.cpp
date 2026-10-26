#include "util/buffer-shaper.h"
#include "locale/japanese.h"
#include "util/string-processor.h"
#include <algorithm>

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

        if (!isprint(static_cast<unsigned char>(sv[i]))) {
            return false;
        }
    }

    return true;
}

/*!
 * @brief 行を分割できる位置
 * @details どちらも分割した後の次の行の先頭の位置 (バイト数)。分割できる位置が無ければ 0
 */
struct SeparatePositions {
    size_t word_break = 0; //!< 語の切れ目 (前後のどちらかが全角文字か、前が空白) で分割できる、もっとも後ろの位置
    size_t mid_word = 0; //!< 語の途中 (半角文字どうしの間、半角の閉じ括弧・句読点の前を除く) で分割できる、もっとも後ろの位置
};

/*!
 * @brief 組み立て中の行を分割できる位置を探す
 * @param line 組み立て中の行
 * @param ch 行に収まらなかった次の文字
 * @return 語の切れ目と語の途中の、分割できる位置
 * @details 分割した後の次の行の先頭の文字 (行の末尾で分割するなら ch) が、空白でも行頭禁則文字でもない位置だけを候補にする。
 * 行の末尾は、ch が全角文字か、行の末尾が空白のときだけ候補にする。
 * 全角文字は shape_buffer() と同じく、前半バイトの後に 1 バイト以上続くときに 2 バイトをまとめて見る。
 */
SeparatePositions find_separate_positions(std::string_view line, std::string_view ch)
{
    // 語の途中では、半角の閉じ括弧・句読点の前で分けない (次の行がこれらで始まり、前の語から離れるのを避ける)
    constexpr std::string_view closing_punctuations = ")]},.!?:;";
    SeparatePositions positions;
    auto is_prev_full_width = false;
    auto is_prev_space = false;
    for (size_t pos = 0; pos <= line.length();) {
        const auto is_end = pos == line.length();
        const auto is_full_width = is_end ? (ch.length() == 2) : is_multibyte_char_at(line, pos);
        const auto next_char = is_end ? ch : line.substr(pos, is_full_width ? 2 : 1);
        // 行の末尾で分割するのは、次の文字が全角文字か前が空白のときだけにする (全角文字の直後の半角文字なら、これまでどおり全角文字の前で分ける)
        const auto can_split = (pos > 0) && (!is_end || is_full_width || is_prev_space) && (next_char != " ") && !is_kinsoku(next_char);
        if (can_split) {
            if (is_full_width || is_prev_full_width || is_prev_space) {
                positions.word_break = pos;
            } else if (closing_punctuations.find(next_char[0]) == std::string_view::npos) {
                positions.mid_word = pos;
            }
        }

        if (is_end) {
            break;
        }

        is_prev_full_width = is_full_width;
        is_prev_space = next_char == " ";
        pos += next_char.length();
    }

    return positions;
}
}

/*!
 * @brief 文字列を指定した最大長を目安として分割する
 *
 * 与えられた文字列 sv を指定した最大長 maxlen ごとに分割する。
 * 分割する際には以下の配慮を行う。
 *
 * - なるべく語の切れ目 (空白の後ろ、全角文字の前後) で分割しようとする。具体的には表示可能なASCII文字が
 *   連続している途中で最大長に達した場合、最後の語の切れ目で分割する。
 *   但し、最後の語の切れ目が行の前半にある場合は、連続の途中でも分割する。
 * - 分割後の先頭の文字が空白である場合は残さず削除する。
 * - 改行文字('\\n')が検出された場合はその位置で強制的に分割する。
 * - 日本語（全角文字）の場合は基本的にどこでも分割するが、分割後の先頭の文字が行頭禁則文字 (is_kinsoku())
 *   にならないようにする。（いわゆる行頭禁則処理）
 *   行頭禁則文字が続く場合や、前が半角の単語や空白の場合は、その前の語の切れ目までさかのぼって分割する。
 *   語の切れ目が行の前半にしか無ければ、それより後ろの半角の単語の途中で分割する。
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
    while (line.length() < sv.length()) {
        const auto is_kanji = _(iskanji(sv[line.length()]), false);
        auto ch = sv.substr(line.length(), is_kanji ? 2 : 1);
        const auto is_newline = ch[0] == '\n';

        if ((ch.length() == 1) && !isprint(static_cast<unsigned char>(ch[0]))) {
            ch = " ";
        }

        if ((!line.empty() && (line.length() + ch.length() >= maxlen)) || is_newline) {
            auto split_pos = line.length();
            auto drops_space = false; // 分ける位置の前の空白を、前の行に残さないか
            // 空白で分けると空白の後ろの行頭禁則文字が次の行の先頭に来るときも、行頭禁則文字と同じく前で分ける
            auto is_before_kinsoku = false;
            if (ch[0] == ' ') {
                const auto rest = sv.substr(line.length());
                const auto next_pos = rest.find_first_not_of(' ');
                is_before_kinsoku = (next_pos != std::string_view::npos) && is_kinsoku(rest.substr(next_pos, 2));
            }

            if (!is_newline && ((ch[0] != ' ') || is_before_kinsoku)) {
                auto [word_break, mid_word] = find_separate_positions(line, ch);
                // 語の切れ目の前の空白は前の行に残さないので、空白を除いた長さで前半か後半かを決める (空白しか残らない位置は使わない)
                const auto has_space_before_break = (word_break > 0) && (line[word_break - 1] == ' ');
                const auto word_break_line_length = has_space_before_break ? word_break - 1 : word_break;
                if (word_break_line_length == 0) {
                    word_break = 0;
                }

                if (line.length() < word_break_line_length * 2) {
                    split_pos = word_break;
                } else if (is_kinsoku(ch) || is_before_kinsoku) {
                    // 語の切れ目が行の前半にしか無ければ、それより後ろの語の途中で分ける
                    const auto pos = std::max(word_break, mid_word);
                    split_pos = (pos > 0) ? pos : line.length();
                }

                // 全角文字の前で行の末尾で分けるときは、これまでどおり空白を残す
                drops_space = (split_pos == word_break) && has_space_before_break && ((split_pos < line.length()) || (ch.length() == 1));
            }

            line.erase(drops_space ? split_pos - 1 : split_pos);
            sv.remove_prefix(split_pos);
            if ((sv.front() == ' ') || is_newline) {
                sv.remove_prefix(1);
            }

            result.push_back(std::move(line));
            line.clear();
            continue;
        }

        line.append(ch);
    }

    if (!line.empty()) {
        result.push_back(std::move(line));
    }

    return result;
}
