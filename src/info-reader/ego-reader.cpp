#include "info-reader/ego-reader.h"
#include "artifact/random-art-effects.h"
#include "info-reader/baseitem-tokens-table.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#include <limits>
#include <nlohmann/json.hpp>

EgoReader::EgoReader(const nlohmann::json &ego_data)
    : ego_data(ego_data)
{
}

/*!
 * @brief 百分率形式のエゴ定義のversionを、公開前に検証する
 * @param root JSON文書のルート
 * @return エラーコード。旧分数形式のversion 1は受理しない。
 */
int EgoReader::validate_root(const nlohmann::json &root)
{
    int version;
    return info_set_integer(get_json_value(root, "version"), version, true, Range(FORMAT_VERSION, FORMAT_VERSION));
}

/*!
 * @brief エゴ1件の数値・参照を検証し、成功した場合にだけ定義とerror_idxを公開する
 * @return 成功時はPARSE_ERROR_NONE、失敗時は解析エラーコード
 * @details 失敗時は既存のegos_infoとerror_idxを保持する。文書のversion検証は呼び出し側が先に行う。
 */
int EgoReader::read() const
{
    if (!this->ego_data.is_object()) {
        return PARSE_ERROR_INVALID_TYPE;
    }

    int id;
    if (auto err = info_set_integer(get_json_value(this->ego_data, "id"), id, true, Range(1, std::numeric_limits<byte>::max()))) {
        return err;
    }
    if (egos_info.contains(i2enum<EgoType>(id))) {
        return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
    }

    EgoItemDefinition ego;
    ego.idx = i2enum<EgoType>(id);
    if (const auto result = info_set_string(get_json_value(this->ego_data, "name"), ego.name, true); !result) {
        return result.error();
    }
    if (auto err = info_set_integer(get_json_value(this->ego_data, "slot"), ego.slot, true, Range(INVEN_AMMO, INVEN_FEET))) {
        return err;
    }
    if (auto err = info_set_integer(get_json_value(this->ego_data, "rating"), ego.rating, true, Range(0, 100))) {
        return err;
    }
    if (auto err = info_set_integer(get_json_value(this->ego_data, "level"), ego.level, true, Range(0, 128))) {
        return err;
    }
    if (auto err = info_set_integer(get_json_value(this->ego_data, "rarity"), ego.rarity, true, Range(0, 255))) {
        return err;
    }
    if (auto err = info_set_integer(get_json_value(this->ego_data, "cost"), ego.cost, true, Range(0, std::numeric_limits<PRICE>::max() / 32))) {
        return err;
    }

    const auto &base = get_json_value(this->ego_data, "base_bonuses");
    if (!base.is_null()) {
        if (!base.is_object()) {
            return PARSE_ERROR_INVALID_TYPE;
        }
        if (auto err = info_set_integer(get_json_value(base, "to_hit"), ego.base_to_h, true, Range(-32768, 32767))) {
            return err;
        }
        if (auto err = info_set_integer(get_json_value(base, "to_damage"), ego.base_to_d, true, Range(-32768, 32767))) {
            return err;
        }
        if (auto err = info_set_integer(get_json_value(base, "to_ac"), ego.base_to_a, true, Range(-32768, 32767))) {
            return err;
        }
    }

    const auto &maximum = get_json_value(this->ego_data, "maximum_bonuses");
    if (!maximum.is_null()) {
        if (!maximum.is_object()) {
            return PARSE_ERROR_INVALID_TYPE;
        }
        if (auto err = info_set_integer(get_json_value(maximum, "to_hit"), ego.max_to_h, true, Range(-32768, 32767))) {
            return err;
        }
        if (auto err = info_set_integer(get_json_value(maximum, "to_damage"), ego.max_to_d, true, Range(-32768, 32767))) {
            return err;
        }
        if (auto err = info_set_integer(get_json_value(maximum, "to_ac"), ego.max_to_a, true, Range(-32768, 32767))) {
            return err;
        }
        if (auto err = info_set_integer(get_json_value(maximum, "pval"), ego.max_pval, true, Range(-32768, 32767))) {
            return err;
        }
    }

    if (const auto result = this->set_activation(ego); !result) {
        return result.error();
    }
    if (const auto result = this->set_flags(ego); !result) {
        return result.error();
    }
    if (const auto result = this->set_extra_flags(ego); !result) {
        return result.error();
    }

    egos_info.emplace(ego.idx, std::move(ego));
    error_idx = id;
    return PARSE_ERROR_NONE;
}

