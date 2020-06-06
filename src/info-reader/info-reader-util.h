#pragma once

#include "system/angband.h"

/*
 * Size of memory reserved for initialization of some arrays
 */
#define FAKE_NAME_SIZE 40 * 1024L /*!< ゲーム情報の種別毎に用意される名前用バッファの容量 */
#define FAKE_TEXT_SIZE 150 * 1024L /*!< ゲーム情報の種別毎に用意されるテキスト用バッファの容量 */
#define FAKE_TAG_SIZE 10 * 1024L /*!< ゲーム情報の種別毎に用意されるタグ用バッファの容量 */

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

extern int error_idx;
extern int error_line;

bool add_text(u32b *offset, angband_header *head, concptr buf, bool normal_text);
bool add_name(u32b *offset, angband_header *head, concptr buf);
bool add_tag(STR_OFFSET *offset, angband_header *head, concptr buf);
