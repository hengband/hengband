/*
 * @brief ベースアイテム定義の読み込み処理
 * @author Hourier
 * @date 2022/10/10
 */

#include "info-reader/baseitem-reader.h"
#include "artifact/random-art-effects.h"
#include "info-reader/baseitem-tokens-table.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "locale/japanese.h"
#include "main/angband-headers.h"
#include "object-enchant/tr-types.h"
#include "object/tval-types.h"
#include "system/baseitem/baseitem-definition.h"
#include "term/gameterm.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(ベースアイテム用)
 * @param baseitem 保管先ベースアイテムへの参照
 * @param what 参照元の文字列
 * @return 見つけたらtrue
 */
static bool grab_one_baseitem_flag(BaseitemInfo &baseitem, std::string_view what)
{
    if (TrFlags::grab_one_flag(baseitem.flags, baseitem_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<ItemGenerationTraitType>::grab_one_flag(baseitem.gen_flags, baseitem_geneneration_flags, what)) {
        return true;
    }

    msg_format(_("未知のアイテム・フラグ '%s'。", "Unknown object flag '%s'."), what.data());
    return false;
}

/*!
 * @brief JSON Objectからベースアイテムのシンボルをセットする
 * @param symbol_data シンボル情報の格納されたJSON Object
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return エラーコード
 */
static errr set_baseitem_symbol(const nlohmann::json &symbol_data, BaseitemInfo &baseitem)
{
    if (symbol_data.is_null()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto &character_obj = symbol_data["character"];
    if (!character_obj.is_string()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto &color_obj = symbol_data["color"];
    if (!color_obj.is_string()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto color = color_list.find(color_obj.get<std::string>());
    if (color == color_list.end()) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    if (color->second > 127) {
        return PARSE_ERROR_GENERIC;
    }
    baseitem.symbol_definition = DisplaySymbol(color->second, character_obj.get<std::string>().front());

    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからベースアイテム種別をセットする
 * @param baseitem_data ベースアイテム情報の格納されたJSON Object
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return エラーコード
 */
static errr set_baseitem_kind(nlohmann::json &baseitem_data, BaseitemInfo &baseitem)
{
    if (baseitem_data.is_null()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    ItemKindType type_value;
    if (auto err = info_set_integer(baseitem_data["type_value"], type_value, true, Range(0, 128))) {
        return err;
    }
    int subtype_value;
    if (auto err = info_set_integer(baseitem_data["subtype_value"], subtype_value, true, Range(0, 128))) {
        return err;
    }

    baseitem.bi_key = { type_value, subtype_value };
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからベースアイテムのpvalをセットする
 * @param pval_data pval情報の格納されたJSON Object
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return エラーコード
 */
static errr set_baseitem_parameter_value(const nlohmann::json &pval_data, BaseitemInfo &baseitem)
{
    if (auto err = info_set_integer(pval_data, baseitem.pval, false, Range(-9999, 9999))) {
        return err;
    }
    if ((baseitem.bi_key.tval() == ItemKindType::ROD) && (baseitem.pval <= 0)) {
        return PAESE_ERROR_INVALID_PVAL;
    }

    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからアイテムの階層/希少度情報をセットする
 * @param alloc_data 階層/希少度情報の格納されたJSON Object
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return エラーコード
 */
static errr set_baseitem_allocations(nlohmann::json &allocations_data, BaseitemInfo &baseitem)
{
    if (allocations_data.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!allocations_data.is_array()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    for (auto i = 0; auto &element : allocations_data.items()) {
        auto &alloc = element.value();
        auto &table = baseitem.alloc_tables[i];
        if (auto err = info_set_integer(alloc["depth"], table.level, true, Range(0, 128))) {
            return err;
        }
        if (auto err = info_set_integer(alloc["rarity"], table.chance, true, Range(0, 256))) {
            return err;
        }
        i++;
    }
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからベースアイテムの発動能力をセットする
 * @param act_data 発動能力情報の格納されたJSON Object
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return エラーコード
 */
static errr set_baseitem_activate(const nlohmann::json &act_data, BaseitemInfo &baseitem)
{
    if (!act_data.is_string()) {
        return PARSE_ERROR_NONE;
    }

    auto activation = grab_one_activation_flag(act_data.get<std::string>());
    if (activation <= RandomArtActType::NONE) {
        return PARSE_ERROR_INVALID_FLAG;
    }

    baseitem.act_idx = activation;
    baseitem.flags.set(tr_type::TR_ACTIVATE);
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからベースアイテムフラグをセットする
 * @param flag_data ベースアイテムフラグ情報の格納されたJSON Object
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return エラーコード
 */
static errr set_baseitem_flags(const nlohmann::json &flag_data, BaseitemInfo &baseitem)
{
    if (flag_data.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!flag_data.is_array()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    for (auto &flag : flag_data) {
        if (!grab_one_baseitem_flag(baseitem, flag.get<std::string>())) {
            return PARSE_ERROR_INVALID_FLAG;
        }
    }
    return PARSE_ERROR_NONE;
}

/*!
 * @brief ベースアイテム情報(JSON Object)のパース関数
 * @param art_data ベースアイテムデータの格納されたJSON Object
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_baseitems_info(nlohmann::json &item_data, angband_header *)
{
    if (!item_data["id"].is_number_integer()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto item_id = item_data["id"].get<int>();
    if (item_id < error_idx) {
        return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
    }

    auto &baseitems = BaseitemList::get_instance();
    error_idx = item_id;
    if (item_id >= static_cast<int>(baseitems.size())) {
        baseitems.resize(item_id + 1);
    }
    const short short_id = static_cast<short>(item_id);
    auto &baseitem = baseitems.get_baseitem(short_id);
    baseitem.idx = short_id;

    if (auto err = info_set_string(item_data["name"], baseitem.name, true)) {
        msg_format(_("アイテムの名称読込失敗。ID: '%d'。", "Failed to load item name. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_string(item_data["flavor_name"], baseitem.flavor_name, false)) {
        msg_format(_("アイテム未識別名の読込失敗。ID: '%d'。", "Failed to load item unidentified name. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_string(item_data["flavor"], baseitem.text, false)) {
        msg_format(_("アイテムのフレーバーテキスト読込失敗。ID: '%d'。", "Failed to load flavor text of item. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = set_baseitem_symbol(item_data["symbol"], baseitem)) {
        msg_format(_("アイテムのシンボル読込失敗。ID: '%d'。", "Failed to load symbol of item. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = set_baseitem_kind(item_data["itemkind"], baseitem)) {
        msg_format(_("アイテム種別の読込失敗。ID: '%d'。", "Failed to load kind of item. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = set_baseitem_parameter_value(item_data["parameter_value"], baseitem)) {
        msg_format(_("アイテムのパラメータ値読込失敗。ID: '%d'。", "Failed to load prameter value of item. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(item_data["level"], baseitem.level, true, Range(0, 128))) {
        msg_format(_("アイテムのレベル読込失敗。ID: '%d'。", "Failed to load level of item. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(item_data["weight"], baseitem.weight, true, Range(0, 9999))) {
        msg_format(_("アイテムの重量読込失敗。ID: '%d'。", "Failed to load weight of item. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(item_data["cost"], baseitem.cost, true, Range(0, 99999999))) {
        msg_format(_("アイテムの売値読込失敗。ID: '%d'。", "Failed to load cost of item. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(item_data["base_ac"], baseitem.ac, false, Range(-99, 99))) {
        msg_format(_("アイテムのベースAC読込失敗。ID: '%d'。", "Failed to load base AC of item. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_dice(item_data["base_dice"], baseitem.damage_dice, false)) {
        msg_format(_("アイテムのベースダイス読込失敗。ID: '%d'。", "Failed to load base dice of item. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(item_data["hit_bonus"], baseitem.to_h, false, Range(-99, 99))) {
        msg_format(_("アイテムの命中補正値読込失敗。ID: '%d'。", "Failed to load hit bonus of item. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(item_data["damage_bonus"], baseitem.to_d, false, Range(-99, 99))) {
        msg_format(_("アイテムの命中補正値読込失敗。ID: '%d'。", "Failed to load damage bonus of item. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(item_data["ac_bonus"], baseitem.to_a, false, Range(-99, 99))) {
        msg_format(_("アイテムのAC補正値読込失敗。ID: '%d'。", "Failed to load AC bonus of item. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = set_baseitem_allocations(item_data["allocations"], baseitem)) {
        msg_format(_("アイテムの生成情報読込失敗。ID: '%d'。", "Failed to load generation info of item. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = set_baseitem_activate(item_data["activate"], baseitem)) {
        msg_format(_("アイテムの生成情報読込失敗。ID: '%d'。", "Failed to load activation of item. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = set_baseitem_flags(item_data["flags"], baseitem)) {
        msg_format(_("アイテムの生成情報読込失敗。ID: '%d'。", "Failed to load flags of item. ID: '%d'."), error_idx);
        return err;
    }

    return PARSE_ERROR_NONE;
}
