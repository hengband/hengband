#include "info-reader/race-reader.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/race-info-tokens-table.h"
#include "main/angband-headers.h"
#include "monster-race/monster-race.h"
#include "term/gameterm.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(モンスター用1) /
 * Grab one (basic) flag in a monster_race from a textual string
 * @param r_ptr 保管先のモンスター種族構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return エラーコード
 */
static errr grab_one_basic_flag(monster_race *r_ptr, concptr what)
{
    if (grab_one_flag(&r_ptr->flags1, r_info_flags1, what) == 0)
        return PARSE_ERROR_NONE;

    if (grab_one_flag(&r_ptr->flags2, r_info_flags2, what) == 0)
        return PARSE_ERROR_NONE;

    if (grab_one_flag(&r_ptr->flags3, r_info_flags3, what) == 0)
        return PARSE_ERROR_NONE;

    if (grab_one_flag(&r_ptr->flags7, r_info_flags7, what) == 0)
        return PARSE_ERROR_NONE;

    if (grab_one_flag(&r_ptr->flags8, r_info_flags8, what) == 0)
        return PARSE_ERROR_NONE;

    if (grab_one_flag(&r_ptr->flags9, r_info_flags9, what) == 0)
        return PARSE_ERROR_NONE;

    if (grab_one_flag(&r_ptr->flagsr, r_info_flagsr, what) == 0)
        return PARSE_ERROR_NONE;

    msg_format(_("未知のモンスター・フラグ '%s'。", "Unknown monster flag '%s'."), what);
    return PARSE_ERROR_GENERIC;
}

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(モンスター用2) /
 * Grab one (spell) flag in a monster_race from a textual string
 * @param r_ptr 保管先のモンスター種族構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return エラーコード
 */
static errr grab_one_spell_flag(monster_race *r_ptr, concptr what)
{
    if (grab_one_flag(&r_ptr->flags4, r_info_flags4, what) == 0)
        return PARSE_ERROR_NONE;

    if (grab_one_flag(&r_ptr->a_ability_flags1, r_a_ability_flags1, what) == 0)
        return PARSE_ERROR_NONE;

    if (grab_one_flag(&r_ptr->a_ability_flags2, r_a_ability_flags2, what) == 0)
        return PARSE_ERROR_NONE;

    msg_format(_("未知のモンスター・フラグ '%s'。", "Unknown monster flag '%s'."), what);
    return PARSE_ERROR_GENERIC;
}

/*!
 * @brief モンスター種族情報(r_info)のパース関数 /
 * Initialize the "r_info" array, by parsing an ascii "template" file
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_r_info(char *buf, angband_header *head)
{
    static monster_race *r_ptr = NULL;
    char *s, *t;
    if (buf[0] == 'N') {
        s = angband_strchr(buf + 2, ':');
        if (!s)
            return PARSE_ERROR_GENERIC;

        *s++ = '\0';
#ifdef JP
        if (!*s)
            return PARSE_ERROR_GENERIC;
#endif

        int i = atoi(buf + 2);
        if (i < error_idx)
            return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;

        if (i >= head->info_num)
            return PARSE_ERROR_ABSOLETE_FILE;

        error_idx = i;
        r_ptr = &r_info[i];
#ifdef JP
        if (!add_name(&r_ptr->name, head, s))
            return PARSE_ERROR_OUT_OF_MEMORY;
#endif
    } else if (!r_ptr) {
        return PARSE_ERROR_MISSING_RECORD_HEADER;
    }
#ifdef JP
    /* 英語名を読むルーチンを追加 */
    /* 'E' から始まる行は英語名 */
    else if (buf[0] == 'E') {
        s = buf + 2;
        if (!add_name(&r_ptr->E_name, head, s))
            return PARSE_ERROR_OUT_OF_MEMORY;
    }
#else
    else if (buf[0] == 'E') {
        s = buf + 2;
        if (!add_name(&r_ptr->name, head, s))
            return PARSE_ERROR_OUT_OF_MEMORY;
    }
