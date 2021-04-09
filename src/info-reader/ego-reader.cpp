#include "info-reader/ego-reader.h"
#include "info-reader/kind-info-tokens-table.h"
#include "main/angband-headers.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
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
static bool grab_one_ego_item_flag(ego_item_type *e_ptr, concptr what)
{
    for (int i = 0; i < TR_FLAG_MAX; i++) {
        if (streq(what, k_info_flags[i])) {
            add_flag(e_ptr->flags, i);
            return 0;
        }
    }

    if (FlagGroup<TRG>::grab_one_flag(e_ptr->gen_flags, k_info_gen_flags, what))
        return 0;

    msg_format(_("未知の名のあるアイテム・フラグ '%s'。", "Unknown ego-item flag '%s'."), what);
    return 1;
}

static bool grab_ego_generate_flags(ego_generate_type &xtra, concptr what)
{
    for (int i = 0; i < TR_FLAG_MAX; i++) {
        if (streq(what, k_info_flags[i])) {
            xtra.tr_flags.push_back(static_cast<tr_type>(i));
            return false;
        }
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
    error_idx = -1;
    error_line = -1;
    char *s, *t;
    if (buf[0] == 'N') {
        s = angband_strchr(buf + 2, ':');
        if (!s)
            return 1;

        *s++ = '\0';
#ifdef JP
        if (!*s)
            return 1;
#endif
        int i = atoi(buf + 2);
        if (i < error_idx)
            return 4;
        if (i >= head->info_num)
            return 2;

        error_idx = i;
        e_ptr = &e_info[i];
#ifdef JP
        e_ptr->name = std::string(s);
#endif
    } else if (!e_ptr) {
        return 3;
    }
#ifdef JP
    /* 英語名を読むルーチンを追加 */
    /* 'E' から始まる行は英語名 */
    else if (buf[0] == 'E') {
        /* nothing to do */
    }
#else
    else if (buf[0] == 'E') {
        s = buf + 2;
        e_ptr->name = std::string(s);
    }
#endif
    else if (buf[0] == 'X') {
        int slot, rating;
        if (2 != sscanf(buf + 2, "%d:%d", &slot, &rating))
            return 1;

        e_ptr->slot = (INVENTORY_IDX)slot;
        e_ptr->rating = (PRICE)rating;
    } else if (buf[0] == 'W') {
        int level, rarity, pad2;
        long cost;

        if (4 != sscanf(buf + 2, "%d:%d:%d:%ld", &level, &rarity, &pad2, &cost))
            return 1;

        e_ptr->level = level;
        e_ptr->rarity = (RARITY)rarity;
        e_ptr->cost = cost;
    } else if (buf[0] == 'C') {
        int th, td, ta, pval;

        if (4 != sscanf(buf + 2, "%d:%d:%d:%d", &th, &td, &ta, &pval))
            return 1;

        e_ptr->max_to_h = (HIT_PROB)th;
        e_ptr->max_to_d = (HIT_POINT)td;
        e_ptr->max_to_a = (ARMOUR_CLASS)ta;
        e_ptr->max_pval = (PARAMETER_VALUE)pval;
    } else if (buf[0] == 'U') {
        byte n;
        n = grab_one_activation_flag(buf + 2);
        if (n > 0) {
            e_ptr->act_idx = n;
        } else {
            return 5;
        }
    } else if (buf[0] == 'F') {
        for (s = buf + 2; *s;) {
            /* loop */
            for (t = s; *t && (*t != ' ') && (*t != '|'); ++t)
                ;

            if (*t) {
                *t++ = '\0';
                while ((*t == ' ') || (*t == '|'))
                    t++;
            }

            if (0 != grab_one_ego_item_flag(e_ptr, s))
                return 5;

            s = t;
        }
    } else if (buf[0] == 'G') {
        ego_generate_type xtra;

        s = angband_strstr(buf + 2, ":");
        if (!s)
            return 1;

        *s++ = '\0';

        if (2 != sscanf(buf + 2, "%d/%d", &xtra.mul, &xtra.dev))
            return 1;

        for (; *s;) {
            for (t = s; *t && (*t != ' ') && (*t != '|'); ++t)
                ;

            if (*t) {
                *t++ = '\0';
                while ((*t == ' ') || (*t == '|'))
                    t++;
            }

            if (grab_ego_generate_flags(xtra, s))
                return 5;

            s = t;
        }

        e_ptr->xtra_flags.push_back(std::move(xtra));
    } else {
        return 6;
    }

    return 0;
}
