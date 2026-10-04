#include "io/tokenizer.h"
#include "system/h-basic.h"

#ifdef JP
namespace {
/*!
 * @brief 有効なローカル文字コードの文字長を返す。不正・不完全な並びは1バイトとして扱う。
 */
size_t multibyte_character_length(std::string_view text)
{
    if (text.size() < 2) {
        return 1;
    }

    const auto lead = static_cast<unsigned char>(text[0]);
    const auto trail = static_cast<unsigned char>(text[1]);
#ifdef _WIN32
    const auto valid_trail = (trail >= 0x40 && trail <= 0x7e) || (trail >= 0x80 && trail <= 0xfc);
    return iskanji(lead) && valid_trail ? 2 : 1;
#else
    const auto valid_trail = trail >= 0xa1 && trail <= 0xfe;
    if (lead == 0x8f) {
        return text.size() >= 3 && valid_trail && static_cast<unsigned char>(text[2]) >= 0xa1 && static_cast<unsigned char>(text[2]) <= 0xfe ? 3 : 1;
    }
    if (lead == 0x8e) {
        return trail >= 0xa1 && trail <= 0xdf ? 2 : 1;
    }
    return lead >= 0xa1 && lead <= 0xfe && valid_trail ? 2 : 1;
#endif
}
}
#endif

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
            else {
                token_end += multibyte_character_length(remaining.substr(token_end)) - 1;
            }
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
