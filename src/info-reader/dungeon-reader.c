#include "info-reader/dungeon-reader.h"
#include "dungeon/dungeon.h"
#include "info-reader/dungeon-info-tokens-table.h"
#include "info-reader/feature-reader.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/race-info-tokens-table.h"
#include "io/tokenizer.h"
#include "main/angband-headers.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(ダンジョン用) /
 * Grab one flag for a dungeon type from a textual string
 * @param d_ptr 保管先のダンジョン構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return エラーコード
 */
static errr grab_one_dungeon_flag(dungeon_type *d_ptr, concptr what)
{
    if (grab_one_flag(&d_ptr->flags1, d_info_flags1, what) == 0)
        return 0;

    msg_format(_("未知のダンジョン・フラグ '%s'。", "Unknown dungeon type flag '%s'."), what);
    return 1;
}

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(モンスターのダンジョン出現条件用1) /
 * Grab one (basic) flag in a monster_race from a textual string
 * @param d_ptr 保管先のダンジョン構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return エラーコード
 */
static errr grab_one_basic_monster_flag(dungeon_type *d_ptr, concptr what)
{
    if (grab_one_flag(&d_ptr->mflags1, r_info_flags1, what) == 0)
        return 0;

    if (grab_one_flag(&d_ptr->mflags2, r_info_flags2, what) == 0)
        return 0;

    if (grab_one_flag(&d_ptr->mflags3, r_info_flags3, what) == 0)
        return 0;

    if (grab_one_flag(&d_ptr->mflags7, r_info_flags7, what) == 0)
        return 0;

    if (grab_one_flag(&d_ptr->mflags8, r_info_flags8, what) == 0)
        return 0;

    if (grab_one_flag(&d_ptr->mflags9, r_info_flags9, what) == 0)
        return 0;

    if (grab_one_flag(&d_ptr->mflagsr, r_info_flagsr, what) == 0)
        return 0;

    msg_format(_("未知のモンスター・フラグ '%s'。", "Unknown monster flag '%s'."), what);
    return 1;
}

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(モンスターのダンジョン出現条件用2) /
 * Grab one (spell) flag in a monster_race from a textual string
 * @param d_ptr 保管先のダンジョン構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return エラーコード
 */
static errr grab_one_spell_monster_flag(dungeon_type *d_ptr, concptr what)
{
    if (grab_one_flag(&d_ptr->mflags4, r_info_flags4, what) == 0)
        return 0;

    if (grab_one_flag(&d_ptr->m_a_ability_flags1, r_a_ability_flags1, what) == 0)
        return 0;

    if (grab_one_flag(&d_ptr->m_a_ability_flags2, r_a_ability_flags2, what) == 0)
        return 0;

    msg_format(_("未知のモンスター・フラグ '%s'。", "Unknown monster flag '%s'."), what);
    return 1;
}

