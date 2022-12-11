/*
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
    const auto &tokens = str_split(buf, ':', false, 10);
    if (tokens[0] == "N") {
        // N:index:name_ja
        if (tokens.size() < 3 || tokens[1].size() == 0) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        auto i = static_cast<short>(std::stoi(tokens[1]));
        if (i < error_idx) {
            return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
        }

        if (i >= static_cast<short>(baseitems_info.size())) {
            baseitems_info.emplace(i, BaseitemInfo());
        }

        error_idx = i;
        auto &baseitem = baseitems_info[i];
#ifdef JP
        baseitem.name = tokens[2];
#endif
        if (tokens.size() > 3) {
            baseitem.flavor_name = tokens[3];
        }

        return PARSE_ERROR_NONE;
    }

    if (baseitems_info.empty()) {
        return PARSE_ERROR_MISSING_RECORD_HEADER;
    }

    if (tokens[0] == "E") {
        // E:name_en
#ifndef JP
        if (tokens.size() < 2 || tokens[1].size() == 0) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto itr = baseitems_info.rbegin();
        auto &baseitem = itr->second;
        baseitem.name = tokens[1];
        if (tokens.size() > 2) {
            baseitem.flavor_name = tokens[2];
        }
#endif
        return PARSE_ERROR_NONE;
    }

    if (tokens[0] == "D") {
        // D:text_ja
        // D:$text_en
        if (tokens.size() < 2 || tokens[1].size() == 0) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }
#ifdef JP
        if (tokens[1][0] == '$') {
            return PARSE_ERROR_NONE;
        }
#else
        if (tokens[1][0] != '$') {
            return PARSE_ERROR_NONE;
        }
#endif
        const auto itr = baseitems_info.rbegin();
        auto &baseitem = itr->second;
        baseitem.text.append(tokens[1].substr(_(0, 1)));
        return PARSE_ERROR_NONE;
    }

    if (tokens[0] == "G") {
        // G:color:symbol
        if (tokens.size() < 3 || tokens[1].size() == 0 || tokens[2].size() == 0) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        auto a = color_char_to_attr(tokens[2][0]);
        if (a > 127) {
            return PARSE_ERROR_GENERIC;
        }

        const auto itr = baseitems_info.rbegin();
        auto &baseitem = itr->second;
        baseitem.d_attr = a;
        baseitem.d_char = tokens[1][0];
        return PARSE_ERROR_NONE;
    }

    if (tokens[0] == "I") {
        // I:tval:sval:pval
        if (tokens.size() < 4) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        constexpr auto base = 10;
        const auto tval = i2enum<ItemKindType>(std::stoi(tokens[1], nullptr, base));
        const auto sval = std::stoi(tokens[2], nullptr, base);
        const auto itr = baseitems_info.rbegin();
        auto &baseitem = itr->second;
        baseitem.bi_key = { tval, sval };
        info_set_value(baseitem.pval, tokens[3]);
        if ((tval == ItemKindType::ROD) && (baseitem.pval <= 0)) {
            return PAESE_ERROR_INVALID_PVAL;
        }

        return PARSE_ERROR_NONE;
    }

    if (tokens[0] == "W") {
        // W:level:weight:cost
        if (tokens.size() < 4) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto itr = baseitems_info.rbegin();
        auto &baseitem = itr->second;
        info_set_value(baseitem.level, tokens[1]);
        info_set_value(baseitem.weight, tokens[2]);
        info_set_value(baseitem.cost, tokens[3]);
        return PARSE_ERROR_NONE;
    }

    if (tokens[0] == "A") {
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

            const auto itr = baseitems_info.rbegin();
            auto &baseitem = itr->second;
            auto &table = baseitem.alloc_tables[i];
            info_set_value(table.level, rarity[0]);
            info_set_value(table.chance, rarity[1]);
            i++;
        }

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

        const auto itr = baseitems_info.rbegin();
        auto &baseitem = itr->second;
        info_set_value(baseitem.ac, tokens[1]);
        info_set_value(baseitem.dd, dice[0]);
        info_set_value(baseitem.ds, dice[1]);
        info_set_value(baseitem.to_h, tokens[3]);
        info_set_value(baseitem.to_d, tokens[4]);
        info_set_value(baseitem.to_a, tokens[5]);
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

        const auto itr = baseitems_info.rbegin();
        auto &baseitem = itr->second;
        baseitem.act_idx = n;
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

            const auto itr = baseitems_info.rbegin();
            auto *bii_ptr = &itr->second;
            if (!grab_one_baseitem_flag(bii_ptr, f)) {
                return PARSE_ERROR_INVALID_FLAG;
            }
        }

        return PARSE_ERROR_NONE;
    }

    return PARSE_ERROR_UNDEFINED_DIRECTIVE;
}
