#include "info-reader/message-reader.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/race-info-tokens-table.h"
#include "locale/character-encoding.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-list.h"
#include "system/monrace/monrace-message.h"
#include "util/enum-converter.h"
#include "view/display-messages.h"
#include <nlohmann/json.hpp>
#include <string>
#include <utility>
#include <vector>

MessageReader::MessageReader(const nlohmann::json &message_data)
    : message_data(message_data)
{
}

/*!
 * @brief モンスターメッセージ情報(JSON Object)のパース関数
 * @return 成功時は空の値、失敗時は解析エラーコード
 */
tl::expected<void, ParseErrorType> MessageReader::read() const
{
    if (const auto result = this->set_mon_message(); !result) {
        msg_print(_("モンスターメッセージ読込失敗。", "Failed to load monster message."));
        return result;
    }

    return {};
}

/*!
 * @brief JSON Objectからid群をセットする
 * @param id_list_data id群情報の格納されたJSON Object
 * @param id_list id群情報を保管するvector
 * @return 成功時は空の値、失敗時は解析エラーコード
 * @details モンスター種族の定義を読み込んだ後に呼ぶ。未定義の種族IDは ParseErrorType::INVALID_VALUE を返す。
 */
static tl::expected<void, ParseErrorType> set_id_list(const nlohmann::json &id_list_data, std::vector<int> &id_list)
{
    if (!id_list_data.is_array()) {
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    for (const auto &id_data : id_list_data) {
        if (!id_data.is_number()) {
            return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
        }
        int id = 0;
        if (const auto result = info_set_integer(id_data, id, true, Range(1, MAX_MONRACE_ID)); !result) {
            return result;
        }
        // 未定義の種族IDに登録したメッセージは使われないため、定義の誤記として拒否する。
        const auto monrace_id = i2enum<MonraceId>(id);
        if (!MonraceList::is_valid(monrace_id) || !MonraceList::get_instance().contains(monrace_id)) {
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }
        id_list.push_back(id);
    }
    return {};
}

/*!
 * @brief JSON Objectからモンスターのメッセージをセットする
 * @return 成功時は空の値、失敗時は解析エラーコード
 * @details 選択言語が無く他言語だけがあるメッセージは読み飛ばし、後続の解析を続ける。
 * グループ全体の解析に成功した場合だけ、選択言語のメッセージをまとめて公開する。
 */
tl::expected<void, ParseErrorType> MessageReader::set_mon_message() const
{
    const auto &group_data = this->message_data;
    const auto message_iter = group_data.find("message");
    if (message_iter == group_data.end() || !message_iter->is_array()) {
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }
    const auto &message_array = message_iter.value();
    auto id_list = std::vector<int>();
    struct PendingMessage {
        MonsterMessageType action;
        int chance = 0;
        bool use_name;
        std::string text;
    };
    std::vector<PendingMessage> pending_messages;

    const auto id_list_iter = group_data.find("id_list");
    const auto has_id_list = (id_list_iter != group_data.end());
    const auto publish = [&] {
        for (const auto &message : pending_messages) {
            if (has_id_list) {
                for (const auto id : id_list) {
                    MonraceMessageList::get_instance().emplace(id, message.action, message.chance, message.use_name, message.text);
                }
            } else {
                MonraceMessageList::get_instance().emplace_default(message.action, message.chance, message.use_name, message.text);
            }
        }
    };

    if (has_id_list) {
        if (const auto result = set_id_list(id_list_iter.value(), id_list); !result) {
            return result;
        }
    } else {
        const auto &name_obj = get_json_value(group_data, "name");
        if (!name_obj.is_string()) {
            return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
        }

        const auto group_name = name_obj.get<std::string>();
        if (group_name != "DEFAULT") {
            return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
        }
    }

    for (const auto &message : message_array) {
        const auto action = info_read_token(get_json_value(message, "action"), r_info_message_flags, ParseErrorType::TOO_FEW_ARGUMENTS);
        if (!action) {
            return tl::make_unexpected(action.error());
        }

        const auto chance_iter = message.find("chance");
        if (chance_iter == message.end() || !chance_iter->is_number()) {
            return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
        }
        const auto &chance_data = chance_iter.value();
        int chance = 0;
        if (const auto result = info_set_integer(chance_data, chance, true, Range(1, 100)); !result) {
            return result;
        }

        bool use_name = true;
        const auto use_name_iter = message.find("use_name");
        if (use_name_iter != message.end()) {
            const auto &use_name_data = use_name_iter.value();
            if (const auto result = info_set_bool(use_name_data, use_name, false); !result) {
                return result;
            }
        }

        const auto language_iter = message.find("message");
        if (language_iter == message.end() || !language_iter->is_object()) {
            return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
        }
        const auto &language_list = language_iter.value();

        const auto &ja_list = language_list.find("ja");
        const auto &en_list = language_list.find("en");
#ifdef JP
        if (ja_list == language_list.end()) {
            if (en_list == language_list.end()) {
                return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
            }
            continue;
        }
        const auto &message_list = ja_list.value();
#else
        if (en_list == language_list.end()) {
            if (ja_list == language_list.end()) {
                return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
            }
            continue;
        }
        const auto &message_list = en_list.value();
#endif

        if (message_list.is_null()) {
            return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
        }
        if (!message_list.is_array()) {
            return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
        }

        for (const auto &message_str : message_list) {
            if (message_str.is_null()) {
                return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
            }
            if (!message_str.is_string()) {
                return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
            }
            auto str_test = utf8_to_sys(message_str.get<std::string>());
            if (!str_test) {
                return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
            }
            pending_messages.push_back({ *action, chance, use_name, std::move(*str_test) });
        }
    }
    publish();
    return {};
}
