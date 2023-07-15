#pragma once

#include "system/angband.h"

#include <deque>

constexpr int MAX_MANE = 16;

enum class MonsterAbilityType;

struct mane_data_type {
    struct mane_type {
        MonsterAbilityType spell{};
        int damage{};
    };

    std::deque<mane_type> mane_list{};
    bool new_mane{};
};
