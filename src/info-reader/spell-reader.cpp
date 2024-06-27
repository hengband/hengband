#include "info-reader/spell-reader.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "main/angband-headers.h"
#include "system/spell-info-list.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

/*!
 * @brief JSON Objectから呪文領域IDを取得する
 * @param spell_data 情報の格納されたJSON Object
 * @param realm_id 呪文領域ID
 * @return エラーコード
 */
static errr set_realm(const nlohmann::json &spell_data, int &realm_id)
{
    if (spell_data.is_null()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto &realmname_obj = spell_data["name"];
    if (!realmname_obj.is_string()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto realm = realms_list.find(realmname_obj.get<std::string>());
    if (realm == realms_list.end()) {
        return PARSE_ERROR_OUT_OF_BOUNDS;
    }
    realm_id = realm->second + 1; // 0-origin to 1-origin
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectから各呪文の詳細を取得する
 * @param spell_data 情報の格納されたJSON Object
 * @param realm_spell_list 呪文情報を格納するvector
 * @return エラーコード
 */
static errr set_spell_data(const nlohmann::json &spell_data, std::vector<SpellInfo> &realm_spell_list)
{
    if (spell_data.is_null()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    int spell_id;
    if (auto err = info_set_integer(spell_data["spell_id"], spell_id, true, Range(0, 31))) {
        msg_format(_("呪文ID読込失敗。ID: '%d'。", "Failed to load spell ID. ID: '%d'."), error_idx);
        return err;
    }
    auto info = SpellInfo();
    info.idx = static_cast<short>(spell_id);

    const auto &tag_obj = spell_data["spell_tag"];
    if (!tag_obj.is_string()) {
        msg_format(_("呪文タグ読込失敗。ID: '%d'。", "Failed to load spell tag. ID: '%d'."), error_idx);
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    info.tag = tag_obj.get<std::string>();

    if (auto err = info_set_string(spell_data["name"], info.name, true)) {
        msg_format(_("呪文名読込失敗。ID: '%d'。", "Failed to load spell name. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_string(spell_data["description"], info.description, true)) {
        msg_format(_("呪文説明読込失敗。ID: '%d'。", "Failed to load spell description. ID: '%d'."), error_idx);
        return err;
    }
    realm_spell_list[spell_id] = std::move(info);

    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectから各呪文の詳細を呪文書単位で取得する
 * @param spell_data 情報の格納されたJSON Object
 * @param realm_spell_list 呪文情報を格納するvector
 * @return エラーコード
 */
static errr set_book_data(const nlohmann::json &spell_data, std::vector<SpellInfo> &realm_spell_list)
{
    auto &book_obj = spell_data["books"];
    if (book_obj.is_null()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    for (auto &element : book_obj.items()) {
        auto &book = element.value();
        auto &spells_obj = book["spells"];
        if (spells_obj.is_null()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        for (auto &spell_element : spells_obj.items()) {
            auto &spell = spell_element.value();
            set_spell_data(spell, realm_spell_list);
        }
    }
    return PARSE_ERROR_NONE;
}

/*!
 * @brief 職業魔法情報(ClassMagicDefinitions)のパース関数
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_spell_info(nlohmann::json &spell_data, angband_header *)
{
    int realm_id;
    if (auto err = set_realm(spell_data, realm_id)) {
        msg_format(_("領域名読込失敗。ID: '%d'。", "Failed to load realm name. ID: '%d'."), error_idx);
        return err;
    }

    auto &spell_info_list = SpellInfoList::get_instance();
    auto &realm_spell_list = spell_info_list.spell_list[realm_id];

    if (auto err = set_book_data(spell_data, realm_spell_list)) {
        msg_format(_("呪文詳細読込失敗。ID: '%d'。", "Failed to load spell data. ID: '%d'."), error_idx);
        return err;
    }

    return PARSE_ERROR_NONE;
}
