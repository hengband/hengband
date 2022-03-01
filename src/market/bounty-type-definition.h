#pragma once

#include "system/angband.h"

struct bounty_type {
    MONRACE_IDX r_idx; //!< 賞金首の対象のモンスター種族ID
    bool is_achieved; //!< 死体を渡して報酬を獲得したかどうか
};
