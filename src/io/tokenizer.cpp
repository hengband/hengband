#include "io/tokenizer.h"
#include "locale/multibyte-char.h"

/*!
 * @brief 各種データテキストをトークン単位に分解する
 * @param buf データテキスト
 * @param num 最大トークン数
 * @return トークン文字列の配列
 */
std::vector<std::string> tokenize(std::string_view buf, size_t num)
{
    std::vector<std::string> tokens;
    if (num == 0) {
        return tokens;
    }

    tokens.reserve(num);
    auto remaining = buf;
    while ((tokens.size() < num - 1) && !remaining.empty()) {
        size_t token_end = 0;
        for (; token_end < remaining.length(); token_end++) {
            const auto c = remaining.at(token_end);
            if ((c == ':') || (c == '/')) {
                break;
            }

            if (c == '\\') {
                token_end++;
            } else {
                // 不正な並びの多バイト文字のために、後ろの区切りを読み飛ばさない
                token_end += mb_valid_char_length_at(remaining, token_end) - 1;
            }
        }

        tokens.emplace_back(remaining.substr(0, token_end));
        if (token_end >= remaining.length()) {
            return tokens;
        }

        remaining = remaining.substr(token_end + 1);
    }

    tokens.emplace_back(remaining);
    return tokens;
}