/*!
 * @brief ダンジョン情報(d_info)のパース関数 /
 * Initialize the "d_info" array, by parsing an ascii "template" file
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_d_info(char *buf, angband_header *head)
{
    static dungeon_type *d_ptr = NULL;
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
        d_ptr = &d_info[i];
#ifdef JP
        if (!add_name(&d_ptr->name, head, s))
            return 7;
#endif
    }
#ifdef JP
    else if (buf[0] == 'E')
        return 0;
#else
    else if (buf[0] == 'E') {
        /* Acquire the Text */
        s = buf + 2;

        /* Store the name */
        if (!add_name(&d_ptr->name, head, s))
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
        if (!add_text(&d_ptr->text, head, s, TRUE))
            return 7;
    } else if (buf[0] == 'W') {
        int min_lev, max_lev;
        int min_plev, mode;
        int min_alloc, max_chance;
        int obj_good, obj_great;
        int pit, nest;

        if (10
            != sscanf(buf + 2, "%d:%d:%d:%d:%d:%d:%d:%d:%x:%x", &min_lev, &max_lev, &min_plev, &mode, &min_alloc, &max_chance, &obj_good, &obj_great,
                (unsigned int *)&pit, (unsigned int *)&nest))
            return 1;

        d_ptr->mindepth = (DEPTH)min_lev;
        d_ptr->maxdepth = (DEPTH)max_lev;
        d_ptr->min_plev = (PLAYER_LEVEL)min_plev;
        d_ptr->mode = (BIT_FLAGS8)mode;
        d_ptr->min_m_alloc_level = min_alloc;
        d_ptr->max_m_alloc_chance = max_chance;
        d_ptr->obj_good = obj_good;
        d_ptr->obj_great = obj_great;
        d_ptr->pit = (BIT_FLAGS16)pit;
        d_ptr->nest = (BIT_FLAGS16)nest;
    } else if (buf[0] == 'P') {
        int dy, dx;
        if (2 != sscanf(buf + 2, "%d:%d", &dy, &dx))
            return 1;

        d_ptr->dy = dy;
        d_ptr->dx = dx;
    } else if (buf[0] == 'L') {
        char *zz[16];
        if (tokenize(buf + 2, DUNGEON_FEAT_PROB_NUM * 2 + 1, zz, 0) != (DUNGEON_FEAT_PROB_NUM * 2 + 1))
            return 1;

        for (int i = 0; i < DUNGEON_FEAT_PROB_NUM; i++) {
            d_ptr->floor[i].feat = f_tag_to_index(zz[i * 2]);
            if (d_ptr->floor[i].feat < 0)
                return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;

            d_ptr->floor[i].percent = (PERCENTAGE)atoi(zz[i * 2 + 1]);
        }

        d_ptr->tunnel_percent = atoi(zz[DUNGEON_FEAT_PROB_NUM * 2]);
    } else if (buf[0] == 'A') {
        char *zz[16];
        if (tokenize(buf + 2, DUNGEON_FEAT_PROB_NUM * 2 + 4, zz, 0) != (DUNGEON_FEAT_PROB_NUM * 2 + 4))
            return 1;

        for (int i = 0; i < DUNGEON_FEAT_PROB_NUM; i++) {
            d_ptr->fill[i].feat = f_tag_to_index(zz[i * 2]);
            if (d_ptr->fill[i].feat < 0)
                return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;

            d_ptr->fill[i].percent = (PERCENTAGE)atoi(zz[i * 2 + 1]);
        }

        d_ptr->outer_wall = f_tag_to_index(zz[DUNGEON_FEAT_PROB_NUM * 2]);
        if (d_ptr->outer_wall < 0)
            return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;

        d_ptr->inner_wall = f_tag_to_index(zz[DUNGEON_FEAT_PROB_NUM * 2 + 1]);
        if (d_ptr->inner_wall < 0)
            return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;

        d_ptr->stream1 = f_tag_to_index(zz[DUNGEON_FEAT_PROB_NUM * 2 + 2]);
        if (d_ptr->stream1 < 0)
            return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;

        d_ptr->stream2 = f_tag_to_index(zz[DUNGEON_FEAT_PROB_NUM * 2 + 3]);
        if (d_ptr->stream2 < 0)
            return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
    } else if (buf[0] == 'F') {
        int artif = 0, monst = 0;

        for (s = buf + 2; *s;) {
            /* loop */
            for (t = s; *t && (*t != ' ') && (*t != '|'); ++t)
                ;

            if (*t) {
                *t++ = '\0';
                while (*t == ' ' || *t == '|')
                    t++;
            }

            if (1 == sscanf(s, "FINAL_ARTIFACT_%d", &artif)) {
                d_ptr->final_artifact = (ARTIFACT_IDX)artif;
                s = t;
                continue;
            }

            if (1 == sscanf(s, "FINAL_OBJECT_%d", &artif)) {
                d_ptr->final_object = (KIND_OBJECT_IDX)artif;
                s = t;
                continue;
            }

            if (1 == sscanf(s, "FINAL_GUARDIAN_%d", &monst)) {
                d_ptr->final_guardian = (MONRACE_IDX)monst;
                s = t;
                continue;
            }

            if (1 == sscanf(s, "MONSTER_DIV_%d", &monst)) {
                d_ptr->special_div = (PROB)monst;
                s = t;
                continue;
            }

            if (0 != grab_one_dungeon_flag(d_ptr, s))
                return 5;

            s = t;
        }
    } else if (buf[0] == 'M') {
        for (s = buf + 2; *s;) {
            /* loop */
            for (t = s; *t && (*t != ' ') && (*t != '|'); ++t)
                ;

            if (*t) {
                *t++ = '\0';
                while (*t == ' ' || *t == '|')
                    t++;
            }

            if (!strncmp(s, "R_CHAR_", 7)) {
                s += 7;
                strncpy(d_ptr->r_char, s, sizeof(d_ptr->r_char));
                s = t;
                continue;
            }

            if (0 != grab_one_basic_monster_flag(d_ptr, s))
                return 5;

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
                s = t;
                continue;
            }

            if (0 != grab_one_spell_monster_flag(d_ptr, s))
                return 5;

            s = t;
        }
    } else {
        return 6;
    }

    return 0;
}
