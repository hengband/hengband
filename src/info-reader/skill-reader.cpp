#include "info-reader/skill-reader.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "main/angband-headers.h"
#include "object/tval-types.h"
#include "player/player-skill.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"

namespace {
const std::unordered_map<int, int> level_to_exp = {
    { EXP_LEVEL_UNSKILLED, WEAPON_EXP_UNSKILLED },
    { EXP_LEVEL_BEGINNER, WEAPON_EXP_BEGINNER },
    { EXP_LEVEL_SKILLED, WEAPON_EXP_SKILLED },
    { EXP_LEVEL_EXPERT, WEAPON_EXP_EXPERT },
    { EXP_LEVEL_MASTER, WEAPON_EXP_MASTER },
};
}

/*!
 * @brief 職業技能情報(s_info)のパース関数 /
 * Initialize the "s_info" array, by parsing an ascii "template" file
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_s_info(std::string_view buf, angband_header *head)
{
    static skill_table *s_ptr = nullptr;
    const auto &tokens = str_split(buf, ':', false, 5);

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
        s_ptr = &s_info[i];
    } else if (!s_ptr)
        return PARSE_ERROR_MISSING_RECORD_HEADER;
    else if(tokens[0] == "W") {
        if (tokens.size() < 5)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;

        int tval_offset, sval, start, max;
        info_set_value(tval_offset, tokens[1]);
        info_set_value(sval, tokens[2]);
        info_set_value(start, tokens[3]);
        info_set_value(max, tokens[4]);

        auto start_exp = level_to_exp.find(start);
        if (start_exp == level_to_exp.end())
            return PARSE_ERROR_INVALID_FLAG;

        auto max_exp = level_to_exp.find(max);
        if (max_exp == level_to_exp.end())
            return PARSE_ERROR_INVALID_FLAG;

        auto tval = ItemKindType::BOW + tval_offset;
        s_ptr->w_start[tval][sval] = (SUB_EXP)start_exp->second;
        s_ptr->w_max[tval][sval] = (SUB_EXP)max_exp->second;
    } else if (tokens[0] == "S") {
        if (tokens.size() < 4)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;

        int num, start, max;
        info_set_value(num, tokens[1]);
        info_set_value(start, tokens[2]);
        info_set_value(max, tokens[3]);

        if (start < WEAPON_EXP_UNSKILLED || start > WEAPON_EXP_MASTER || max < WEAPON_EXP_UNSKILLED || max > WEAPON_EXP_MASTER || start > max)
            return PARSE_ERROR_INVALID_FLAG;

        s_ptr->s_start[num] = (SUB_EXP)start;
        s_ptr->s_max[num] = (SUB_EXP)max;
    } else
        return PARSE_ERROR_UNDEFINED_DIRECTIVE;

    return PARSE_ERROR_NONE;
}
