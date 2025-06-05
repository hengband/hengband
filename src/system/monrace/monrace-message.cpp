#include "system/monrace/monrace-message.h"
#include "term/z-rand.h"
#include <optional>
#include <vector>

MonsterMessage::MonsterMessage(int chance, std::string message)
    : chance(chance)
    , message(message)
{
}

int MonsterMessage::get_message_chance() const
{
    return this->chance;
}

const std::string &MonsterMessage::get_message() const
{
    return this->message;
}

void MonsterMessageList::emplace(MonsterMessage message)
{
    this->messages.emplace_back(std::move(message));
}

const std::optional<MonsterMessage> MonsterMessageList::get_message() const
{
    if (this->messages.empty()) {
        return std::nullopt;
    }

    const auto size = this->messages.size();
    return this->messages[randint1(size) - 1];
}

void MonraceMessage::emplace(const MonsterMessageType message_type, const int chance, const std::string &message_str)
{
    const auto &message = this->messages.find(message_type);
    if (message == this->messages.end()) {
        auto list = MonsterMessageList();
        list.emplace(MonsterMessage(chance, message_str));
        this->messages.emplace(message_type, list);
        return;
    }
    message->second.emplace(MonsterMessage(chance, message_str));
}

const std::optional<MonsterMessage> MonraceMessage::get_message(MonsterMessageType message_type) const
{
    const auto &message = this->messages.find(message_type);
    if (message == this->messages.end()) {
        return std::nullopt;
    }
    return message->second.get_message();
}

MonraceMessageList MonraceMessageList::instance{};

MonraceMessageList &MonraceMessageList::get_instance()
{
    return instance;
}

void MonraceMessageList::emplace(const int monrace_id, const MonsterMessageType message_type, const int chance, const std::string &message_str)
{
    const auto &message = this->messages.find(monrace_id);
    if (message == this->messages.end()) {
        auto monrace_message = MonraceMessage();
        monrace_message.emplace(message_type, chance, message_str);
        this->messages.emplace(monrace_id, monrace_message);
        return;
    }
    message->second.emplace(message_type, chance, message_str);
}

const std::optional<MonsterMessage> MonraceMessageList::get_message(const int monrace_id, const MonsterMessageType message_type) const
{
    auto message = this->messages.find(monrace_id);
    if (message == this->messages.end()) {
        return std::nullopt;
    }
    return message->second.get_message(message_type);
}
