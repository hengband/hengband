#pragma once

#include <nlohmann/json.hpp>

class MessageReader {
public:
    explicit MessageReader(const nlohmann::json &message_data);
    MessageReader(nlohmann::json &&) = delete;
    MessageReader(const MessageReader &) = delete;
    MessageReader(MessageReader &&) = delete;
    MessageReader &operator=(const MessageReader &) = delete;
    MessageReader &operator=(MessageReader &&) = delete;

    int read() const;

private:
    int set_mon_message() const;

    const nlohmann::json &message_data;
};
