#pragma once

#include "system/angband.h"
#include "object/object-index-list.h"

enum flow_type {
    FLOW_NORMAL = 0,
    FLOW_CAN_FLY = 1,
    FLOW_MAX = 2,
};

struct grid_type {
    BIT_FLAGS info{}; /* Hack -- grid flags */

    FEAT_IDX feat{}; /* Hack -- feature type */
    ObjectIndexList o_idx_list; /* Object list in this grid */
    MONSTER_IDX m_idx{}; /* Monster in this grid */

    /*! 地形の特別な情報を保存する / Special grid info
     * 具体的な使用一覧はクエスト行き階段の移行先クエストID、
     * 各ダンジョン入口の移行先ダンジョンID、
     *
     */
    s16b special{};

    FEAT_IDX mimic{}; /* Feature to mimic */

    byte costs[FLOW_MAX]{}; /* Hack -- cost of flowing */
    byte dists[FLOW_MAX]{}; /* Hack -- distance from player */
    byte when{}; /* Hack -- when cost was computed */
};
