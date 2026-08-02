#include "info-reader/spell-reader.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "system/spell-info-list.h"
#include "util/enum-converter.h"
#include "view/display-messages.h"

SpellReader::SpellReader(const nlohmann::json &realm_data, SpellInfoList &spell_info_list)
    : realm_data(realm_data)
    , spell_info_list(spell_info_list)
{
}

/*!
 * @brief 呪文情報(SpellDefinitions)のパース関数
 * @return エラーコード
 */
int SpellReader::read() const
{
    error_idx++; //!< @note 呪文領域は数値IDを持たないため、エラー表示用に要素の順序を用いる

    RealmType realm_id;
    if (auto err = this->set_realm(realm_id)) {
        msg_format(_("領域名読込失敗。ID: '%d'。", "Failed to load realm name. ID: '%d'."), error_idx);
        return err;
    }

    if (auto err = this->set_book_data(realm_id)) {
        msg_format(_("呪文詳細読込失敗。ID: '%d'。", "Failed to load spell data. ID: '%d'."), error_idx);
        return err;
    }

    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectから呪文領域IDを取得する
 * @param realm 呪文領域
 * @return エラーコード
 */
int SpellReader::set_realm(RealmType &realm) const
{
    if (this->realm_data.is_null()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto &realmname_obj = get_json_value(this->realm_data, "name");
    if (!realmname_obj.is_string()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto it = realms_list.find(realmname_obj.get<std::string>());
    if (it == realms_list.end()) {
        return PARSE_ERROR_OUT_OF_BOUNDS;
    }
    realm = it->second;
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectから各呪文の詳細を取得する
 * @param spell_data 情報の格納されたJSON Object
 * @param realm 領域ID
 * @return エラーコード
 */
int SpellReader::set_spell_data(const nlohmann::json &spell_data, RealmType realm) const
{
    if (spell_data.is_null()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    int spell_id;
    if (auto err = info_set_integer(get_json_value(spell_data, "spell_id"), spell_id, true, Range(0, 31))) {
        msg_format(_("呪文ID読込失敗。ID: '%d'。", "Failed to load spell ID. ID: '%d'."), error_idx);
        return err;
    }
    auto info = SpellInfo();
    info.idx = static_cast<short>(spell_id);

    const auto &tag_obj = get_json_value(spell_data, "spell_tag");
    if (!tag_obj.is_string()) {
        msg_format(_("呪文タグ読込失敗。ID: '%d'。", "Failed to load spell tag. ID: '%d'."), error_idx);
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    info.tag = tag_obj.get<std::string>();

    if (auto err = info_set_string(get_json_value(spell_data, "name"), info.name, true)) {
        msg_format(_("呪文名読込失敗。ID: '%d'。", "Failed to load spell name. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_string(get_json_value(spell_data, "description"), info.description, true)) {
        msg_format(_("呪文説明読込失敗。ID: '%d'。", "Failed to load spell description. ID: '%d'."), error_idx);
        return err;
    }
    this->spell_info_list.set_spell_info(realm, spell_id, std::move(info));

    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectから各呪文の詳細を呪文書単位で取得する
 * @param realm 領域ID
 * @return エラーコード
 */
int SpellReader::set_book_data(RealmType realm) const
{
    const auto &book_obj = get_json_value(this->realm_data, "books");
    if (book_obj.is_null()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    for (const auto &book : book_obj) {
        const auto &spells_obj = get_json_value(book, "spells");
        if (spells_obj.is_null()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        for (const auto &spell : spells_obj) {
            if (auto err = this->set_spell_data(spell, realm)) {
                return err;
            }
        }
    }
    return PARSE_ERROR_NONE;
}
