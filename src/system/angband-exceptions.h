#pragma once

#include <stdexcept>

class SaveDataNotSupportedException : public std::runtime_error {
public:
    SaveDataNotSupportedException(const char *message)
        : std::runtime_error(message)
    {
    }
};
