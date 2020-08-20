#include "info-reader/skill-reader.h"
#include "main/angband-headers.h"
#include "player/player-skill.h"

/*!
 * @brief 職業技能情報(s_info)のパース関数 /
 * Initialize the "s_info" array, by parsing an ascii "template" file
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_s_info(char *buf, angband_header *head)
{
    static skill_table *s_ptr = NULL;
    if (buf[0] == 'N') {
        int i = atoi(buf + 2);
        if (i <= error_idx)
            return 4;
        if (i >= head->info_num)
            return 2;

        error_idx = i;
        s_ptr = &s_info[i];
    } else if (!s_ptr) {
        return 3;
    } else if (buf[0] == 'W') {
        int tval, sval, start, max;
        const s16b exp_conv_table[] = { WEAPON_EXP_UNSKILLED, WEAPON_EXP_BEGINNER, WEAPON_EXP_SKILLED, WEAPON_EXP_EXPERT, WEAPON_EXP_MASTER };

        if (4 != sscanf(buf + 2, "%d:%d:%d:%d", &tval, &sval, &start, &max))
            return 1;

        if (start < EXP_LEVEL_UNSKILLED || start > EXP_LEVEL_MASTER || max < EXP_LEVEL_UNSKILLED || max > EXP_LEVEL_MASTER)
            return 8;

        s_ptr->w_start[tval][sval] = exp_conv_table[start];
        s_ptr->w_max[tval][sval] = exp_conv_table[max];
    } else if (buf[0] == 'S') {
        int num, start, max;
        if (3 != sscanf(buf + 2, "%d:%d:%d", &num, &start, &max))
            return 1;

        if (start < WEAPON_EXP_UNSKILLED || start > WEAPON_EXP_MASTER || max < WEAPON_EXP_UNSKILLED || max > WEAPON_EXP_MASTER)
            return 8;

        s_ptr->s_start[num] = (SUB_EXP)start;
        s_ptr->s_max[num] = (SUB_EXP)max;
    } else
        return 6;

    return 0;
}
