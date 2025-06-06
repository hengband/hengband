#include "system/monrace/monrace-message.h"
#include "term/z-rand.h"
#include <optional>
#include <vector>

MonsterMessage::MonsterMessage(int chance, std::string_view message)
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

void MonsterMessageList::emplace(const int chance, std::string_view message_str)
{
    this->messages.emplace_back(chance, message_str);
}

tl::optional<const MonsterMessage &> MonsterMessageList::get_message() const
{
    if (this->messages.empty()) {
        return tl::nullopt;
    }

    return rand_choice(this->messages);
}

void MonraceMessage::emplace(const MonsterMessageType message_type, const int chance, std::string_view message_str)
{
    // map::operator[] はキーが存在しない場合にデフォルトコンストラクタでオブジェクトを生成する。これは意図した動作である。
    auto &message = this->messages[message_type];
    message.emplace(chance, message_str);
}

tl::optional<const MonsterMessage &> MonraceMessage::get_message(MonsterMessageType message_type) const
{
    const auto &message = this->messages.find(message_type);
    if (message == this->messages.end()) {
        return tl::nullopt;
    }
    return message->second.get_message();
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

tl::optional<const MonsterMessage &> MonraceMessageList::get_message(const int monrace_id, const MonsterMessageType message_type) const
{
    auto message = this->messages.find(monrace_id);
    if (message == this->messages.end()) {
        return tl::nullopt;
    }
    return message->second.get_message(message_type);
}
