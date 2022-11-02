#include "info-reader/artifact-reader.h"
#include "artifact/fixed-art-types.h"
#include "artifact/random-art-effects.h"
#include "info-reader/baseitem-tokens-table.h"
#include "info-reader/info-reader-util.h"
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
static bool grab_one_artifact_flag(ArtifactType *a_ptr, std::string_view what)
{
    if (TrFlags::grab_one_flag(a_ptr->flags, baseitem_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<ItemGenerationTraitType>::grab_one_flag(a_ptr->gen_flags, baseitem_geneneration_flags, what)) {
        return true;
    }

    msg_format(_("未知の伝説のアイテム・フラグ '%s'。", "Unknown artifact flag '%s'."), what.data());
    return false;
}

/*!
 * @brief 固定アーティファクト定義(ArtifactDefinitions)のパース関数
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_artifacts_info(std::string_view buf, angband_header *)
{
    const auto &tokens = str_split(buf, ':', false, 10);
    if (tokens[0] == "N") {
        // N:index:name_ja
        if (tokens.size() < 3 || tokens[1].size() == 0) {
            return PARSE_ERROR_GENERIC;
        }

        const auto int_idx = std::stoi(tokens[1]);
        const auto a_idx = i2enum<FixedArtifactId>(int_idx);
        if (int_idx < error_idx) {
            return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
        }

        error_idx = int_idx;
        ArtifactType artifact;
        artifact.flags.set(TR_IGNORE_ACID);
        artifact.flags.set(TR_IGNORE_ELEC);
        artifact.flags.set(TR_IGNORE_FIRE);
        artifact.flags.set(TR_IGNORE_COLD);
#ifdef JP
        artifact.name = tokens[2];
#endif
        artifacts_info.emplace(a_idx, artifact);
        return PARSE_ERROR_NONE;
    }

    if (tokens[0] == "E") {
        // E:name_en
#ifdef JP
        return PARSE_ERROR_NONE;
#else
        if (tokens[1].size() == 0) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto it = artifacts_info.rbegin();
        auto &a_ref = it->second;
        a_ref.name = tokens[1];
        return PARSE_ERROR_NONE;
#endif
    }

    if (tokens[0] == "D") {
        // D:JapaneseText
        // D:$EnglishText
        if (tokens.size() < 2 || tokens[1].size() == 0) {
            return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
        }
#ifdef JP
        if (tokens[1][0] == '$') {
            return PARSE_ERROR_NONE;
        }

        const auto it = artifacts_info.rbegin();
        auto &a_ref = it->second;
        a_ref.text.append(buf.substr(2));
#else
        if (tokens[1][0] != '$') {
            return PARSE_ERROR_NONE;
        }

        const auto it = artifacts_info.rbegin();
        auto &a_ref = it->second;
        append_english_text(a_ref.text, buf.substr(3));
#endif
        return PARSE_ERROR_NONE;
    }

    if (tokens[0] == "I") {
        // I:tval:sval:pval
        if (tokens.size() < 4) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto it = artifacts_info.rbegin();
        auto &a_ref = it->second;
        info_set_value(a_ref.tval, tokens[1]);
        info_set_value(a_ref.sval, tokens[2]);
        info_set_value(a_ref.pval, tokens[3]);
        return PARSE_ERROR_NONE;
    }

    if (tokens[0] == "W") {
        // W:level:ratiry:weight:cost
        if (tokens.size() < 5) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto it = artifacts_info.rbegin();
        auto &a_ref = it->second;
        info_set_value(a_ref.level, tokens[1]);
        info_set_value(a_ref.rarity, tokens[2]);
        info_set_value(a_ref.weight, tokens[3]);
        info_set_value(a_ref.cost, tokens[4]);
        return PARSE_ERROR_NONE;
    }

    if (tokens[0] == "P") {
        // P:ac:dd:ds:to_h:to_d:to_a
        if (tokens.size() < 6) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto &dice = str_split(tokens[2], 'd', false, 2);
        if (dice.size() != 2) {
            return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
        }

        const auto it = artifacts_info.rbegin();
        auto &a_ref = it->second;
        info_set_value(a_ref.ac, tokens[1]);
        info_set_value(a_ref.dd, dice[0]);
        info_set_value(a_ref.ds, dice[1]);
        info_set_value(a_ref.to_h, tokens[3]);
        info_set_value(a_ref.to_d, tokens[4]);
        info_set_value(a_ref.to_a, tokens[5]);
        return PARSE_ERROR_NONE;
    }

    if (tokens[0] == "U") {
        // U:activation_flag
        if (tokens.size() < 2 || tokens[1].size() == 0) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        auto n = grab_one_activation_flag(tokens[1].data());
        if (n <= RandomArtActType::NONE) {
            return PARSE_ERROR_INVALID_FLAG;
        }

        const auto it = artifacts_info.rbegin();
        auto &a_ref = it->second;
        a_ref.act_idx = n;
        return PARSE_ERROR_NONE;
    }

    if (tokens[0] == "F") {
        // F:flags
        if (tokens.size() < 2 || tokens[1].size() == 0) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto &flags = str_split(tokens[1], '|', true, 10);
        for (const auto &f : flags) {
            if (f.size() == 0) {
                continue;
            }

            const auto it = artifacts_info.rbegin();
            auto *a_ptr = &it->second;
            if (!grab_one_artifact_flag(a_ptr, f)) {
                return PARSE_ERROR_INVALID_FLAG;
            }
        }

        return PARSE_ERROR_NONE;
    }

    return PARSE_ERROR_UNDEFINED_DIRECTIVE;
}
