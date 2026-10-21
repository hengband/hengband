#pragma once

#include "info-reader/parse-error-types.h"
#include <nlohmann/json_fwd.hpp>
#include <tl/expected.hpp>

class MessageReader {
public:
    explicit MessageReader(const nlohmann::json &message_data);
    MessageReader(nlohmann::json &&) = delete;
    MessageReader(const MessageReader &) = delete;
    MessageReader(MessageReader &&) = delete;
    MessageReader &operator=(const MessageReader &) = delete;
    MessageReader &operator=(MessageReader &&) = delete;

    tl::expected<void, parse_error_type> read() const;

private:
    tl::expected<void, parse_error_type> set_mon_message() const;

    const nlohmann::json &message_data;
};
