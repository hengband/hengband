#pragma once
/*!
 * @file angband-headers.h
 * @brief ゲームデータのグローバルヘッダ情報ヘッダ
 */

#include "system/angband.h"
#include "util/sha256.h"

/*!
 * @brief 各初期データ用ヘッダ構造体 / Template file header information (see "init.c").
 */
struct angband_header {
    util::SHA256::Digest digest; //!< Checksum of "info" records
    uint16_t info_num; //!< このinfoのデータ数
};

extern angband_header artifacts_header;
extern angband_header baseitems_header;
extern angband_header class_magics_header;
extern angband_header class_skills_header;
extern angband_header dungeons_header;
extern angband_header egos_header;
extern angband_header monraces_header;
extern angband_header terrains_header;
extern angband_header vaults_header;
