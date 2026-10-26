#pragma once

#include "system/monrace/monrace-message.h"
#include <utility>

namespace test {
class MonraceMessageListTestAccess {
public:
    MonraceMessageListTestAccess()
        : list(MonraceMessageList::get_instance())
    {
        this->messages.swap(this->list.messages);
        std::swap(this->defaults, this->list.default_messages);
    }

    MonraceMessageListTestAccess(const MonraceMessageListTestAccess &) = delete;
    MonraceMessageListTestAccess &operator=(const MonraceMessageListTestAccess &) = delete;

    ~MonraceMessageListTestAccess()
    {
        this->messages.swap(this->list.messages);
        std::swap(this->defaults, this->list.default_messages);
    }

    size_t message_count(int id, MonsterMessageType action) const
    {
        const auto it = this->list.messages.find(id);
        return it != this->list.messages.end() ? count(it->second, action) : 0;
    }

    size_t default_message_count(MonsterMessageType action) const
    {
        return count(this->list.default_messages, action);
    }

private:
    static size_t count(const MonraceMessage &message, MonsterMessageType action)
    {
        const auto it = message.messages.find(action);
        return it != message.messages.end() ? it->second.messages.size() : 0;
    }

    MonraceMessageList &list;
    std::map<int, MonraceMessage> messages;
    MonraceMessage defaults;
};
}
