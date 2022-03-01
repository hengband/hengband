#pragma once

#include "object-use/read/read-executor-base.h"

class PlayerType;
class ObjectType;
class ScrollReadExecutor : public ReadExecutorBase {
public:
    ScrollReadExecutor(PlayerType *player_ptr, ObjectType *o_ptr, bool known);
    bool is_identified() const override;
    bool read() override;

private:
    PlayerType *player_ptr;
    ObjectType *o_ptr;
    bool known;
    bool ident = false;
};
