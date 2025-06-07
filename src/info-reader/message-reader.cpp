#include "info-reader/message-reader.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/race-info-tokens-table.h"
#include "locale/japanese.h"
#include "main/angband-headers.h"
#include "player-ability/player-ability-types.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monrace/monrace-message.h"
#include "term/gameterm.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include <string>

/*!
 * @brief JSON Objectからid群をセットする
 * @param id_list_data id群情報の格納されたJSON Object
 * @param id_list id群情報を保管するvector
 * @return エラーコード
 */
static errr set_id_list(const nlohmann::json &id_list_data, std::vector<int> &id_list)
{
    if (!id_list_data.is_array()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    for (auto &id_data : id_list_data.items()) {
        if (!id_data.value().is_number()) {
            return PARSE_ERROR_INVALID_FLAG;
        }
        int id;
        if (auto err = info_set_integer(id_data.value(), id, true, Range(1, 9999))) {
            return err;
        }
        id_list.push_back(id);
    }
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからモンスターのメッセージをセットする
 * @param message_data メッセージ情報の格納されたJSON Object
 * @return エラーコード
 */
static errr set_mon_message(const nlohmann::json &group_data)
{
    const auto message_iter = group_data.find("message");
    if (message_iter == group_data.end() || !message_iter->is_array()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    const auto &message_data = message_iter.value();
    auto id_list = std::vector<int>();

    const auto id_list_iter = group_data.find("id_list");
    const auto has_id_list = (id_list_iter != group_data.end());

    if (has_id_list) {
        errr id_err = set_id_list(id_list_iter.value(), id_list);
        if (id_err != PARSE_ERROR_NONE) {
            return id_err;
        }
    } else {
        if (!group_data["name"].is_string()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto group_name = group_data["name"].get<std::string>();
        if (group_name != "DEFAULT") {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }
    }

    if (!message_data.is_array()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    for (const auto &message : message_data.items()) {
        const auto action_iter = message.value().find("action");
        if (action_iter == message.value().end() || !action_iter->is_string()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }
        const auto &action_data = action_iter.value();
        const auto action = r_info_message_flags.find(action_data.get<std::string>());
        if (action == r_info_message_flags.end()) {
            return PARSE_ERROR_INVALID_FLAG;
        }

        const auto chance_iter = message.value().find("chance");
        if (chance_iter == message.value().end() || !chance_iter->is_number()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }
        const auto &chance_data = chance_iter.value();
        int chance;
        if (auto err = info_set_integer(chance_data, chance, true, Range(1, 100))) {
            return err;
        }

        const auto language_iter = message.value().find("message");
        if (language_iter == message.value().end() || !language_iter->is_object()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }
        const auto &language_list = language_iter.value();

        const auto &ja_list = language_list.find("ja");
        const auto &en_list = language_list.find("en");
#ifdef JP
        if (ja_list == language_list.end()) {
            if (en_list == language_list.end()) {
                return PARSE_ERROR_TOO_FEW_ARGUMENTS;
            }
            return PARSE_ERROR_NONE;
        }
        const auto &message_list = ja_list.value();
#else
        if (en_list == language_list.end()) {
            if (ja_list == language_list.end()) {
                return PARSE_ERROR_TOO_FEW_ARGUMENTS;
            }
            return PARSE_ERROR_NONE;
        }
        const auto &message_list = en_list.value();
#endif

        for (const auto &message_str : message_list.items()) {
            if (message_str.value().is_null()) {
                return PARSE_ERROR_TOO_FEW_ARGUMENTS;
            }
            if (!message_str.value().is_string()) {
                return PARSE_ERROR_INVALID_FLAG;
            }
#ifdef JP
            auto str_test = utf8_to_sys(message_str.value().get<std::string>());
            if (!str_test) {
                return PARSE_ERROR_INVALID_FLAG;
            }
            auto str = std::move(*str_test);
#else
            auto str = message_str.value().get<std::string>();
#endif
            if (has_id_list) {
                for (auto id : id_list) {
                    MonraceMessageList::get_instance().emplace(id, action->second, chance, str);
                }
            } else {
                MonraceMessageList::get_instance().emplace_default(action->second, chance, str);
            }
        }
    }
    return PARSE_ERROR_NONE;
}

/*!
 * @brief モンスターメッセージ情報(JSON Object)のパース関数
 * @param mon_data モンスターメッセージの格納されたJSON Object
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_monster_messages_info(nlohmann::json &message_data, angband_header *)
{
    errr err;
    err = set_mon_message(message_data);
    if (err) {
        msg_format(_("モンスターメッセージ読込失敗。", "Failed to load monster message."));
        return err;
    }

    return PARSE_ERROR_NONE;
}
