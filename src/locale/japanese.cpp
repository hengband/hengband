/*!
 *  @file japanese.cpp
 *  @brief 日本語処理関数
 *  @date 2014/07/07
 */

#include "locale/japanese.h"
#include "util/enum-converter.h"
#include <set>
#include <sstream>

#ifdef JP

struct convert_key;

struct convert_key {
    concptr key1;
    concptr key2;
};

static const convert_key s2j_table[] = { { "mb", "nb" }, { "mp", "np" }, { "mv", "nv" }, { "mm", "nm" }, { "x", "ks" },
    /* sindar:シンダール  parantir:パランティア  feanor:フェアノール */
    { "ar$", "a-ru$" }, { "ir$", "ia$" }, { "or$", "o-ru$" }, { "ra", "ラ" }, { "ri", "リ" }, { "ru", "ル" }, { "re", "レ" }, { "ro", "ロ" }, { "ir", "ia" },
    { "ur", "ua" }, { "er", "ea" }, { "ar", "aル" }, { "sha", "シャ" }, { "shi", "シ" }, { "shu", "シュ" }, { "she", "シェ" }, { "sho", "ショ" },
    { "tha", "サ" }, { "thi", "シ" }, { "thu", "ス" }, { "the", "セ" }, { "tho", "ソ" }, { "cha", "ハ" }, { "chi", "ヒ" }, { "chu", "フ" }, { "che", "ヘ" },
    { "cho", "ホ" }, { "dha", "ザ" }, { "dhi", "ジ" }, { "dhu", "ズ" }, { "dhe", "ゼ" }, { "dho", "ゾ" }, { "ba", "バ" }, { "bi", "ビ" }, { "bu", "ブ" },
    { "be", "ベ" }, { "bo", "ボ" }, { "ca", "カ" }, { "ci", "キ" }, { "cu", "ク" }, { "ce", "ケ" }, { "co", "コ" }, { "da", "ダ" }, { "di", "ディ" },
    { "du", "ドゥ" }, { "de", "デ" }, { "do", "ド" }, { "fa", "ファ" }, { "fi", "フィ" }, { "fu", "フ" }, { "fe", "フェ" }, { "fo", "フォ" }, { "ga", "ガ" },
    { "gi", "ギ" }, { "gu", "グ" }, { "ge", "ゲ" }, { "go", "ゴ" }, { "ha", "ハ" }, { "hi", "ヒ" }, { "hu", "フ" }, { "he", "ヘ" }, { "ho", "ホ" },
    { "ja", "ジャ" }, { "ji", "ジ" }, { "ju", "ジュ" }, { "je", "ジェ" }, { "jo", "ジョ" }, { "ka", "カ" }, { "ki", "キ" }, { "ku", "ク" }, { "ke", "ケ" },
    { "ko", "コ" }, { "la", "ラ" }, { "li", "リ" }, { "lu", "ル" }, { "le", "レ" }, { "lo", "ロ" }, { "ma", "マ" }, { "mi", "ミ" }, { "mu", "ム" },
    { "me", "メ" }, { "mo", "モ" }, { "na", "ナ" }, { "ni", "ニ" }, { "nu", "ヌ" }, { "ne", "ネ" }, { "no", "ノ" }, { "pa", "パ" }, { "pi", "ピ" },
    { "pu", "プ" }, { "pe", "ペ" }, { "po", "ポ" }, { "qu", "ク" }, { "sa", "サ" }, { "si", "シ" }, { "su", "ス" }, { "se", "セ" }, { "so", "ソ" },
    { "ta", "タ" }, { "ti", "ティ" }, { "tu", "トゥ" }, { "te", "テ" }, { "to", "ト" }, { "va", "ヴァ" }, { "vi", "ヴィ" }, { "vu", "ヴ" }, { "ve", "ヴェ" },
    { "vo", "ヴォ" }, { "wa", "ワ" }, { "wi", "ウィ" }, { "wu", "ウ" }, { "we", "ウェ" }, { "wo", "ウォ" }, { "ya", "ヤ" }, { "yu", "ユ" }, { "yo", "ヨ" },
    { "za", "ザ" }, { "zi", "ジ" }, { "zu", "ズ" }, { "ze", "ゼ" }, { "zo", "ゾ" }, { "dh", "ズ" }, { "ch", "フ" }, { "th", "ス" }, { "b", "ブ" },
    { "c", "ク" }, { "d", "ド" }, { "f", "フ" }, { "g", "グ" }, { "h", "フ" }, { "j", "ジュ" }, { "k", "ク" }, { "l", "ル" }, { "m", "ム" }, { "n", "ン" },
    { "p", "プ" }, { "q", "ク" }, { "r", "ル" }, { "s", "ス" }, { "t", "ト" }, { "v", "ヴ" }, { "w", "ウ" }, { "y", "イ" }, { "a", "ア" }, { "i", "イ" },
    { "u", "ウ" }, { "e", "エ" }, { "o", "オ" }, { "-", "ー" }, { nullptr, nullptr } };

/*!
 * @brief シンダリンを日本語の読みに変換する
 * @param sindarin 変換前のシンダリン文字列
 * @return std::string 変換後のシンダリン文字列
 * @details
 */
