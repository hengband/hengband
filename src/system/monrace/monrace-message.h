#pragma once

#include "monster-race/race-speak-flags.h"
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

class MonsterMessage {
public:
    MonsterMessage(int chance, std::string_view message);
    std::optional<std::string_view> get_message() const;

private:
    int chance;
    std::string message;
};

class MonsterMessageList {
public:
    std::optional<std::string_view> get_message() const;
    void emplace(const int chance, std::string_view message_str);

private:
    std::vector<MonsterMessage> messages;
};

class MonraceMessage {
public:
    std::optional<std::string_view> get_message(MonsterMessageType message_type) const;
    bool has_message(MonsterMessageType message_type) const;
    void emplace(const MonsterMessageType message_type, const int chance, std::string_view message_str);

private:
    std::map<MonsterMessageType, MonsterMessageList> messages;
};

class MonraceMessageList {
public:
    MonraceMessageList(const MonraceMessageList &) = delete;
    MonraceMessageList(MonraceMessageList &&) = delete;
    MonraceMessageList &operator=(const MonraceMessageList &) = delete;
    MonraceMessageList &operator=(MonraceMessageList &&) = delete;
    ~MonraceMessageList() = default;

    static MonraceMessageList &get_instance();
    std::optional<std::string_view> get_message(const int monrace_id, const MonsterMessageType message_type) const;
    void emplace(const int monrace_id, const MonsterMessageType message_type, const int chance, std::string_view message_str);
    void emplace_default(const MonsterMessageType message_type, const int chance, std::string_view message_str);

private:
    MonraceMessageList() = default;
    static MonraceMessageList instance;

    std::map<int, MonraceMessage> messages;
    MonraceMessage default_messages;
};
