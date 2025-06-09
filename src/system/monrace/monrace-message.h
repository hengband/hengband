#pragma once

#include "monster-race/race-speak-flags.h"
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <tl/optional.hpp>
#include <vector>

class MonsterMessage {
public:
    MonsterMessage(int chance, bool use_name, std::string_view message);
    std::optional<std::string_view> get_message() const;
    bool start_with_monname() const;

private:
    int chance;
    bool use_name;
    std::string message;
};

class MonsterMessageList {
public:
    tl::optional<const MonsterMessage &> get_message_obj() const;
    void emplace(const int chance, bool use_name, std::string_view message_str);

private:
    std::vector<MonsterMessage> messages;
};

class MonraceMessage {
public:
    tl::optional<const MonsterMessage &> get_message_obj(MonsterMessageType message_type) const;
    bool has_message(MonsterMessageType message_type) const;
    void emplace(const MonsterMessageType message_type, const int chance, bool use_name, std::string_view message_str);

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
    std::optional<std::string> get_message(const int monrace_id, std::string_view monrace_name, const MonsterMessageType message_type);
    void emplace(const int monrace_id, const MonsterMessageType message_type, const int chance, bool use_name, std::string_view message_str);
    void emplace_default(const MonsterMessageType message_type, const int chance, bool use_name, std::string_view message_str);

private:
    MonraceMessageList() = default;
    static MonraceMessageList instance;
    tl::optional<const MonsterMessage &> get_message_obj(const int monrace_id, const MonsterMessageType message_type) const;

    std::map<int, MonraceMessage> messages;
    MonraceMessage default_messages;
};
