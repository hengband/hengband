#include "info-reader/magic-reader.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "main/angband-headers.h"
#include "player-ability/player-ability-types.h"
#include "player-info/class-info.h"
#include "system/spell-info-list.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

namespace {
/*!
 * @brief 魔法タイプ名とtvalの対応表
 */
const std::unordered_map<std::string_view, ItemKindType> spelltype_to_tval = {
    { "SORCERY", ItemKindType::SORCERY_BOOK },
    { "LIFE", ItemKindType::LIFE_BOOK },
    { "MUSIC", ItemKindType::MUSIC_BOOK },
    { "HISSATSU", ItemKindType::HISSATSU_BOOK },
    { "NONE", ItemKindType::NONE },
};

/*!
 * @brief 魔法必須能力とenumの対応表
 */
const std::unordered_map<std::string_view, int> name_to_stat = {
    { "STR", A_STR },
    { "INT", A_INT },
    { "WIS", A_WIS },
    { "DEX", A_DEX },
    { "CON", A_CON },
    { "CHR", A_CHR },
};

/*!
 * @brief 職業名とenumの対応表
 */
const std::unordered_map<std::string_view, PlayerClassType> class_list = {
    { "WARRIOR", PlayerClassType::WARRIOR },
    { "MAGE", PlayerClassType::MAGE },
    { "PRIEST", PlayerClassType::PRIEST },
    { "ROGUE", PlayerClassType::ROGUE },
    { "RANGER", PlayerClassType::RANGER },
    { "PALADIN", PlayerClassType::PALADIN },
    { "WARRIOR_MAGE", PlayerClassType::WARRIOR_MAGE },
    { "CHAOS_WARRIOR", PlayerClassType::CHAOS_WARRIOR },
    { "MONK", PlayerClassType::MONK },
    { "MINDCRAFTER", PlayerClassType::MINDCRAFTER },
    { "HIGH_MAGE", PlayerClassType::HIGH_MAGE },
    { "TOURIST", PlayerClassType::TOURIST },
    { "IMITATOR", PlayerClassType::IMITATOR },
    { "BEASTMASTER", PlayerClassType::BEASTMASTER },
    { "SORCERER", PlayerClassType::SORCERER },
    { "ARCHER", PlayerClassType::ARCHER },
    { "MAGIC_EATER", PlayerClassType::MAGIC_EATER },
    { "BARD", PlayerClassType::BARD },
    { "RED_MAGE", PlayerClassType::RED_MAGE },
    { "SAMURAI", PlayerClassType::SAMURAI },
    { "FORCETRAINER", PlayerClassType::FORCETRAINER },
    { "BLUE_MAGE", PlayerClassType::BLUE_MAGE },
    { "CAVALRY", PlayerClassType::CAVALRY },
    { "BERSERKER", PlayerClassType::BERSERKER },
    { "SMITH", PlayerClassType::SMITH },
    { "MIRROR_MASTER", PlayerClassType::MIRROR_MASTER },
    { "NINJA", PlayerClassType::NINJA },
    { "SNIPER", PlayerClassType::SNIPER },
    { "ELEMENTALIST", PlayerClassType::ELEMENTALIST },
};
}

/*!
 * @brief JSON Objectから職業IDを取得する
 * @param class_data 情報の格納されたJSON Object
 * @param class_id 職業ID
 * @return エラーコード
 */
