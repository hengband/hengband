#include "info-reader/artifact-reader.h"
#include "artifact/random-art-effects.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/kind-info-tokens-table.h"
#include "info-reader/parse-error-types.h"
#include "main/angband-headers.h"
#include "object-enchant/tr-types.h"
#include "system/artifact-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(アーティファクト用) /
 * Grab one activation index flag
 * @param a_ptr 保管先のアーティファクト構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return 見つかったらtrue
 */
static bool grab_one_artifact_flag(artifact_type *a_ptr, std::string_view what)
{
    if (TrFlags::grab_one_flag(a_ptr->flags, k_info_flags, what))
        return true;

    if (EnumClassFlagGroup<TRG>::grab_one_flag(a_ptr->gen_flags, k_info_gen_flags, what))
        return true;

    msg_format(_("未知の伝説のアイテム・フラグ '%s'。", "Unknown artifact flag '%s'."), what.data());
    return false;
}

/*!
 * @brief 固定アーティファクト情報(a_info)のパース関数 /
 * Initialize the "a_info" array, by parsing an ascii "template" file
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_a_info(std::string_view buf, angband_header *)
{
    static artifact_type *a_ptr = nullptr;
    const auto &tokens = str_split(buf, ':', false, 10);

    if (tokens[0] == "N") {
        // N:index:name_ja
        if (tokens.size() < 3 || tokens[1].size() == 0)
            return PARSE_ERROR_GENERIC;

        auto i = std::stoi(tokens[1]);
        if (i < error_idx)
            return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
        if (i >= static_cast<int>(a_info.size())) {
            a_info.resize(i + 1);
        }

        error_idx = i;
        a_ptr = &a_info[i];
        a_ptr->idx = static_cast<ARTIFACT_IDX>(i);
        a_ptr->flags.set(TR_IGNORE_ACID);
        a_ptr->flags.set(TR_IGNORE_ELEC);
        a_ptr->flags.set(TR_IGNORE_FIRE);
        a_ptr->flags.set(TR_IGNORE_COLD);

#ifdef JP
        a_ptr->name = tokens[2];
#endif
    } else if (!a_ptr)
        return PARSE_ERROR_MISSING_RECORD_HEADER;
    else if (tokens[0] == "E") {
        // E:name_en
#ifndef JP
        if (tokens[1].size() == 0)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        a_ptr->name = tokens[1];
#endif
    } else if (tokens[0] == "D") {
        // D:JapaneseText
        // D:$EnglishText
        if (tokens.size() < 2 || tokens[1].size() == 0)
            return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
#ifdef JP
        if (tokens[1][0] == '$')
            return PARSE_ERROR_NONE;
        a_ptr->text.append(buf.substr(2));
#else
        if (tokens[1][0] != '$')
            return PARSE_ERROR_NONE;
        a_ptr->text.append(buf.substr(3));
#endif
    } else if (tokens[0] == "I") {
        // I:tval:sval:pval
        if (tokens.size() < 4)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;

        info_set_value(a_ptr->tval, tokens[1]);
        info_set_value(a_ptr->sval, tokens[2]);
        info_set_value(a_ptr->pval, tokens[3]);
    } else if (tokens[0] == "W") {
        // W:level:ratiry:weight:cost
        if (tokens.size() < 5)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;

        info_set_value(a_ptr->level, tokens[1]);
        info_set_value(a_ptr->rarity, tokens[2]);
        info_set_value(a_ptr->weight, tokens[3]);
        info_set_value(a_ptr->cost, tokens[4]);
    } else if (tokens[0] == "P") {
        // P:ac:dd:ds:to_h:to_d:to_a
        if (tokens.size() < 6)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;

        const auto &dice = str_split(tokens[2], 'd', false, 2);
        if (dice.size() != 2)
            return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;

        info_set_value(a_ptr->ac, tokens[1]);
        info_set_value(a_ptr->dd, dice[0]);
        info_set_value(a_ptr->ds, dice[1]);
        info_set_value(a_ptr->to_h, tokens[3]);
        info_set_value(a_ptr->to_d, tokens[4]);
        info_set_value(a_ptr->to_a, tokens[5]);
    } else if (tokens[0] == "U") {
        // U:activation_flag
        if (tokens.size() < 2 || tokens[1].size() == 0)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        auto n = grab_one_activation_flag(tokens[1].c_str());
        if (n <= RandomArtActType::NONE)
            return PARSE_ERROR_INVALID_FLAG;

        a_ptr->act_idx = n;
    } else if (tokens[0] == "F") {
        // F:flags
        if (tokens.size() < 2 || tokens[1].size() == 0)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;

        const auto &flags = str_split(tokens[1], '|', true, 10);
        for (const auto &f : flags) {
            if (f.size() == 0)
                continue;
            if (!grab_one_artifact_flag(a_ptr, f))
                return PARSE_ERROR_INVALID_FLAG;
        }
    } else
        return PARSE_ERROR_UNDEFINED_DIRECTIVE;

    return PARSE_ERROR_NONE;
}
