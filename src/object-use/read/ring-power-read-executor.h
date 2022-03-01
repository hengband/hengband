#pragma once

#include "object-use/read/read-executor-base.h"

class RingOfPowerReadExecutor : public ReadExecutorBase {
public:
    RingOfPowerReadExecutor() = default;
    bool read() override;
    bool is_identified() const override;
};
