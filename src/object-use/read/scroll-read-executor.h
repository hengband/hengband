#pragma once

#include "object-use/read/read-executor-base.h"

class PlayerType;
class ItemEntity;
class ScrollReadExecutor : public ReadExecutorBase {
public:
    ScrollReadExecutor(PlayerType *player_ptr, ItemEntity *o_ptr, bool known);
    bool is_identified() const override;
    bool read() override;

private:
    PlayerType *player_ptr;
    ItemEntity *o_ptr;
    bool known;
    bool ident = false;
};
