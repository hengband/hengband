#pragma once

class ReadExecutorBase {
public:
    virtual ~ReadExecutorBase() = default;

    virtual bool read() = 0;
    virtual bool is_identified() const = 0;

protected:
    ReadExecutorBase() = default;
};
