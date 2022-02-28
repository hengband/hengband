#pragma once

#include <stdexcept>

class SaveDataNotSupportedException : public std::runtime_error {
public:
    SaveDataNotSupportedException() = delete;
    using std::runtime_error::runtime_error;
};
