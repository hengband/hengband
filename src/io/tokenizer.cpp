#include "io/tokenizer.h"
#include "system/h-basic.h"

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
            }
#ifdef JP
#ifdef _WIN32
            else if (iskanji(c)) {
                token_end++;
            }
#else
            else if (static_cast<unsigned char>(c) == 0x8f) {
                token_end += 2;
            } else if (iskanji(c)) {
                token_end++;
            }
#endif
#endif
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
