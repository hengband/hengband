#pragma once

#include "system/h-type.h"
#include <memory>

// Activation Execution.
class ItemEntity;
class PlayerType;
struct ae_type {
    int dir = 0;
    bool success = false;
    std::shared_ptr<ItemEntity> item = nullptr;
    int lev = 0;
    int chance = 0;
    int fail = 0;

    ae_type() = default;
    ae_type(PlayerType *player_ptr, short i_idx);

    void decide_activation_level();
    void decide_chance_fail(ACTION_SKILL_POWER skill);
};
