#include "system/monrace/monrace-message.h"
#include "term/z-rand.h"
#include <optional>
#include <string>
#include <string_view>
#include <vector>

MonsterMessage::MonsterMessage(int chance, std::string_view message)
    : chance(chance)
    , message(message)
{
}

std::optional<std::string_view> MonsterMessage::get_message() const
{
    if (this->chance < 1) {
        return std::nullopt;
    }
    if (!one_in_(this->chance)) {
        return std::nullopt;
    }
    return this->message;
}

void MonsterMessageList::emplace(const int chance, std::string_view message_str)
{
    this->messages.emplace_back(chance, message_str);
}

std::optional<std::string_view> MonsterMessageList::get_message() const
{
    if (this->messages.empty()) {
        return std::nullopt;
    }

    return rand_choice(this->messages).get_message();
}

void MonraceMessage::emplace(const MonsterMessageType message_type, const int chance, std::string_view message_str)
{
    // map::operator[] はキーが存在しない場合にデフォルトコンストラクタでオブジェクトを生成する。これは意図した動作である。
    auto &message = this->messages[message_type];
    message.emplace(chance, message_str);
}

std::optional<std::string_view> MonraceMessage::get_message(MonsterMessageType message_type) const
{
    const auto &message = this->messages.find(message_type);
    if (message == this->messages.end()) {
        return std::nullopt;
    }
    return message->second.get_message();
}

bool MonraceMessage::has_message(MonsterMessageType message_type) const
{
    return this->messages.contains(message_type);
}

MonraceMessageList MonraceMessageList::instance{};

MonraceMessageList &MonraceMessageList::get_instance()
{
    return instance;
}

void MonraceMessageList::emplace(const int monrace_id, const MonsterMessageType message_type, const int chance, std::string_view message_str)
{
    // map::operator[] はキーが存在しない場合にデフォルトコンストラクタでオブジェクトを生成する。これは意図した動作である。
    auto &message = this->messages[monrace_id];
    message.emplace(message_type, chance, message_str);
}

std::optional<std::string_view> MonraceMessageList::get_message(const int monrace_id, const MonsterMessageType message_type) const
{
    auto message = this->messages.find(monrace_id);
    if (message == this->messages.end()) {
        return this->default_messages.get_message(message_type);
    }
    if (!message->second.has_message(message_type)) {
        return this->default_messages.get_message(message_type);
    }
    return message->second.get_message(message_type);
}

void MonraceMessageList::emplace_default(const MonsterMessageType message_type, const int chance, std::string_view message_str)
{
    this->default_messages.emplace(message_type, chance, message_str);
}