std::string sindarin_to_kana(std::string_view sindarin)
{
    std::string kana;

    for (const auto &ch : sindarin) {
        kana.push_back(isupper(ch) ? static_cast<char>(tolower(ch)) : ch);
    }
    kana.append("$");

    for (auto idx = 0; s2j_table[idx].key1 != nullptr; idx++) {
        concptr pat1 = s2j_table[idx].key1;
        size_t len = strlen(pat1);
        std::string::size_type i = 0;

        while (i < kana.length()) {
            if (strncmp(kana.data() + i, pat1, len) == 0) {
                concptr pat2 = s2j_table[idx].key2;

                kana.replace(i, len, pat2);
                i += strlen(pat2);
            } else {
                if (iskanji(kana[i])) {
                    ++i;
                }
                ++i;
            }
        }
    }

    kana.erase(kana.find('$'));
    return kana;
}

/*!
 * 日本語動詞活用 (打つ＞打って,打ち etc)
 * AND : 殴る,蹴る > 殴り,蹴る
 * TO  : 殴る,蹴る > 殴って蹴る
 * OR  : 殴る,蹴る > 殴ったり蹴ったり
 */
static constexpr struct jverb_table_t {
    std::string_view from;
    std::string_view to_list[3];
} jverb_table[] = {
    { "する", { "し", "して", "した" } },
    { "いる", { "いて", "いて", "いた" } },

    { "える", { "え", "えて", "えた" } },
    { "ける", { "け", "けて", "けた" } },
    { "げる", { "げ", "えて", "げた" } },
    { "せる", { "せ", "せて", "せた" } },
    { "ぜる", { "ぜ", "ぜて", "ぜた" } },
    { "てる", { "て", "てって", "てった" } },
    { "でる", { "で", "でて", "でた" } },
    { "ねる", { "ね", "ねて", "ねた" } },
    { "へる", { "へ", "へて", "へた" } },
    { "べる", { "べ", "べて", "べた" } },
    { "める", { "め", "めて", "めた" } },
    { "れる", { "れ", "れて", "れた" } },

    { "う", { "い", "って", "った" } },
    { "く", { "き", "いて", "いた" } },
    { "ぐ", { "ぎ", "いで", "いだ" } },
    { "す", { "し", "して", "した" } },
    { "ず", { "じ", "じて", "じた" } },
    { "つ", { "ち", "って", "った" } },
    { "づ", { "ぢ", "って", "った" } },
    { "ぬ", { "に", "ねて", "ねた" } },
    { "ふ", { "ひ", "へて", "へた" } },
    { "ぶ", { "び", "んで", "んだ" } },
    { "む", { "み", "んで", "んだ" } },
    { "る", { "り", "って", "った" } },
};

/*!
 * @brief jverb_table_tに従って動詞を活用する
 * @param in 変換元となる原形動詞
 * @param type 変換種類を指定(AND/TO/OR)
 * @return 活用形の動詞
 */
std::string conjugate_jverb(std::string_view in, JVerbConjugationType type)
{
    std::stringstream ss;

    for (const auto &[from, to_list] : jverb_table) {
        const auto stem_length = in.length() - from.length();
        if (in.substr(stem_length) == from) {
            ss << in.substr(0, stem_length) << to_list[enum2i(type)];
            return ss.str();
        }
    }

    constexpr std::string_view conjuctions[3] = {
        "そして",
        "ことにより",
        "ことや",
    };

    ss << in << conjuctions[enum2i(type)];
    return ss.str();
}

static const std::set<std::string_view> kinsoku_list{
    // clang-format off
    "、", "。", "，", "．", "？", "！",
    "ァ", "ィ", "ゥ", "ェ", "ォ", "ャ", "ュ", "ョ", "ッ",
    "ぁ", "ぃ", "ぅ", "ぇ", "ぉ", "ゃ", "ゅ", "ょ", "っ",
    "ー", "～",
    "」", "』", "）", "｝", "］", "》", "】",
    // clang-format on
};

/*!
 * @brief 引数で与えられた文字が行頭禁則文字であるかどうか調べる
 *
 * @param ch 調べる文字
 * @return 行頭禁則文字であるなら true、そうでないなら false
 */
bool is_kinsoku(std::string_view ch)
{
    return (ch.length() >= 2) && kinsoku_list.contains(ch);
}

/*!
 * @brief 変愚蛮怒基準のポンド→キログラム変換定義(全体)
 * @param x ポンド値
 * @return キログラム値
 * @details 帝国ポンドとは完全にずれているが、気にするな！
 */
static int lb_to_kg_all(int x)
{
    return x * 5;
}

/*!
 * @brief 変愚蛮怒基準のポンド→キログラム変換定義(整数部)
 * @param x ポンド値
 * @return キログラム値の整数部
 */
int lb_to_kg_integer(int x)
{
    return lb_to_kg_all(x) / 100;
}

/*!
 * 変愚蛮怒基準のポンド→キログラム変換定義(小数部)
 * @param x ポンド値
 * @return キログラム値の小数部
 */
int lb_to_kg_fraction(int x)
{
    return (lb_to_kg_all(x) % 100) / 10;
}

#endif /* JP */
