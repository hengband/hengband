#pragma once

#include "locale/localized-string.h"
#include "system/angband.h"

enum player_sex : byte {
    SEX_FEMALE = 0,
    SEX_MALE = 1,
    MAX_SEXES = 2, /*!< 性別の定義最大数 / Maximum number of player "sex" types (see "table.c", etc) */
};

struct player_sex_type {
    LocalizedString title; /* Type of sex */
    LocalizedString winner; /* Name of winner */
};

extern const player_sex_type sex_info[MAX_SEXES];
extern const player_sex_type *sp_ptr;
