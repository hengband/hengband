#pragma once

#include "system/angband.h"

/*!
 * @brief 各初期データ用ヘッダ構造体 / Template file header information (see "init.c").
 */
typedef struct angband_header angband_header;
typedef errr (*parse_info_txt_func)(char *buf, angband_header *head);

struct angband_header {
    byte v_major; /* Version -- major */
    byte v_minor; /* Version -- minor */
    byte v_patch; /* Version -- patch */
    byte v_extra; /* Version -- extra */

    u16b info_num; /* Number of "info" records */
    int info_len; /* Size of each "info" record */
    u16b head_size; /* Size of the "header" in bytes */

    STR_OFFSET info_size; /* Size of the "info" array in bytes */
    STR_OFFSET name_size; /* Size of the "name" array in bytes */
    STR_OFFSET text_size; /* Size of the "text" array in bytes */
    STR_OFFSET tag_size; /* Size of the "tag" array in bytes */

    void *info_ptr;
    char *name_ptr;
    char *text_ptr;
    char *tag_ptr;

    parse_info_txt_func parse_info_txt;

    void (*retouch)(angband_header *head);
};

extern angband_header f_head;
extern angband_header v_head;
extern angband_header k_head;
extern angband_header a_head;
extern angband_header e_head;
extern angband_header r_head;
extern angband_header d_head;
extern angband_header s_head;
extern angband_header m_head;
