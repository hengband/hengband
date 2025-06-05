#pragma once

#include "monster-race/race-speak-flags.h"
#include <map>
#include <string>
#include <tl/optional.hpp>
#include <vector>

class MonsterMessage {
public:
    MonsterMessage(int chance, std::string message);
    int get_message_chance() const;
    const std::string &get_message() const;

private:
    int chance;
    std::string message;
};

class MonsterMessageList {
public:
    tl::optional<const MonsterMessage &> get_message() const;
    void emplace(MonsterMessage message);

private:
    std::vector<MonsterMessage> messages;
};

class MonraceMessage {
public:
    tl::optional<const MonsterMessage &> get_message(MonsterMessageType message_type) const;
    void emplace(const MonsterMessageType message_type, const int chance, const std::string &message_str);

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
    tl::optional<const MonsterMessage &> get_message(const int monrace_id, const MonsterMessageType message_type) const;
    void emplace(const int monrace_id, const MonsterMessageType message_type, const int chance, const std::string &message_str);

private:
    MonraceMessageList() = default;
    static MonraceMessageList instance;

    std::map<int, MonraceMessage> messages;
};
