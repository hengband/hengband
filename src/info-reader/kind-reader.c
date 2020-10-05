#include "info-reader/kind-reader.h"
#include "info-reader/kind-info-tokens-table.h"
#include "main/angband-headers.h"
#include "object-enchant/tr-types.h"
#include "object/object-kind.h"
#include "term/gameterm.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(ベースアイテム用) /
 * Grab one flag in an object_kind from a textual string
 * @param k_ptr 保管先のベースアイテム構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return エラーコード
 */
static errr grab_one_kind_flag(object_kind *k_ptr, concptr what)
{
    for (int i = 0; i < TR_FLAG_MAX; i++) {
        if (streq(what, k_info_flags[i])) {
            add_flag(k_ptr->flags, i);
            return 0;
        }
    }

    if (grab_one_flag(&k_ptr->gen_flags, k_info_gen_flags, what) == 0)
        return 0;

    msg_format(_("未知のアイテム・フラグ '%s'。", "Unknown object flag '%s'."), what);
    return 1;
}

/*!
 * @brief ベースアイテム(k_info)のパース関数 /
 * Initialize the "k_info" array, by parsing an ascii "template" file
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_k_info(char *buf, angband_header *head)
{
    static object_kind *k_ptr = NULL;

    char *s, *t;
    if (buf[0] == 'N') {
#ifdef JP
        char *flavor;
#endif
        s = angband_strchr(buf + 2, ':');
        if (!s)
            return 1;

        *s++ = '\0';
        int i = atoi(buf + 2);

        if (i <= error_idx)
            return 4;
        if (i >= head->info_num)
            return 2;

        error_idx = i;
        k_ptr = &k_info[i];

#ifdef JP
        if (!*s)
            return 1;

        flavor = angband_strchr(s, ':');
        if (flavor) {
            *flavor++ = '\0';
            if (!add_name(&k_ptr->flavor_name, head, flavor))
                return 7;
        }

        if (!add_name(&k_ptr->name, head, s))
            return 7;
#endif
    } else if (!k_ptr) {
        return 3;
    }
#ifdef JP
    /* 英語名を読むルーチンを追加 */
    /* 'E' から始まる行は英語名としている */
    else if (buf[0] == 'E') {
        /* nothing to do */
    }
#else
    else if (buf[0] == 'E') {
        char *flavor;
        s = buf + 2;
        flavor = angband_strchr(s, ':');
        if (flavor) {
            *flavor++ = '\0';
            if (!add_name(&k_ptr->flavor_name, head, flavor))
                return 7;
        }

        if (!add_name(&k_ptr->name, head, s))
            return 7;
    }
#endif
    else if (buf[0] == 'D') {
#ifdef JP
        if (buf[2] == '$')
            return 0;
        s = buf + 2;
#else
        if (buf[2] != '$')
            return 0;
        s = buf + 3;
#endif
        if (!add_text(&k_ptr->text, head, s, TRUE))
            return 7;
    } else if (buf[0] == 'G') {
        char sym;
        byte tmp;
        if (buf[1] != ':')
            return 1;
        if (!buf[2])
            return 1;
        if (buf[3] != ':')
            return 1;
        if (!buf[4])
            return 1;

        sym = buf[2];
        tmp = color_char_to_attr(buf[4]);
        if (tmp > 127)
            return 1;

        k_ptr->d_attr = tmp;
        k_ptr->d_char = sym;
    } else if (buf[0] == 'I') {
        int tval, sval, pval;
        if (3 != sscanf(buf + 2, "%d:%d:%d", &tval, &sval, &pval))
            return 1;

        k_ptr->tval = (tval_type)tval;
        k_ptr->sval = (OBJECT_SUBTYPE_VALUE)sval;
        k_ptr->pval = (PARAMETER_VALUE)pval;
    } else if (buf[0] == 'W') {
        int level, extra, wgt;
        long cost;
        if (4 != sscanf(buf + 2, "%d:%d:%d:%ld", &level, &extra, &wgt, &cost))
            return 1;

        k_ptr->level = (DEPTH)level;
        k_ptr->extra = (BIT_FLAGS8)extra;
        k_ptr->weight = (WEIGHT)wgt;
        k_ptr->cost = (PRICE)cost;
    } else if (buf[0] == 'A') {
        int i = 0;
        for (s = buf + 1; s && (s[0] == ':') && s[1]; ++i) {
            k_ptr->chance[i] = 1;
            k_ptr->locale[i] = atoi(s + 1);
            t = angband_strchr(s + 1, '/');
            s = angband_strchr(s + 1, ':');
            if (t && (!s || t < s)) {
                int chance = atoi(t + 1);
                if (chance > 0)
                    k_ptr->chance[i] = (PROB)chance;
            }
        }
    } else if (buf[0] == 'P') {
        int ac, hd1, hd2, th, td, ta;
        if (6 != sscanf(buf + 2, "%d:%dd%d:%d:%d:%d", &ac, &hd1, &hd2, &th, &td, &ta))
            return 1;

        k_ptr->ac = (ARMOUR_CLASS)ac;
        k_ptr->dd = (DICE_NUMBER)hd1;
        k_ptr->ds = (DICE_SID)hd2;
        k_ptr->to_h = (HIT_PROB)th;
        k_ptr->to_d = (HIT_POINT)td;
        k_ptr->to_a = (ARMOUR_CLASS)ta;
    } else if (buf[0] == 'U') {
        byte n;
        n = grab_one_activation_flag(buf + 2);
        if (n > 0) {
            k_ptr->act_idx = n;
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
                while (*t == ' ' || *t == '|')
                    t++;
            }

            if (0 != grab_one_kind_flag(k_ptr, s))
                return 5;
            s = t;
        }
    } else {
        return 6;
    }

    return 0;
}
