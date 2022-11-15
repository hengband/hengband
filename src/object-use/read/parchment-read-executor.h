#pragma once

#include "object-use/read/read-executor-base.h"

class PlayerType;
class ItemEntity;
class ParchmentReadExecutor : public ReadExecutorBase {
public:
    ParchmentReadExecutor(PlayerType *player_ptr, ItemEntity *o_ptr);
    bool read() override;
    bool is_identified() const override;

private:
    PlayerType *player_ptr;
    ItemEntity *o_ptr;
};
