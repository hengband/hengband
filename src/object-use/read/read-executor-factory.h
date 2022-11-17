#pragma once

#include "object-use/read/read-executor-base.h"
#include <memory>

class ItemEntity;
class PlayerType;
class ReadExecutorFactory {
public:
    static std::unique_ptr<ReadExecutorBase> create(PlayerType *player_ptr, ItemEntity *o_ptr, bool known);

private:
    ReadExecutorFactory() = delete;
};