static errr set_class_id(const nlohmann::json &class_data, int &class_id)
{
    if (class_data.is_null()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto &classname_obj = class_data["name"];
    if (!classname_obj.is_string()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto player_class = class_list.find(classname_obj.get<std::string>());
    if (player_class == class_list.end()) {
        return PARSE_ERROR_OUT_OF_BOUNDS;
    }
    class_id = enum2i(player_class->second);
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectから職業で使用する呪文タイプを取得する
 * @param class_data 情報の格納されたJSON Object
 * @param magics_info 保存先の魔法情報構造体
 * @return エラーコード
 */
static errr set_spell_type(const nlohmann::json &class_data, player_magic &magics_info)
{
    if (class_data.is_null()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto &spell_book = class_data["spell_type"];
    if (!spell_book.is_string()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto tval = spelltype_to_tval.find(spell_book.get<std::string>());
    if (tval == spelltype_to_tval.end()) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    magics_info.spell_book = tval->second;
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectから職業で呪文行使に使用する能力値を取得する
 * @param class_data 情報の格納されたJSON Object
 * @param magics_info 保存先の魔法情報構造体
 * @return エラーコード
 */
static errr set_magic_status(const nlohmann::json &class_data, player_magic &magics_info)
{
    if (class_data.is_null()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto &magic_status = class_data["magic_status"];
    if (!magic_status.is_string()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto status = name_to_stat.find(magic_status.get<std::string>());
    if (status == name_to_stat.end()) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    magics_info.spell_stat = status->second;
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectから各呪文の詳細を取得する
 * @param class_data 情報の格納されたJSON Object
 * @param magics_info 保存先の魔法情報構造体
 * @return エラーコード
 */
static errr set_spell_data(const nlohmann::json &spell_data, player_magic &magics_info, int realm_id)
{
    const auto &spell_tag_obj = spell_data["spell_tag"];
    if (!spell_tag_obj.is_string()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    const auto spell_id = SpellInfoList::get_instance().get_spell_id(realm_id + 1, spell_tag_obj.get<std::string>());
    if (!spell_id) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    auto &info = magics_info.info[realm_id][*spell_id];

    if (auto err = info_set_integer(spell_data["learn_level"], info.slevel, true, Range(0, 99))) {
        msg_format(_("呪文学習レベル読込失敗。ID: '%d'。", "Failed to load spell learn_level. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(spell_data["mana_cost"], info.smana, true, Range(0, 999))) {
        msg_format(_("呪文コスト読込失敗。ID: '%d'。", "Failed to load spell mana_cost. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(spell_data["difficulty"], info.sfail, true, Range(0, 999))) {
        msg_format(_("呪文難易度読込失敗。ID: '%d'。", "Failed to load spell difficulty. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(spell_data["first_cast_exp_rate"], info.sexp, true, Range(0, 999))) {
        msg_format(_("呪文詠唱ボーナスEXP読込失敗。ID: '%d'。", "Failed to load spell first_cast_exp_rate. ID: '%d'."), error_idx);
        return err;
    }

    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectから職業毎に定義された各呪文の詳細を取得する
 * @param class_data 情報の格納されたJSON Object
 * @param magics_info 保存先の魔法情報構造体
 * @return エラーコード
 */
static errr set_realm_data(const nlohmann::json &class_data, player_magic &magics_info)
{
    if (class_data.is_null()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    for (auto &element : class_data["realms"].items()) {
        auto &realm = element.value();
        auto &realm_name_obj = realm["name"];
        if (!realm_name_obj.is_string()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto realm_name = realms_list.find(realm_name_obj.get<std::string>());
        if (realm_name == realms_list.end()) {
            return PARSE_ERROR_INVALID_FLAG;
        }

        const auto realm_id = realm_name->second;
        auto &spells_info_obj = realm["spells_info"];
        if (spells_info_obj.is_null()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        for (auto &spell_info : spells_info_obj.items()) {
            if (auto err = set_spell_data(spell_info.value(), magics_info, realm_id)) {
                msg_format(_("呪文データ読込失敗。ID: '%d'。", "Failed to load spell data. ID: '%d'."), error_idx);
                return err;
            }
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
errr parse_class_magics_info(nlohmann::json &class_data, angband_header *)
{
    int class_id;
    if (auto err = set_class_id(class_data, class_id)) {
        msg_format(_("職業ID読込失敗。ID: '%d'。", "Failed to load class id. ID: '%d'."), error_idx);
        return err;
    }

    if (class_id < error_idx) {
        return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
    }
    error_idx = class_id;
    player_magic &magics_info = class_magics_info[class_id];

    if (auto err = set_spell_type(class_data, magics_info)) {
        msg_format(_("呪文タイプ読込失敗。ID: '%d'。", "Failed to load spell type. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = set_magic_status(class_data, magics_info)) {
        msg_format(_("呪文行使に使用する能力値読込失敗。ID: '%d'。", "Failed to load magic status. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_bool(class_data["has_glove_mp_penalty"], magics_info.has_glove_mp_penalty, true)) {
        msg_format(_("籠手によるMPペナルティ読込失敗。ID: '%d'。", "Failed to load glove_mp_penalty. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_bool(class_data["has_magic_fail_rate_cap"], magics_info.has_magic_fail_rate_cap, true)) {
        msg_format(_("最低呪文失率情報読込失敗。ID: '%d'。", "Failed to load magic_fail_rate_cap. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_bool(class_data["is_spell_trainable"], magics_info.is_spell_trainable, true)) {
        msg_format(_("呪文訓練可能性読込失敗。ID: '%d'。", "Failed to load spell_trainability. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(class_data["first_spell_level"], magics_info.spell_first, true, Range(0, 99))) {
        msg_format(_("呪文行使開始レベル読込失敗。ID: '%d'。", "Failed to load spell-first level. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(class_data["armour_weight_limit"], magics_info.spell_weight, true, Range(0, 999))) {
        msg_format(_("MP重量制限値読込失敗。ID: '%d'。", "Failed to load spell-weight value. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = set_realm_data(class_data, magics_info)) {
        msg_format(_("呪文データ読込失敗。ID: '%d'。", "Failed to load spell data. ID: '%d'."), error_idx);
        return err;
    }

    return PARSE_ERROR_NONE;
}
