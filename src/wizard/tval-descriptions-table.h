#pragma once

#include "system/angband.h"

/*!
 * ベースアイテムの大項目IDの種別名をまとめる構造体 / A structure to hold a tval and its description
 */
typedef struct tval_desc {
    int tval; /*!< 大項目のID */
    concptr desc; /*!< 大項目名 */
} tval_desc;

#define MAX_TVAL_DESCRIPTIONS 52
#define MAX_DEBUG_COMMAND_SYMBOLS 63

extern tval_desc tvals[MAX_TVAL_DESCRIPTIONS];
extern const char listsym[MAX_DEBUG_COMMAND_SYMBOLS];
