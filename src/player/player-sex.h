#pragma once

#include "system/angband.h"

enum player_sex : byte {
    SEX_FEMALE = 0,
    SEX_MALE = 1,
    MAX_SEXES = 2, /*!< 性別の定義最大数 / Maximum number of player "sex" types (see "table.c", etc) */
};

struct player_sex_type {
    concptr title; /* Type of sex */
    concptr winner; /* Name of winner */
#ifdef JP
    concptr E_title; /* 英語性別 */
    concptr E_winner; /* 英語性別 */
#endif
};

extern const player_sex_type sex_info[MAX_SEXES];
extern const player_sex_type *sp_ptr;
