#pragma once
/*!
 * @file angband-headers.h
 * @brief ゲームデータのグローバルヘッダ情報ヘッダ
 */

#include "system/angband.h"

/*!
 * @brief 各初期データ用ヘッダ構造体 / Template file header information (see "init.c").
 */
struct angband_header {
    byte checksum; //!< Checksum of "info" records
    uint16_t info_num; //!< このinfoのデータ数
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
