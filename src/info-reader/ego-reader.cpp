#include "info-reader/ego-reader.h"
#include "info-reader/kind-info-tokens-table.h"
#include "main/angband-headers.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "parse-error-types.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include <string>
#include <utility>

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(エゴ用) /
 * Grab one flag in a ego-item_type from a textual string
 * @param e_ptr 保管先のエゴ構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return エラーがあった場合1、エラーがない場合0を返す
 */
static bool grab_one_ego_item_flag(ego_item_type *e_ptr, std::string_view what)
{
    if (k_info_flags.find(what) != k_info_flags.end()) {
        add_flag(e_ptr->flags, k_info_flags[what]);
        return 0;
    }

    if (EnumClassFlagGroup<TRG>::grab_one_flag(e_ptr->gen_flags, k_info_gen_flags, what))
        return 0;

    msg_format(_("未知の名のあるアイテム・フラグ '%s'。", "Unknown ego-item flag '%s'."), what);
    return 1;
}

static bool grab_ego_generate_flags(ego_generate_type &xtra, std::string_view what)
{
    if (k_info_flags.find(what) != k_info_flags.end()) {
        xtra.tr_flags.push_back(k_info_flags[what]);
        return 0;
    }

    auto it = k_info_gen_flags.find(what);
    if (it != k_info_gen_flags.end()) {
        xtra.trg_flags.push_back(it->second);
        return false;
    }

    return true;
}


/*!
 * @brief アイテムエゴ情報(e_info)のパース関数 /
 * Initialize the "e_info" array, by parsing an ascii "template" file
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_e_info(char *buf, angband_header *head)
{
    static ego_item_type *e_ptr = NULL;
    auto line = std::string(buf);
    auto tok = str_split(line, ':', false);

    error_idx = 0;

    if (tok[0] == "N") {
        // N:index:name_ja
        if (tok[1].size() == 0)
            return PARSE_ERROR_GENERIC;

        auto i = std::stoi(tok[1]);
        if (i < error_idx)
            return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
        if (i >= head->info_num)
            return PARSE_ERROR_OUT_OF_BOUNDS;

        error_idx = i;
        e_ptr = &e_info[i];
#ifdef JP
        e_ptr->name = tok[2];
#endif
    } else if (!e_ptr)
        return PARSE_ERROR_MISSING_RECORD_HEADER;
    else if (tok[0] == "E") {
        // E:name_en
#ifndef JP
        if (tok[1].size() == 0)
            return 1;
        e_ptr->name = tok[1];
#endif
    }
    else if (tok[0] == "X") {
        // X:slot:rating
        if (tok.size() < 3)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        e_ptr->slot = (INVENTORY_IDX)std::stoi(tok[1]);
        e_ptr->rating = (PRICE)std::stoi(tok[2]);
    } else if (tok[0] == "W") {
        // W:level:ratiry:xtra:cost
        // xtra is not used
        if (tok.size() < 5)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        e_ptr->level = (DEPTH)std::stoi(tok[1]);
        e_ptr->rarity = (RARITY)std::stoi(tok[2]);
        e_ptr->cost = (PRICE)std::stoi(tok[4]);
    } else if (tok[0] == "B") {
        // Base bonuses
        // B:to_hit:to_dam:to_ac
        if (tok.size() < 4)
            return 1;
        e_ptr->base_to_h = (HIT_PROB)std::stoi(tok[1]);
        e_ptr->base_to_d = (HIT_POINT)std::stoi(tok[2]);
        e_ptr->base_to_a = (ARMOUR_CLASS)std::stoi(tok[3]);
    } else if (tok[0] == "C") {
        // Creation bonuses (random plus)
        // C:to_hit:to_dam:to_ac:pval
        if (tok.size() < 5)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        e_ptr->max_to_h = (HIT_PROB)std::stoi(tok[1]);
        e_ptr->max_to_d = (HIT_POINT)std::stoi(tok[2]);
        e_ptr->max_to_a = (ARMOUR_CLASS)std::stoi(tok[3]);
        e_ptr->max_pval = (PARAMETER_VALUE)std::stoi(tok[4]);
    } else if (tok[0] == "U") {
        // U:activation_flag
        if (tok.size() < 2 || tok[1].size() == 0)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        auto n = grab_one_activation_flag(tok[1].c_str());
        if (n <= 0)
            return PARSE_ERROR_INVALID_FLAG;
        e_ptr->act_idx = (IDX)n;
    } else if (tok[0] == "F") {
        // F:flags
        if (tok.size() < 2 || tok[1].size() == 0)
            return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;

        const auto& flags = str_split(tok[1], '|', true);
        for (const auto& f : flags) {
            if (f.size() == 0)
                continue;
            if (0 != grab_one_ego_item_flag(e_ptr, f))
                return PARSE_ERROR_INVALID_FLAG;
        }
    } else if (tok[0] == "G") {
        // G:mul/dev:flags
        if (tok.size() < 3)
            return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;

        auto prob = str_split(tok[1], '/');
        if (prob.size() != 2 || tok[1].size() == 0 || tok[2].size() == 0)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;

        ego_generate_type xtra;
        xtra.mul = std::stoi(prob[0]);
        xtra.dev = std::stoi(prob[1]);

        const auto& flags = str_split(tok[2], '|', true);
        for (const auto& f : flags) {
            if (f.size() == 0)
                continue;
            if (grab_ego_generate_flags(xtra, f))
                return PARSE_ERROR_INVALID_FLAG;
        }

        e_ptr->xtra_flags.push_back(std::move(xtra));
    } else
        return PARSE_ERROR_UNDEFINED_DIRECTIVE;

    return PARSE_ERROR_NONE;
}
