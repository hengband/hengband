#include "info-reader/ego-reader.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/kind-info-tokens-table.h"
#include "info-reader/parse-error-types.h"
#include "main/angband-headers.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(エゴ用) /
 * Grab one flag in a ego-item_type from a textual string
 * @param e_ptr 保管先のエゴ構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return 見つけたらtrue
 */
static bool grab_one_ego_item_flag(ego_item_type *e_ptr, std::string_view what)
{
    if (info_grab_one_flag(e_ptr->flags, k_info_flags, what))
        return true;

    if (EnumClassFlagGroup<TRG>::grab_one_flag(e_ptr->gen_flags, k_info_gen_flags, what))
        return true;

    msg_format(_("未知の名のあるアイテム・フラグ '%s'。", "Unknown ego-item flag '%s'."), what.data());
    return false;
}

/*!
 * @brief テキストトークンを走査して生成フラグを一つ得る(エゴ用) /
 * Grab one genetation flag in a ego-item_type from a textual string
 * @param e_ptr 保管先のエゴ構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return 見つけたらtrue
 */
static bool grab_ego_generate_flags(ego_generate_type &xtra, std::string_view what)
{
    if (auto it = k_info_flags.find(what); it != k_info_flags.end()) {
        xtra.tr_flags.push_back(it->second);
        return true;
    }

    if (auto it = k_info_gen_flags.find(what); it != k_info_gen_flags.end()) {
        xtra.trg_flags.push_back(it->second);
        return true;
    }

    return false;
}

/*!
 * @brief アイテムエゴ情報(e_info)のパース関数 /
 * Initialize the "e_info" array, by parsing an ascii "template" file
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_e_info(std::string_view buf, angband_header *head)
{
    static ego_item_type *e_ptr = nullptr;
    const auto &tokens = str_split(buf, ':', false, 10);

    error_idx = 0; //!< @note 順不同で登録しているため

    if (tokens[0] == "N") {
        // N:index:name_ja
        if (tokens.size() < 3 || tokens[1].size() == 0)
            return PARSE_ERROR_GENERIC;

        auto i = std::stoi(tokens[1]);
        if (i < error_idx)
            return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
        if (i >= head->info_num)
            return PARSE_ERROR_OUT_OF_BOUNDS;

        error_idx = i;
        e_ptr = &e_info[i];
#ifdef JP
        e_ptr->name = tokens[2];
#endif
    } else if (!e_ptr)
        return PARSE_ERROR_MISSING_RECORD_HEADER;
    else if (tokens[0] == "E") {
        // E:name_en
#ifndef JP
        if (tokens[1].size() == 0)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        e_ptr->name = tokens[1];
#endif
    } else if (tokens[0] == "X") {
        // X:slot:rating
        if (tokens.size() < 3)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;

        info_set_value(e_ptr->slot, tokens[1]);
        info_set_value(e_ptr->rating, tokens[2]);
    } else if (tokens[0] == "W") {
        // W:level:ratiry:xtra:cost
        // xtra is not used
        if (tokens.size() < 5)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;

        info_set_value(e_ptr->level, tokens[1]);
        info_set_value(e_ptr->rarity, tokens[2]);
        info_set_value(e_ptr->cost, tokens[4]);
    } else if (tokens[0] == "B") {
        // Base bonuses
        // B:to_hit:to_dam:to_ac
        if (tokens.size() < 4)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;

        info_set_value(e_ptr->base_to_h, tokens[1]);
        info_set_value(e_ptr->base_to_d, tokens[2]);
        info_set_value(e_ptr->base_to_a, tokens[3]);
    } else if (tokens[0] == "C") {
        // Creation bonuses (random plus)
        // C:to_hit:to_dam:to_ac:pval
        if (tokens.size() < 5)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;

        info_set_value(e_ptr->max_to_h, tokens[1]);
        info_set_value(e_ptr->max_to_d, tokens[2]);
        info_set_value(e_ptr->max_to_a, tokens[3]);
        info_set_value(e_ptr->max_pval, tokens[4]);
    } else if (tokens[0] == "U") {
        // U:activation_flag
        if (tokens.size() < 2 || tokens[1].size() == 0)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;

        auto n = grab_one_activation_flag(tokens[1].c_str());
        if (n <= 0)
            return PARSE_ERROR_INVALID_FLAG;

        e_ptr->act_idx = (IDX)n;
    } else if (tokens[0] == "F") {
        // F:flags
        if (tokens.size() < 2 || tokens[1].size() == 0)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;

        const auto& flags = str_split(tokens[1], '|', true, 10);
        for (const auto& f : flags) {
            if (f.size() == 0)
                continue;
            if (!grab_one_ego_item_flag(e_ptr, f))
                return PARSE_ERROR_INVALID_FLAG;
        }
    } else if (tokens[0] == "G") {
        // G:mul/dev:flags
        if (tokens.size() < 3)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;

        const auto &prob = str_split(tokens[1], '/', false, 2);
        if (prob.size() != 2 || tokens[1].size() == 0 || tokens[2].size() == 0)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;

        ego_generate_type xtra;
        xtra.mul = std::stoi(prob[0]);
        xtra.dev = std::stoi(prob[1]);

        const auto& flags = str_split(tokens[2], '|', true, 10);
        for (const auto& f : flags) {
            if (f.size() == 0)
                continue;
            if (!grab_ego_generate_flags(xtra, f))
                return PARSE_ERROR_INVALID_FLAG;
        }

        e_ptr->xtra_flags.push_back(std::move(xtra));
    } else
        return PARSE_ERROR_UNDEFINED_DIRECTIVE;

    return PARSE_ERROR_NONE;
}