#endif
    else if (buf[0] == 'D') {
#ifdef JP
        if (buf[2] == '$')
            return PARSE_ERROR_NONE;

        s = buf + 2;
#else
        if (buf[2] != '$')
            return PARSE_ERROR_NONE;
        s = buf + 3;
#endif
        if (!add_text(&r_ptr->text, head, s, TRUE))
            return PARSE_ERROR_OUT_OF_MEMORY;
    } else if (buf[0] == 'G') {
        if ((buf[1] != ':') || !buf[2] || (buf[3] != ':') || !buf[4])
            return PARSE_ERROR_GENERIC;

        char sym = buf[2];
        byte tmp = color_char_to_attr(buf[4]);
        if (tmp > 127)
            return PARSE_ERROR_GENERIC;

        r_ptr->d_char = sym;
        r_ptr->d_attr = tmp;
    } else if (buf[0] == 'I') {
        int spd, hp1, hp2, aaf, ac, slp;
        if (sscanf(buf + 2, "%d:%dd%d:%d:%d:%d", &spd, &hp1, &hp2, &aaf, &ac, &slp) != 6)
            return PARSE_ERROR_GENERIC;

        r_ptr->speed = (SPEED)spd;
        r_ptr->hdice = (DICE_NUMBER)MAX(hp1, 1);
        r_ptr->hside = (DICE_SID)MAX(hp2, 1);
        r_ptr->aaf = (POSITION)aaf;
        r_ptr->ac = (ARMOUR_CLASS)ac;
        r_ptr->sleep = (SLEEP_DEGREE)slp;
    } else if (buf[0] == 'W') {
        int lev, rar, pad;
        long exp;
        long nextexp;
        int nextmon;
        if (sscanf(buf + 2, "%d:%d:%d:%ld:%ld:%d", &lev, &rar, &pad, &exp, &nextexp, &nextmon) != 6)
            return PARSE_ERROR_GENERIC;

        r_ptr->level = (DEPTH)lev;
        r_ptr->rarity = (RARITY)rar;
        r_ptr->extra = (BIT_FLAGS16)pad;
        r_ptr->mexp = (EXP)exp;
        r_ptr->next_exp = (EXP)nextexp;
        r_ptr->next_r_idx = (MONRACE_IDX)nextmon;
    } else if (buf[0] == 'R') {
        int id, ds, dd;
        int i = 0;
        for (; i < A_MAX; i++)
            if (r_ptr->reinforce_id[i] == 0)
                break;

        if ((i == 6) || (sscanf(buf + 2, "%d:%dd%d", &id, &dd, &ds) != 3))
            return PARSE_ERROR_GENERIC;

        r_ptr->reinforce_id[i] = (MONRACE_IDX)id;
        r_ptr->reinforce_dd[i] = (DICE_NUMBER)dd;
        r_ptr->reinforce_ds[i] = (DICE_SID)ds;
    } else if (buf[0] == 'B') {
        int n1, n2;
        int i = 0;
        for (i = 0; i < 4; i++)
            if (!r_ptr->blow[i].method)
                break;

        if (i == 4)
            return PARSE_ERROR_GENERIC;

        /* loop */
        for (s = t = buf + 2; *t && (*t != ':'); t++)
            ;

        if (*t == ':')
            *t++ = '\0';

        for (n1 = 0; r_info_blow_method[n1]; n1++) {
            if (streq(s, r_info_blow_method[n1]))
                break;
        }

        if (!r_info_blow_method[n1])
            return PARSE_ERROR_GENERIC;

        /* loop */
        for (s = t; *t && (*t != ':'); t++)
            ;

        if (*t == ':')
            *t++ = '\0';

        for (n2 = 0; r_info_blow_effect[n2]; n2++) {
            if (streq(s, r_info_blow_effect[n2]))
                break;
        }

        if (!r_info_blow_effect[n2])
            return PARSE_ERROR_GENERIC;

        /* loop */
        for (s = t; *t && (*t != 'd'); t++)
            ;

        if (*t == 'd')
            *t++ = '\0';

        r_ptr->blow[i].method = (rbm_type)n1;
        r_ptr->blow[i].effect = (rbe_type)n2;
        r_ptr->blow[i].d_dice = atoi(s);
        r_ptr->blow[i].d_side = atoi(t);
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

            if (0 != grab_one_basic_flag(r_ptr, s))
                return PARSE_ERROR_INVALID_FLAG;

            s = t;
        }
    } else if (buf[0] == 'S') {
        for (s = buf + 2; *s;) {

            /* loop */
            for (t = s; *t && (*t != ' ') && (*t != '|'); ++t)
                ;

            if (*t) {
                *t++ = '\0';
                while ((*t == ' ') || (*t == '|'))
                    t++;
            }

            int i;
            if (1 == sscanf(s, "1_IN_%d", &i)) {
                r_ptr->freq_spell = 100 / i;
                s = t;
                continue;
            }

            if (grab_one_spell_flag(r_ptr, s) != PARSE_ERROR_NONE)
                return PARSE_ERROR_INVALID_FLAG;

            s = t;
        }
    } else if (buf[0] == 'A') {
        int id, per, rarity;
        int i = 0;
        for (i = 0; i < 4; i++)
            if (!r_ptr->artifact_id[i])
                break;

        if ((i == 4) || (sscanf(buf + 2, "%d:%d:%d", &id, &rarity, &per) != 3))
            return PARSE_ERROR_GENERIC;

        r_ptr->artifact_id[i] = (ARTIFACT_IDX)id;
        r_ptr->artifact_rarity[i] = (RARITY)rarity;
        r_ptr->artifact_percent[i] = (PERCENTAGE)per;
    } else if (buf[0] == 'V') {
        int val;
        if (sscanf(buf + 2, "%d", &val) != 3)
            return 1;

        r_ptr->arena_ratio = (PERCENTAGE)val;
    } else {
        return PARSE_ERROR_UNDEFINED_DIRECTIVE;
    }

    return PARSE_ERROR_NONE;
}
