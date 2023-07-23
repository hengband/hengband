#pragma once

#include "system/angband.h"
#include <vector>

/*!
 * ベースアイテムの大項目IDの種別名をまとめる構造体 / A structure to hold a tval and its description
 */
enum class ItemKindType : short;
struct tval_desc {
    ItemKindType tval; /*!< 大項目のID */
    concptr desc; /*!< 大項目名 */
};

extern const std::vector<tval_desc> tvals;
extern const std::vector<char> listsym;
