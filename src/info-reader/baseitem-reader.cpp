﻿/*
 * @brief ベースアイテム定義の読み込み処理
 * @author Hourier
 * @date 2022/10/10
 */

#include "info-reader/baseitem-reader.h"
#include "artifact/random-art-effects.h"
#include "info-reader/baseitem-tokens-table.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "main/angband-headers.h"
#include "object-enchant/tr-types.h"
#include "object/tval-types.h"
#include "system/baseitem-info.h"
#include "term/gameterm.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(ベースアイテム用) /
 * Grab one flag in an BaseitemInfo from a textual string
 * @param bii_ptr 保管先のベースアイテム構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return 見つけたらtrue
 */
static bool grab_one_baseitem_flag(BaseitemInfo *bii_ptr, std::string_view what)
{
    if (TrFlags::grab_one_flag(bii_ptr->flags, baseitem_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<ItemGenerationTraitType>::grab_one_flag(bii_ptr->gen_flags, baseitem_geneneration_flags, what)) {
        return true;
    }

    msg_format(_("未知のアイテム・フラグ '%s'。", "Unknown object flag '%s'."), what.data());
    return false;
}

/*!
 * @brief ベースアイテム(BaseitemDefinitions)のパース関数
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_baseitems_info(std::string_view buf, angband_header *head)
{
    (void)head;
    static BaseitemInfo *bii_ptr = nullptr;
    const auto &tokens = str_split(buf, ':', false, 10);

    if (tokens[0] == "N") {
        // N:index:name_ja
        if (tokens.size() < 3 || tokens[1].size() == 0) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        auto i = std::stoi(tokens[1]);
        if (i < error_idx) {
            return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
        }
        if (i >= static_cast<int>(baseitems_info.size())) {
            baseitems_info.resize(i + 1);
        }

        error_idx = i;
        bii_ptr = &baseitems_info[i];
        bii_ptr->idx = static_cast<short>(i);
#ifdef JP
        bii_ptr->name = tokens[2];
#endif
        if (tokens.size() > 3) {
            bii_ptr->flavor_name = tokens[3];
        }

    } else if (!bii_ptr) {
        return PARSE_ERROR_MISSING_RECORD_HEADER;
    } else if (tokens[0] == "E") {
        // E:name_en
#ifndef JP
        if (tokens.size() < 2 || tokens[1].size() == 0) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }
        bii_ptr->name = tokens[1];

        if (tokens.size() > 2) {
            bii_ptr->flavor_name = tokens[2];
        }
#endif
    } else if (tokens[0] == "D") {
        // D:text_ja
        // D:$text_en
        if (tokens.size() < 2 || tokens[1].size() == 0) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }
#ifdef JP
        if (tokens[1][0] == '$') {
            return PARSE_ERROR_NONE;
        }
        bii_ptr->text.append(buf.substr(2));
#else
        if (tokens[1][0] != '$') {
            return PARSE_ERROR_NONE;
        }
        append_english_text(bii_ptr->text, buf.substr(3));
#endif
    } else if (tokens[0] == "G") {
        // G:color:symbol
        if (tokens.size() < 3 || tokens[1].size() == 0 || tokens[2].size() == 0) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        auto a = color_char_to_attr(tokens[2][0]);
        if (a > 127) {
            return PARSE_ERROR_GENERIC;
        }

        bii_ptr->d_attr = a;
        bii_ptr->d_char = tokens[1][0];
    } else if (tokens[0] == "I") {
        // I:tval:sval:pval
        if (tokens.size() < 4) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        constexpr auto base = 10;
        const auto tval = i2enum<ItemKindType>(std::stoi(tokens[1], nullptr, base));
        const auto sval = std::stoi(tokens[2], nullptr, base);
        bii_ptr->bi_key = { tval, sval };
        info_set_value(bii_ptr->pval, tokens[3]);
        if ((tval == ItemKindType::ROD) && (bii_ptr->pval <= 0)) {
            return PAESE_ERROR_INVALID_PVAL;
        }
    } else if (tokens[0] == "W") {
        // W:level:weight:cost
        if (tokens.size() < 4) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        info_set_value(bii_ptr->level, tokens[1]);
        info_set_value(bii_ptr->weight, tokens[2]);
        info_set_value(bii_ptr->cost, tokens[3]);
    } else if (tokens[0] == "A") {
        // A:level/chance(:level/chance:level/chance:level/chance)
        if (tokens.size() < 2 || tokens.size() > 5) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        auto i = 0;
        for (auto t = tokens.begin() + 1; t != tokens.end(); t++) {
            const auto &rarity = str_split(*t, '/', false, 2);
            if (rarity.size() != 2 || rarity[0].size() == 0 || rarity[1].size() == 0) {
                return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
            }
            info_set_value(bii_ptr->alloc_levels[i], rarity[0]);
            info_set_value(bii_ptr->alloc_chances[i], rarity[1]);
            i++;
        }
    } else if (tokens[0] == "P") {
        // P:ac:dd:ds:to_h:to_d:to_a
        if (tokens.size() < 6) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto &dice = str_split(tokens[2], 'd', false, 2);
        if (dice.size() != 2) {
            return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
        }

        info_set_value(bii_ptr->ac, tokens[1]);
        info_set_value(bii_ptr->dd, dice[0]);
        info_set_value(bii_ptr->ds, dice[1]);
        info_set_value(bii_ptr->to_h, tokens[3]);
        info_set_value(bii_ptr->to_d, tokens[4]);
        info_set_value(bii_ptr->to_a, tokens[5]);
    } else if (tokens[0] == "U") {
        // U:activation_flag
        if (tokens.size() < 2 || tokens[1].size() == 0) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }
        auto n = grab_one_activation_flag(tokens[1].data());
        if (n <= RandomArtActType::NONE) {
            return PARSE_ERROR_INVALID_FLAG;
        }

        bii_ptr->act_idx = n;
    } else if (tokens[0] == "F") {
        // F:flags
        if (tokens.size() < 2 || tokens[1].size() == 0) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto &flags = str_split(tokens[1], '|', true, 10);
        for (const auto &f : flags) {
            if (f.size() == 0) {
                continue;
            }
            if (!grab_one_baseitem_flag(bii_ptr, f)) {
                return PARSE_ERROR_INVALID_FLAG;
            }
        }
    } else {
        return PARSE_ERROR_UNDEFINED_DIRECTIVE;
    }

    return PARSE_ERROR_NONE;
}
