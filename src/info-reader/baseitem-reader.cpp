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
#include "object-enchant/tr-types.h"
#include "object/tval-types.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "term/gameterm.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include <limits>

BaseitemReader::BaseitemReader(const nlohmann::json &baseitem_data)
    : baseitem_data(baseitem_data)
{
}

/*!
 * @brief ベースアイテム情報(JSON Object)のパース関数
 * @note BaseitemReader が保持する baseitem_data を参照する
 * @return エラーコード
 */
int BaseitemReader::read() const
{
    if (!this->baseitem_data.is_object()) {
        return PARSE_ERROR_INVALID_TYPE;
    }

    const auto &id_data = get_json_value(this->baseitem_data, "id");
    if (id_data.is_null()) {
        msg_print(_("ベースアイテムIDの読込失敗。", "Failed to load baseitem ID."));
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    if (!id_data.is_number_integer()) {
        msg_print(_("ベースアイテムIDの読込失敗。", "Failed to load baseitem ID."));
        return PARSE_ERROR_INVALID_TYPE;
    }

    const auto item_id = id_data.get<int>();
    if (item_id <= error_idx) {
        msg_print(_("ベースアイテムIDが非連番です。ID: '{}'、直前ID: '{}'。", "Baseitem ID is not sequential. ID: '{}', previous ID: '{}'."), item_id, error_idx);
        return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
    }
    if (item_id > std::numeric_limits<short>::max()) {
        msg_print(_("ベースアイテムIDが範囲外です。ID: '{}'。", "Baseitem ID is out of bounds. ID: '{}'."), item_id);
        return PARSE_ERROR_OUT_OF_BOUNDS;
    }

    auto &baseitems = BaseitemList::get_instance();
    error_idx = item_id;
    if (item_id >= static_cast<int>(baseitems.size())) {
        baseitems.resize(item_id + 1);
    }
    const short short_id = static_cast<short>(item_id);
    auto &baseitem = baseitems.get_baseitem(short_id);
    if (auto err = info_set_string(get_json_value(this->baseitem_data, "name"), baseitem.name, true)) {
        msg_print(_("アイテムの名称読込失敗。ID: '{}'。", "Failed to load item name. ID: '{}'."), error_idx);
        return err;
    }
    if (auto err = info_set_string(get_json_value(this->baseitem_data, "flavor_name"), baseitem.flavor_name, false)) {
        msg_print(_("アイテム未識別名の読込失敗。ID: '{}'。", "Failed to load item unidentified name. ID: '{}'."), error_idx);
        return err;
    }
    if (auto err = info_set_string(get_json_value(this->baseitem_data, "flavor"), baseitem.text, false)) {
        msg_print(_("アイテムのフレーバーテキスト読込失敗。ID: '{}'。", "Failed to load flavor text of item. ID: '{}'."), error_idx);
        return err;
    }
    if (auto err = this->set_baseitem_symbol(baseitem)) {
        msg_print(_("アイテムのシンボル読込失敗。ID: '{}'。", "Failed to load symbol of item. ID: '{}'."), error_idx);
        return err;
    }
    if (auto err = this->set_baseitem_kind(baseitem)) {
        msg_print(_("アイテム種別の読込失敗。ID: '{}'。", "Failed to load kind of item. ID: '{}'."), error_idx);
        return err;
    }
    if (auto err = this->set_baseitem_parameter_value(baseitem)) {
        msg_print(_("アイテムのパラメータ値読込失敗。ID: '{}'。", "Failed to load parameter value of item. ID: '{}'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(get_json_value(this->baseitem_data, "level"), baseitem.level, true, Range(0, 128))) {
        msg_print(_("アイテムのレベル読込失敗。ID: '{}'。", "Failed to load level of item. ID: '{}'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(get_json_value(this->baseitem_data, "weight"), baseitem.weight, true, Range(0, 9999))) {
        msg_print(_("アイテムの重量読込失敗。ID: '{}'。", "Failed to load weight of item. ID: '{}'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(get_json_value(this->baseitem_data, "cost"), baseitem.cost, true, Range(0, 99999999))) {
        msg_print(_("アイテムの売値読込失敗。ID: '{}'。", "Failed to load cost of item. ID: '{}'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(get_json_value(this->baseitem_data, "base_ac"), baseitem.ac, false, Range(-99, 99))) {
        msg_print(_("アイテムのベースAC読込失敗。ID: '{}'。", "Failed to load base AC of item. ID: '{}'."), error_idx);
        return err;
    }
    if (auto err = info_set_dice(get_json_value(this->baseitem_data, "base_dice"), baseitem.damage_dice, false)) {
        msg_print(_("アイテムのベースダイス読込失敗。ID: '{}'。", "Failed to load base dice of item. ID: '{}'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(get_json_value(this->baseitem_data, "hit_bonus"), baseitem.to_h, false, Range(-99, 99))) {
        msg_print(_("アイテムの命中補正値読込失敗。ID: '{}'。", "Failed to load hit bonus of item. ID: '{}'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(get_json_value(this->baseitem_data, "damage_bonus"), baseitem.to_d, false, Range(-99, 99))) {
        msg_print(_("アイテムのダメージ補正値読込失敗。ID: '{}'。", "Failed to load damage bonus of item. ID: '{}'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(get_json_value(this->baseitem_data, "ac_bonus"), baseitem.to_a, false, Range(-99, 99))) {
        msg_print(_("アイテムのAC補正値読込失敗。ID: '{}'。", "Failed to load AC bonus of item. ID: '{}'."), error_idx);
        return err;
    }
    if (auto err = this->set_baseitem_allocations(baseitem)) {
        msg_print(_("アイテムの生成情報読込失敗。ID: '{}'。", "Failed to load generation info of item. ID: '{}'."), error_idx);
        return err;
    }
    if (auto err = this->set_baseitem_activate(baseitem)) {
        msg_print(_("アイテムの発動能力読込失敗。ID: '{}'。", "Failed to load activation of item. ID: '{}'."), error_idx);
        return err;
    }
    if (auto err = this->set_baseitem_flags(baseitem)) {
        msg_print(_("アイテムのフラグ読込失敗。ID: '{}'。", "Failed to load flags of item. ID: '{}'."), error_idx);
        return err;
    }

    return PARSE_ERROR_NONE;
}

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(ベースアイテム用)
 * @param baseitem 保管先ベースアイテムへの参照
 * @param what 参照元の文字列
 * @return 見つけたらtrue
 */
bool BaseitemReader::grab_one_baseitem_flag(BaseitemDefinition &baseitem, std::string_view what) const
{
    if (TrFlags::grab_one_flag(baseitem.flags, baseitem_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<ItemGenerationTraitType>::grab_one_flag(baseitem.gen_flags, baseitem_geneneration_flags, what)) {
        return true;
    }

    msg_print(_("未知のアイテム・フラグ '{}'。", "Unknown object flag '{}'."), what);
    return false;
}

/*!
 * @brief JSON Objectからベースアイテムのシンボルをセットする
 * @note BaseitemReader が保持する baseitem_data を参照する
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return エラーコード
 */
int BaseitemReader::set_baseitem_symbol(BaseitemDefinition &baseitem) const
{
    const auto &symbol_data = get_json_value(this->baseitem_data, "symbol");
    if (symbol_data.is_null()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    if (!symbol_data.is_object()) {
        return PARSE_ERROR_INVALID_TYPE;
    }

    const auto &character_obj = get_json_value(symbol_data, "character");
    if (!character_obj.is_string()) {
        return character_obj.is_null() ? PARSE_ERROR_TOO_FEW_ARGUMENTS : PARSE_ERROR_INVALID_TYPE;
    }
    const auto &character = character_obj.get_ref<const std::string &>();
    if (character.size() != 1) {
        return PARSE_ERROR_INVALID_VALUE;
    }

    const auto &color_obj = get_json_value(symbol_data, "color");
    if (!color_obj.is_string()) {
        return color_obj.is_null() ? PARSE_ERROR_TOO_FEW_ARGUMENTS : PARSE_ERROR_INVALID_TYPE;
    }

    const auto color = color_list.find(color_obj.get<std::string>());
    if (color == color_list.end()) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    if (color->second > 127) {
        return PARSE_ERROR_GENERIC;
    }

    baseitem.init_symbol(DisplaySymbol(color->second, character.front()));
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからベースアイテム種別をセットする
 * @note BaseitemReader が保持する baseitem_data を参照する
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return エラーコード
 */
int BaseitemReader::set_baseitem_kind(BaseitemDefinition &baseitem) const
{
    const auto &itemkind_data = get_json_value(this->baseitem_data, "itemkind");
    if (itemkind_data.is_null()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    if (!itemkind_data.is_object()) {
        return PARSE_ERROR_INVALID_TYPE;
    }

    ItemKindType type_value;
    if (auto err = info_set_integer(get_json_value(itemkind_data, "type_value"), type_value, true, Range(0, 128))) {
        return err;
    }
    int subtype_value;
    if (auto err = info_set_integer(get_json_value(itemkind_data, "subtype_value"), subtype_value, true, Range(0, 128))) {
        return err;
    }

    baseitem.bi_key = { type_value, subtype_value };
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからベースアイテムのpvalをセットする
 * @note BaseitemReader が保持する baseitem_data を参照する
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return エラーコード
 */
int BaseitemReader::set_baseitem_parameter_value(BaseitemDefinition &baseitem) const
{
    const auto &pval_data = get_json_value(this->baseitem_data, "parameter_value");
    if (auto err = info_set_integer(pval_data, baseitem.pval, false, Range(-9999, 9999))) {
        return err;
    }
    if ((baseitem.bi_key.tval() == ItemKindType::ROD) && (baseitem.pval <= 0)) {
        return PARSE_ERROR_INVALID_PVAL;
    }

    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからアイテムの階層/希少度情報をセットする
 * @note BaseitemReader が保持する baseitem_data を参照する
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return エラーコード
 */
int BaseitemReader::set_baseitem_allocations(BaseitemDefinition &baseitem) const
{
    const auto &allocations_data = get_json_value(this->baseitem_data, "allocations");
    if (allocations_data.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!allocations_data.is_array()) {
        return PARSE_ERROR_INVALID_TYPE;
    }

    for (auto i = 0U; const auto &alloc : allocations_data) {
        if (i >= baseitem.alloc_tables.size()) {
            return PARSE_ERROR_OUT_OF_BOUNDS;
        }

        if (!alloc.is_object()) {
            return PARSE_ERROR_INVALID_TYPE;
        }

        auto &table = baseitem.alloc_tables[i];
        if (auto err = info_set_integer(get_json_value(alloc, "depth"), table.level, true, Range(0, 128))) {
            return err;
        }
        if (auto err = info_set_integer(get_json_value(alloc, "rarity"), table.chance, true, Range(0, 256))) {
            return err;
        }
        i++;
    }
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからベースアイテムの発動能力をセットする
 * @note BaseitemReader が保持する baseitem_data を参照する
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return エラーコード
 */
int BaseitemReader::set_baseitem_activate(BaseitemDefinition &baseitem) const
{
    const auto &act_data = get_json_value(this->baseitem_data, "activate");
    if (act_data.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!act_data.is_string()) {
        return PARSE_ERROR_INVALID_TYPE;
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
 * @note BaseitemReader が保持する baseitem_data を参照する
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return エラーコード
 */
int BaseitemReader::set_baseitem_flags(BaseitemDefinition &baseitem) const
{
    const auto &flag_data = get_json_value(this->baseitem_data, "flags");
    if (flag_data.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!flag_data.is_array()) {
        return PARSE_ERROR_INVALID_TYPE;
    }

    for (auto &flag : flag_data) {
        if (!flag.is_string()) {
            return PARSE_ERROR_INVALID_TYPE;
        }
        if (!this->grab_one_baseitem_flag(baseitem, flag.get<std::string>())) {
            return PARSE_ERROR_INVALID_FLAG;
        }
    }
    return PARSE_ERROR_NONE;
}
