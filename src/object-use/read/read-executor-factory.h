#pragma once

#include "object-use/read/read-executor-base.h"
#include <memory>

class ObjectType;
class PlayerType;
class ReadExecutorFactory {
public:
    static std::shared_ptr<ReadExecutorBase> create(PlayerType *player_ptr, ObjectType *o_ptr, bool known);

private:
    ReadExecutorFactory() = delete;
};
