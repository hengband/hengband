#pragma once

#include <memory>

// Activation Execution.
class ItemEntity;
class PlayerType;
struct ae_type {
    int dir = 0;
    std::shared_ptr<ItemEntity> item = nullptr;
    int lev = 0;

    ae_type() = default;
    ae_type(PlayerType *player_ptr, short i_idx);

    void decide_activation_level();
};
