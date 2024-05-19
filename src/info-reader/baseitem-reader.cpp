/*
 * @brief ベースアイテム定義の読み込み処理
 * @author Hourier
 * @date 2022/10/10
 */

#include "info-reader/baseitem-reader.h"
#include "artifact/random-art-effects.h"
#include "info-reader/baseitem-tokens-table.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "locale/japanese.h"
#include "main/angband-headers.h"
#include "object-enchant/tr-types.h"
#include "object/tval-types.h"
#include "system/baseitem-info.h"
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
 * @brief JSON Objectからベースアイテム名をセットする
 * @param name_data 名前情報の格納されたJSON Object
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return エラーコード
 */
static errr set_baseitem_name(const nlohmann::json &name_data, BaseitemInfo &baseitem)
{
    if (name_data.is_null()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

#ifdef JP
    const auto &ja_name = name_data.find("ja");
    if (ja_name == name_data.end()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    auto ja_name_sys = utf8_to_sys(ja_name->get<std::string>());
    if (!ja_name_sys) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    baseitem.name = std::move(*ja_name_sys);
#else
    const auto &en_name = name_data.find("en");
    if (en_name == name_data.end()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    baseitem.name = en_name->get<std::string>();
#endif

    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからベースアイテムの未識別名をセットする
 * @param flavor_name_data 未識別名情報の格納されたJSON Object
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return エラーコード
 */
static errr set_baseitem_flavor_name(const nlohmann::json &flavor_name_data, BaseitemInfo &baseitem)
{
    if (flavor_name_data.is_null()) {
        return PARSE_ERROR_NONE;
    }

#ifdef JP
    const auto &ja_name = flavor_name_data.find("ja");
    if (ja_name == flavor_name_data.end()) {
        return PARSE_ERROR_NONE;
    }
    auto ja_name_sys = utf8_to_sys(ja_name->get<std::string>());
    if (!ja_name_sys) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    baseitem.flavor_name = std::move(*ja_name_sys);
#else
    const auto &en_name = flavor_name_data.find("en");
    if (en_name == flavor_name_data.end()) {
        return PARSE_ERROR_NONE;
    }
    baseitem.flavor_name = en_name->get<std::string>();
#endif

    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからベースアイテムのフレーバーをセットする
 * @param flavor_data フレーバー情報の格納されたJSON Object
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return エラーコード
 */
static errr set_baseitem_flavor(const nlohmann::json &flavor_data, BaseitemInfo &baseitem)
{
    if (flavor_data.is_null()) {
        return PARSE_ERROR_NONE;
    }

#ifdef JP
    const auto &ja_flavor = flavor_data.find("ja");
    if (ja_flavor == flavor_data.end()) {
        return PARSE_ERROR_NONE;
    }
    auto ja_flavor_sys = utf8_to_sys(ja_flavor->get<std::string>());
    if (!ja_flavor_sys) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    baseitem.text = std::move(*ja_flavor_sys);
#else
    const auto &en_flavor = flavor_data.find("en");
    if (en_flavor == flavor_data.end()) {
        return PARSE_ERROR_NONE;
    }
    baseitem.text = en_flavor->get<std::string>();
#endif

    return PARSE_ERROR_NONE;
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
    baseitem.cc_def = ColoredChar(color->second, character_obj.get<std::string>().front());

    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからベースアイテム種別をセットする
 * @param baseitem_data ベースアイテム情報の格納されたJSON Object
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return エラーコード
 */
static errr set_baseitem_kind(const nlohmann::json &baseitem_data, BaseitemInfo &baseitem)
{
    if (baseitem_data.is_null()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto &item_type = baseitem_data.find("type_value");
    if (item_type == baseitem_data.end()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    if (!item_type->is_number_integer()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    const auto type_value = item_type->get<int>();
    if (type_value < 0 || type_value > 128) {
        return PARSE_ERROR_INVALID_FLAG;
    }

    const auto &item_subtype = baseitem_data.find("subtype_value");
    if (item_subtype == baseitem_data.end()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    if (!item_subtype->is_number_integer()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    const auto subtype_value = item_subtype->get<int>();
    if (subtype_value < 0 || subtype_value > 128) {
        return PARSE_ERROR_INVALID_FLAG;
    }

    baseitem.bi_key = { i2enum<ItemKindType>(type_value), subtype_value };
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
    if (pval_data.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!pval_data.is_number_integer()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto parameter_value = pval_data.get<short>();
    if (parameter_value < -9999 || parameter_value > 9999) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    if ((baseitem.bi_key.tval() == ItemKindType::ROD) && (parameter_value <= 0)) {
        return PAESE_ERROR_INVALID_PVAL;
    }

    baseitem.pval = parameter_value;
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからベースアイテムのレベルをセットする
 * @param level_data レベル情報の格納されたJSON Object
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return エラーコード
 */
static errr set_baseitem_level(const nlohmann::json &level_data, BaseitemInfo &baseitem)
{
    if (level_data.is_null()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    if (!level_data.is_number_integer()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto level = level_data.get<int>();
    if (level < 0 || level > 128) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    baseitem.level = level;
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからベースアイテムの重量をセットする
 * @param weight_data weight情報の格納されたJSON Object
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return エラーコード
 */
static errr set_baseitem_weight(const nlohmann::json &weight_data, BaseitemInfo &baseitem)
{
    if (weight_data.is_null()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    if (!weight_data.is_number_integer()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto weight = weight_data.get<int>();
    if (weight < 0 || weight > 9999) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    baseitem.weight = weight;
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからベースアイテムの売値をセットする
 * @param cost_data cost情報の格納されたJSON Object
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return エラーコード
 */
static errr set_baseitem_cost(const nlohmann::json &cost_data, BaseitemInfo &baseitem)
{
    if (cost_data.is_null()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    if (!cost_data.is_number_integer()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto cost = cost_data.get<int>();
    if (cost < 0 || cost > 99999999) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    baseitem.cost = cost;
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON ObjectからベースアイテムのベースACをセットする
 * @param ac_data ac情報の格納されたJSON Object
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return エラーコード
 */
static errr set_baseitem_base_ac(const nlohmann::json &ac_data, BaseitemInfo &baseitem)
{
    if (ac_data.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!ac_data.is_number_integer()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto base_ac = ac_data.get<short>();
    if (base_ac < -99 || base_ac > 99) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    baseitem.ac = base_ac;
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからベースアイテムのベースダイスをセットする
 * @param dice_data ac情報の格納されたJSON Object
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return エラーコード
 */
static errr set_baseitem_base_dice(const nlohmann::json &dice_data, BaseitemInfo &baseitem)
{
    if (!dice_data.is_string()) {
        return PARSE_ERROR_NONE;
    }

    const auto &dice = str_split(dice_data.get<std::string>(), 'd', false, 2);
    if (dice.size() < 2) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    baseitem.dd = std::stoi(dice[0]);
    baseitem.ds = std::stoi(dice[1]);
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからベースアイテムの命中補正値をセットする
 * @param hit_bonus_data 命中補正値情報の格納されたJSON Object
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return エラーコード
 */
static errr set_baseitem_hit_bonus(const nlohmann::json &hit_bonus_data, BaseitemInfo &baseitem)
{
    if (hit_bonus_data.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!hit_bonus_data.is_number_integer()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto hit_bonus = hit_bonus_data.get<short>();
    if (hit_bonus < -99 || hit_bonus > 99) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    baseitem.to_h = hit_bonus;
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからベースアイテムのダメージ補正値をセットする
 * @param damage_bonus_data ダメージ補正値情報の格納されたJSON Object
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return エラーコード
 */
static errr set_baseitem_damage_bonus(const nlohmann::json &damage_bonus_data, BaseitemInfo &baseitem)
{
    if (damage_bonus_data.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!damage_bonus_data.is_number_integer()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto damage_bonus = damage_bonus_data.get<int>();
    if (damage_bonus < -99 || damage_bonus > 99) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    baseitem.to_d = damage_bonus;
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON ObjectからベースアイテムのAC補正値をセットする
 * @param ac_bonus_data AC補正値情報の格納されたJSON Object
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return エラーコード
 */
static errr set_baseitem_ac_bonus(const nlohmann::json &ac_bonus_data, BaseitemInfo &baseitem)
{
    if (ac_bonus_data.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!ac_bonus_data.is_number_integer()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto ac_bonus = ac_bonus_data.get<short>();
    if (ac_bonus < -99 || ac_bonus > 99) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    baseitem.to_a = ac_bonus;
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからアイテムの階層/希少度情報をセットする
 * @param alloc_data 階層/希少度情報の格納されたJSON Object
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return エラーコード
 */
static errr set_baseitem_allocations(const nlohmann::json &allocations_data, BaseitemInfo &baseitem)
{
    if (allocations_data.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!allocations_data.is_array()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    int i = 0;
    for (auto &element : allocations_data.items()) {
        auto &alloc = element.value();
        const auto &depth = alloc.find("depth");
        const auto &rarity = alloc.find("rarity");
        if (depth == alloc.end()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }
        if (rarity == alloc.end()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }
        if (!depth->is_number_integer()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }
        if (!rarity->is_number_integer()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto depth_int = depth->get<int>();
        const auto rarity_short = rarity->get<short>();
        if (depth_int < 0 || depth_int > 128) {
            return PARSE_ERROR_INVALID_FLAG;
        }
        if (rarity_short < 0 || rarity_short > 256) {
            return PARSE_ERROR_INVALID_FLAG;
        }

        auto &table = baseitem.alloc_tables[i];
        table.level = depth_int;
        table.chance = rarity_short;
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

    if (auto err = set_baseitem_name(item_data["name"], baseitem)) {
        msg_format(_("アイテムの名称読込失敗。ID: '%d'。", "Failed to load item name. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = set_baseitem_flavor_name(item_data["flavor_name"], baseitem)) {
        msg_format(_("アイテム未識別名の読込失敗。ID: '%d'。", "Failed to load item unidentified name. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = set_baseitem_flavor(item_data["flavor"], baseitem)) {
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
    if (auto err = set_baseitem_level(item_data["level"], baseitem)) {
        msg_format(_("アイテムのレベル読込失敗。ID: '%d'。", "Failed to load level of item. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = set_baseitem_weight(item_data["weight"], baseitem)) {
        msg_format(_("アイテムの重量読込失敗。ID: '%d'。", "Failed to load weight of item. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = set_baseitem_cost(item_data["cost"], baseitem)) {
        msg_format(_("アイテムの売値読込失敗。ID: '%d'。", "Failed to load cost of item. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = set_baseitem_base_ac(item_data["base_ac"], baseitem)) {
        msg_format(_("アイテムのベースAC読込失敗。ID: '%d'。", "Failed to load base AC of item. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = set_baseitem_base_dice(item_data["base_dice"], baseitem)) {
        msg_format(_("アイテムのベースダイス読込失敗。ID: '%d'。", "Failed to load base dice of item. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = set_baseitem_hit_bonus(item_data["hit_bonus"], baseitem)) {
        msg_format(_("アイテムの命中補正値読込失敗。ID: '%d'。", "Failed to load hit bonus of item. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = set_baseitem_damage_bonus(item_data["damage_bonus"], baseitem)) {
        msg_format(_("アイテムの命中補正値読込失敗。ID: '%d'。", "Failed to load damage bonus of item. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = set_baseitem_ac_bonus(item_data["ac_bonus"], baseitem)) {
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
