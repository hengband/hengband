#pragma once

#include "object-use/read/read-executor-base.h"

class GbhShirtReadExecutor : public ReadExecutorBase {
public:
    GbhShirtReadExecutor() = default;
    bool read() override;
    bool is_identified() const override;
};
