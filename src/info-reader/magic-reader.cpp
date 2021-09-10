#include "info-reader/magic-reader.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "main/angband-headers.h"
#include "player-ability/player-ability-types.h"
#include "player-info/class-info.h"
#include "util/string-processor.h"

namespace {
/*!
 * @brief 魔法タイプ名とtvalの対応表
 */
const std::unordered_map<std::string_view, tval_type> name_to_tval = {
    { "SORCERY", TV_SORCERY_BOOK },
    { "LIFE", TV_LIFE_BOOK },
    { "MUSIC", TV_MUSIC_BOOK },
    { "HISSATSU", TV_HISSATSU_BOOK },
    { "NONE", TV_NONE },
};

/*!
 * @brief 魔法必須能力とenumの対応表
 */
const std::unordered_map<std::string_view, int> name_to_stat = {
    { "STR", A_STR },
    { "INT", A_INT },
    { "WIS", A_WIS },
    { "DEX", A_DEX },
    { "CON", A_CON },
    { "CHR", A_CHR },
};
}

/*!
 * @brief 職業魔法情報(m_info)のパース関数 /
 * Initialize the "m_info" array, by parsing an ascii "template" file
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_m_info(std::string_view buf, angband_header *head)
{
    static player_magic *m_ptr = nullptr;
    static int realm, magic_idx = 0, readable = 0;
    const auto &tokens = str_split(buf, ':', false, 7);

    if (tokens[0] == "N") {
        // N:class-index
        if (tokens.size() < 2 && tokens[1].size() == 0)
            return PARSE_ERROR_GENERIC;

        auto i = std::stoi(tokens[1]);
        if (i < error_idx)
            return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
        if (i >= head->info_num)
            return PARSE_ERROR_OUT_OF_BOUNDS;

        error_idx = i;
        m_ptr = &m_info[i];
    } else if (!m_ptr)
        return PARSE_ERROR_MISSING_RECORD_HEADER;
    else if (tokens[0] == "I") {
        if (tokens.size() < 7 || tokens[1].size() == 0)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;

        const auto tval = name_to_tval.find(tokens[1]);
        if (tval == name_to_tval.end())
            return PARSE_ERROR_INVALID_FLAG;

        m_ptr->spell_book = tval->second;

        const auto stat = name_to_stat.find(tokens[2]);
        if (stat == name_to_stat.end())
            return PARSE_ERROR_INVALID_FLAG;

        m_ptr->spell_stat = stat->second;

        info_set_value(m_ptr->spell_xtra, tokens[3], 16);
        info_set_value(m_ptr->spell_type, tokens[4]);
        info_set_value(m_ptr->spell_first, tokens[5]);
        info_set_value(m_ptr->spell_weight, tokens[6]);
    } else if (tokens[0] == "R") {
        if (tokens.size() < 3)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;

        info_set_value(realm, tokens[1]);
        info_set_value(readable, tokens[2]);
        magic_idx = 0;
    } else if (tokens[0] == "T") {
        if (!readable)
            return PARSE_ERROR_GENERIC;

        if (tokens.size() < 5)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;

        auto &magic = m_ptr->info[realm][magic_idx];
        info_set_value(magic.slevel, tokens[1]);
        info_set_value(magic.smana, tokens[2]);
        info_set_value(magic.sfail, tokens[3]);
        info_set_value(magic.sexp, tokens[4]);
        magic_idx++;
    } else
        return PARSE_ERROR_UNDEFINED_DIRECTIVE;

    return PARSE_ERROR_NONE;
}
