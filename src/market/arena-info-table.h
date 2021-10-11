#pragma once

#include "object/tval-types.h"
#include "system/angband.h"
#include <vector>

/*!
 * @brief 闘技場のモンスターエントリー構造体 / A structure type for on_defeat_arena_monster entry
 */
struct arena_type {
    MONRACE_IDX r_idx; /*!< 闘技場のモンスター種族ID(0ならば表彰式) / Monster (0 means victory prizing) */
    tval_type tval; /*!< モンスター打倒後に得られるアイテムの大カテゴリID / tval of prize (0 means no prize) */
    OBJECT_SUBTYPE_VALUE sval; /*!< モンスター打倒後に得られるアイテムの小カテゴリID / sval of prize */
};

extern const std::vector<arena_type> arena_info;
extern const int MAX_ARENA_MONS; // 表彰式までの最大数.