bool EgoReader::grab_one_flag(EgoItemDefinition &ego, const nlohmann::json &flag) const
{
    if (!flag.is_string()) {
        return false;
    }
    const auto &name = flag.get_ref<const std::string &>();
    return TrFlags::grab_one_flag(ego.flags, baseitem_flags, name) ||
           EnumClassFlagGroup<ItemGenerationTraitType>::grab_one_flag(ego.gen_flags, baseitem_geneneration_flags, name);
}

bool EgoReader::grab_one_extra_flag(ego_generate_type &extra, const nlohmann::json &flag) const
{
    if (!flag.is_string()) {
        return false;
    }
    const auto &name = flag.get_ref<const std::string &>();
    if (const auto it = baseitem_flags.find(name); it != baseitem_flags.end()) {
        extra.tr_flags.push_back(it->second);
        return true;
    }
    if (const auto it = baseitem_geneneration_flags.find(name); it != baseitem_geneneration_flags.end()) {
        extra.trg_flags.push_back(it->second);
        return true;
    }
    return false;
}

/*!
 * @brief エゴのフラグを検証し、公開前の定義へ格納する
 * @param ego 読み込み途中のエゴ定義
 * @return 成功時は空の値、失敗時は解析エラーコード
 */
tl::expected<void, parse_error_type> EgoReader::set_flags(EgoItemDefinition &ego) const
{
    const auto &flags = get_json_value(this->ego_data, "flags");
    if (!flags.is_array()) {
        if (flags.is_null()) {
            return {};
        }
        return tl::make_unexpected(PARSE_ERROR_INVALID_TYPE);
    }
    for (const auto &flag : flags) {
        if (!this->grab_one_flag(ego, flag)) {
            return tl::make_unexpected(flag.is_string() ? PARSE_ERROR_INVALID_FLAG : PARSE_ERROR_INVALID_TYPE);
        }
    }
    return {};
}

/*!
 * @brief 追加能力の整数百分率とフラグを検証し、公開前のエゴへ格納する
 * @param ego 読み込み途中のエゴ定義
 * @return 成功時は空の値、失敗時は解析エラーコード
 * @details extra_flagsの省略は許可する。旧分数形式・新旧混在、0～100以外の確率を拒否する。
 */
tl::expected<void, parse_error_type> EgoReader::set_extra_flags(EgoItemDefinition &ego) const
{
    const auto &extras = get_json_value(this->ego_data, "extra_flags");
    if (!extras.is_array()) {
        if (extras.is_null()) {
            return {};
        }
        return tl::make_unexpected(PARSE_ERROR_INVALID_TYPE);
    }
    for (const auto &extra_data : extras) {
        if (!extra_data.is_object()) {
            return tl::make_unexpected(PARSE_ERROR_INVALID_TYPE);
        }
        ego_generate_type extra;
        // 旧分数や混在指定を黙って無視せず、整数百分率へ移行させる。
        if (extra_data.contains("numerator") || extra_data.contains("denominator")) {
            return tl::make_unexpected(PARSE_ERROR_INVALID_VALUE);
        }
        if (const auto err = info_set_integer(get_json_value(extra_data, "chance"), extra.chance, true, Range(0, 100)); err != PARSE_ERROR_NONE) {
            return tl::make_unexpected(static_cast<parse_error_type>(err));
        }
        const auto &flags = get_json_value(extra_data, "flags");
        if (!flags.is_array()) {
            return tl::make_unexpected(flags.is_null() ? PARSE_ERROR_TOO_FEW_ARGUMENTS : PARSE_ERROR_INVALID_TYPE);
        }
        if (flags.empty()) {
            return tl::make_unexpected(PARSE_ERROR_TOO_FEW_ARGUMENTS);
        }
        for (const auto &flag : flags) {
            if (!this->grab_one_extra_flag(extra, flag)) {
                return tl::make_unexpected(flag.is_string() ? PARSE_ERROR_INVALID_FLAG : PARSE_ERROR_INVALID_TYPE);
            }
        }
        ego.xtra_flags.push_back(std::move(extra));
    }
    return {};
}

/*!
 * @brief 任意の発動指定を検証し、公開前の定義へ格納する
 * @param ego 読み込み途中のエゴ定義
 * @return 成功時は空の値、失敗時は解析エラーコード
 */
tl::expected<void, parse_error_type> EgoReader::set_activation(EgoItemDefinition &ego) const
{
    const auto &activation = get_json_value(this->ego_data, "activation");
    if (activation.is_null()) {
        return {};
    }
    if (!activation.is_string()) {
        return tl::make_unexpected(PARSE_ERROR_INVALID_TYPE);
    }
    ego.act_idx = grab_one_activation_flag(activation.get<std::string>());
    if (ego.act_idx <= RandomArtActType::NONE) {
        return tl::make_unexpected(PARSE_ERROR_INVALID_FLAG);
    }
    return {};
}
