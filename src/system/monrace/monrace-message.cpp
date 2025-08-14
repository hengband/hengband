#include "system/monrace/monrace-message.h"
#include "locale/language-switcher.h"
#include "term/z-rand.h"
#include "util/string-processor.h"
#include <tl/optional.hpp>
#include <vector>

MonsterMessage::MonsterMessage(int chance, bool use_name, std::string_view message)
    : chance(chance)
    , use_name(use_name)
    , message(message)
{
}

tl::optional<std::string_view> MonsterMessage::get_message() const
{
    if (this->chance < 1) {
        return tl::nullopt;
    }
    if (!one_in_(this->chance)) {
        return tl::nullopt;
    }
    return this->message;
}

bool MonsterMessage::start_with_monname() const
{
    return this->use_name;
}

void MonsterMessageList::emplace(const int chance, bool use_name, std::string_view message_str)
{
    this->messages.emplace_back(chance, use_name, message_str);
}

tl::optional<const MonsterMessage &> MonsterMessageList::get_message_obj() const
{
    if (this->messages.empty()) {
        return tl::nullopt;
    }

    return rand_choice(this->messages);
}

void MonraceMessage::emplace(const MonsterMessageType message_type, const int chance, bool use_name, std::string_view message_str)
{
    // map::operator[] はキーが存在しない場合にデフォルトコンストラクタでオブジェクトを生成する。これは意図した動作である。
    auto &message = this->messages[message_type];
    message.emplace(chance, use_name, message_str);
}

tl::optional<const MonsterMessage &> MonraceMessage::get_message_obj(MonsterMessageType message_type) const
{
    const auto &message = this->messages.find(message_type);
    if (message == this->messages.end()) {
        return tl::nullopt;
    }
    return message->second.get_message_obj();
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

tl::optional<std::string> MonraceMessageList::get_message(const int monrace_id, std::string_view monrace_name, const MonsterMessageType message_type)
{
    auto message_obj = this->get_message_obj(monrace_id, message_type);

    if (!message_obj) {
        return tl::nullopt;
    }

    auto message_str = message_obj->get_message();
    if (!message_str) {
        return tl::nullopt;
    }

    if (message_obj->start_with_monname()) {
        return str_upcase_first(monrace_name) + _("", " ") + std::string(*message_str);
    }
    return std::string(*message_str);
}

void MonraceMessageList::emplace(const int monrace_id, const MonsterMessageType message_type, const int chance, bool use_name, std::string_view message_str)
{
    // map::operator[] はキーが存在しない場合にデフォルトコンストラクタでオブジェクトを生成する。これは意図した動作である。
    auto &message = this->messages[monrace_id];
    message.emplace(message_type, chance, use_name, message_str);
}

tl::optional<const MonsterMessage &> MonraceMessageList::get_message_obj(const int monrace_id, const MonsterMessageType message_type) const
{
    auto message_iter = this->messages.find(monrace_id);
    if (message_iter == this->messages.end()) {
        return this->default_messages.get_message_obj(message_type);
    }
    if (!message_iter->second.has_message(message_type)) {
        switch (message_type) {
        case MonsterMessageType::SPEAK_BATTLE:
        case MonsterMessageType::SPEAK_FEAR:
        case MonsterMessageType::SPEAK_FRIEND:
            return this->get_message_obj(monrace_id, MonsterMessageType::SPEAK_ALL);
        default:
            return this->default_messages.get_message_obj(message_type);
        }
    }
    return message_iter->second.get_message_obj(message_type);
}

void MonraceMessageList::emplace_default(const MonsterMessageType message_type, const int chance, bool use_name, std::string_view message_str)
{
    this->default_messages.emplace(message_type, chance, use_name, message_str);
}
