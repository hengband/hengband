#include "info-reader/ego-reader.h"
#include "info-reader/kind-info-tokens-table.h"
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

    if (grab_one_flag(&e_ptr->gen_flags, k_info_gen_flags, what) == 0)
        return 0;

    msg_format(_("未知の名のあるアイテム・フラグ '%s'。", "Unknown ego-item flag '%s'."), what);
    return 1;
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
        if (!add_name(&e_ptr->name, head, s))
            return 7;
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
        if (!add_name(&e_ptr->name, head, s))
            return 7;
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
    } else {
        return 6;
    }

    return 0;
}
