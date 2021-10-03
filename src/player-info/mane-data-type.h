#pragma once

#include "system/angband.h"

#include <deque>

constexpr int MAX_MANE = 16;

enum class RF_ABILITY;

struct mane_data_type {
    struct mane_type {
        RF_ABILITY spell{};
        HIT_POINT damage{};
    };

    std::deque<mane_type> mane_list{};
    bool new_mane{};
};
