#pragma once

#include "object-use/read/read-executor-base.h"

class PlayerType;
class ObjectType;
class ParchmentReadExecutor : public ReadExecutorBase {
public:
    ParchmentReadExecutor(PlayerType *player_ptr, ObjectType *o_ptr);
    bool read() override;
    bool is_identified() const override;

private:
    PlayerType *player_ptr;
    ObjectType *o_ptr;
};
