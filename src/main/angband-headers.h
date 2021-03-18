#pragma once

#include "system/angband.h"

/*!
 * @brief 各初期データ用ヘッダ構造体 / Template file header information (see "init.c").
 */
typedef struct angband_header angband_header;
typedef errr (*parse_info_txt_func)(char *buf, angband_header *head);

struct angband_header {
    byte v_major; //!< Major version
    byte v_minor; //!< Minor version
    byte v_patch; //!< Patch version
    byte checksum; //!< Checksum of "info" records

    u16b info_num; //!< このinfoのデータ数
    int info_len; //!< このinfoの総サイズ
    u16b head_size; //!< このinfoのヘッダサイズ

    STR_OFFSET info_size; //!< info配列サイズ
    STR_OFFSET name_size; //!< 名前文字列群サイズ(総文字長)
    STR_OFFSET text_size; //!< フレーバー文字列群サイズ(総文字長)
    STR_OFFSET tag_size; //!< タグ文字列群サイズ(総文字長)

    void *info_ptr; //!< info配列へのポインタ
    char *name_ptr; //!< 名前文字列群へのポインタ
    char *text_ptr; //!< フレーバー文字列群へのポインタ
    char *tag_ptr; //!< タグ文字列群へのポインタ

    parse_info_txt_func parse_info_txt; //!< Pointer to parser callback function

    void (*retouch)(angband_header *head); //!< 設定再読み込み用？

    byte v_extra; ///< Extra version for Alpha, Beta
    byte v_savefile; ///< Savefile version
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
