#include "info-reader/artifact-reader.h"
#include "info-reader/kind-info-tokens-table.h"
#include "main/angband-headers.h"
#include "object-enchant/tr-types.h"
#include "system/artifact-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(アーティファクト用) /
 * Grab one activation index flag
 * @param a_ptr 保管先のアーティファクト構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return エラーがあった場合1、エラーがない場合0を返す
 */
static errr grab_one_artifact_flag(artifact_type *a_ptr, concptr what)
{
    for (int i = 0; i < TR_FLAG_MAX; i++) {
        if (streq(what, k_info_flags[i])) {
            add_flag(a_ptr->flags, i);
            return 0;
        }
    }

    if (grab_one_flag(&a_ptr->gen_flags, k_info_gen_flags, what) == 0)
        return 0;

    msg_format(_("未知の伝説のアイテム・フラグ '%s'。", "Unknown artifact flag '%s'."), what);
    return 1;
}

/*!
 * @brief 固定アーティファクト情報(a_info)のパース関数 /
 * Initialize the "a_info" array, by parsing an ascii "template" file
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_a_info(char *buf, angband_header *head)
{
    static artifact_type *a_ptr = NULL;
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
        a_ptr = &a_info[i];
        add_flag(a_ptr->flags, TR_IGNORE_ACID);
        add_flag(a_ptr->flags, TR_IGNORE_ELEC);
        add_flag(a_ptr->flags, TR_IGNORE_FIRE);
        add_flag(a_ptr->flags, TR_IGNORE_COLD);
#ifdef JP
        if (!add_name(&a_ptr->name, head, s))
            return 7;
#endif
    } else if (!a_ptr) {
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
        s = buf + 2;
        if (!add_name(&a_ptr->name, head, s))
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
        if (!add_text(&a_ptr->text, head, s, TRUE))
            return 7;
    } else if (buf[0] == 'I') {
        int tval, sval, pval;
        if (3 != sscanf(buf + 2, "%d:%d:%d", &tval, &sval, &pval))
            return 1;

        a_ptr->tval = (tval_type)tval;
        a_ptr->sval = (OBJECT_SUBTYPE_VALUE)sval;
        a_ptr->pval = (PARAMETER_VALUE)pval;
    } else if (buf[0] == 'W') {
        int level, rarity, wgt;
        long cost;
        if (4 != sscanf(buf + 2, "%d:%d:%d:%ld", &level, &rarity, &wgt, &cost))
            return 1;

        a_ptr->level = (DEPTH)level;
        a_ptr->rarity = (RARITY)rarity;
        a_ptr->weight = (WEIGHT)wgt;
        a_ptr->cost = (PRICE)cost;
    } else if (buf[0] == 'P') {
        int ac, hd1, hd2, th, td, ta;
        if (6 != sscanf(buf + 2, "%d:%dd%d:%d:%d:%d", &ac, &hd1, &hd2, &th, &td, &ta))
            return 1;

        a_ptr->ac = (ARMOUR_CLASS)ac;
        a_ptr->dd = (DICE_NUMBER)hd1;
        a_ptr->ds = (DICE_SID)hd2;
        a_ptr->to_h = (HIT_PROB)th;
        a_ptr->to_d = (HIT_POINT)td;
        a_ptr->to_a = (ARMOUR_CLASS)ta;
    } else if (buf[0] == 'U') {
        byte n;
        n = grab_one_activation_flag(buf + 2);
        if (n > 0) {
            a_ptr->act_idx = n;
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

            if (0 != grab_one_artifact_flag(a_ptr, s))
                return 5;

            s = t;
        }
    } else {
        return 6;
    }

    return 0;
}
