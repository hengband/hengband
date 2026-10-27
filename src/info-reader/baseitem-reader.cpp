/*
 * @brief ベースアイテム定義の読み込み処理
 * @author Hourier
 * @date 2022/10/10
 */

#include "info-reader/baseitem-reader.h"
#include "artifact/random-art-effects.h"
#include "cmd-action/cmd-spell.h"
#include "floor/floor-base-definitions.h"
#include "grid/trap.h"
#include "info-reader/baseitem-tokens-table.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/item-reader-limits.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "locale/japanese.h"
#include "locale/language-switcher.h"
#include "object-enchant/tr-types.h"
#include "object/tval-types.h"
#include "player/digestion-processor.h"
#include "player/player-skill.h"
#include "sv-definition/sv-rod-types.h"
#include "sv-definition/sv-staff-types.h"
#include "sv-definition/sv-wand-types.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/gamevalue.h"
#include "term/gameterm.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#include "view/display-messages.h"
#include <algorithm>
#include <array>
#include <iterator>
#include <limits>
#include <memory>
#include <nlohmann/json.hpp>
#include <utility>

namespace {
// 装備はアーティファクトと同じ補正幅、回数は魔道具術師の取り込みで byte に回らない幅に揃える。
constexpr auto EQUIPMENT_PVAL_MAX = 128;
constexpr auto DEVICE_CHARGES_MAX = 127;
// ロッドは重ねる数を充填時間から制限する。定義の充填時間は仕様として 1,000 までとする。
constexpr auto ROD_PVAL_MAX = 1000;
// エゴと同じ価格上限に揃え、金貨は生成時の額 (最大で基本価格の9倍) を十分小さく保つ。
constexpr auto BASEITEM_COST_MAX = std::numeric_limits<PRICE>::max() / 32;
constexpr auto GOLD_COST_MAX = 1024;

// ItemKindType の列挙子だけを受け付け、空き番号を定義として使わせない。
constexpr ItemKindType DEFINED_ITEM_KINDS[] = {
    ItemKindType::NONE,
    ItemKindType::FLAVOR_SKELETON,
    ItemKindType::BOTTLE,
    ItemKindType::JUNK,
    ItemKindType::WHISTLE,
    ItemKindType::SPIKE,
    ItemKindType::CHEST,
    ItemKindType::FIGURINE,
    ItemKindType::STATUE,
    ItemKindType::MONSTER_REMAINS,
    ItemKindType::CAPTURE,
    ItemKindType::NO_AMMO,
    ItemKindType::SHOT,
    ItemKindType::ARROW,
    ItemKindType::BOLT,
    ItemKindType::BOW,
    ItemKindType::DIGGING,
    ItemKindType::HAFTED,
    ItemKindType::POLEARM,
    ItemKindType::SWORD,
    ItemKindType::BOOTS,
    ItemKindType::GLOVES,
    ItemKindType::HELM,
    ItemKindType::CROWN,
    ItemKindType::SHIELD,
    ItemKindType::CLOAK,
    ItemKindType::SOFT_ARMOR,
    ItemKindType::HARD_ARMOR,
    ItemKindType::DRAG_ARMOR,
    ItemKindType::LITE,
    ItemKindType::AMULET,
    ItemKindType::RING,
    ItemKindType::CARD,
    ItemKindType::STAFF,
    ItemKindType::WAND,
    ItemKindType::ROD,
    ItemKindType::PARCHMENT,
    ItemKindType::SCROLL,
    ItemKindType::POTION,
    ItemKindType::FLASK,
    ItemKindType::FOOD,
    ItemKindType::LIFE_BOOK,
    ItemKindType::SORCERY_BOOK,
    ItemKindType::NATURE_BOOK,
    ItemKindType::CHAOS_BOOK,
    ItemKindType::DEATH_BOOK,
    ItemKindType::TRUMP_BOOK,
    ItemKindType::ARCANE_BOOK,
    ItemKindType::CRAFT_BOOK,
    ItemKindType::DEMON_BOOK,
    ItemKindType::CRUSADE_BOOK,
    ItemKindType::MUSIC_BOOK,
    ItemKindType::HISSATSU_BOOK,
    ItemKindType::HEX_BOOK,
    ItemKindType::GOLD,
};

/*!
 * @brief ベースアイテムの種別ごとの pval の範囲を返す
 * @param key ベースアイテムのキー (種別だけを見る)
 * @return 受け付ける pval の下限と上限
 */
Range pval_range(const BaseitemKey &key)
{
    if (key.is_equipement()) {
        return { -EQUIPMENT_PVAL_MAX, EQUIPMENT_PVAL_MAX };
    }
    switch (key.tval()) {
    case ItemKindType::STAFF:
    case ItemKindType::WAND:
        // 基本回数は価格計算の除数でもあるため、0 を認めない。
        return { 1, DEVICE_CHARGES_MAX };
    case ItemKindType::ROD:
        return { 1, ROD_PVAL_MAX };
    case ItemKindType::FOOD:
        // 1個の滋養は満腹の閾値まで。薬は同じ量を減らすことも認める。
        return { 0, PY_FOOD_FULL };
    case ItemKindType::POTION:
        return { -PY_FOOD_FULL, PY_FOOD_FULL };
    case ItemKindType::FLASK:
        // 空のランタンを満たす量までの正の燃料とする。
        return { 1, FUEL_LAMP };
    case ItemKindType::CHEST:
        // 未解除の罠の表の添え字。0 は空の箱を表す。
        return { 0, static_cast<int>(chest_traps.size()) - 1 };
    default:
        // 定義の pval に意味のない種別は0に統一する。
        return { 0, 0 };
    }
}

/*!
 * @brief ベースアイテムの種別ごとの副種別の上限を返す
 * @param key ベースアイテムのキー (種別だけを見る)
 * @return 受け付ける副種別の上限 (下限は 0)
 */
int subtype_max(const BaseitemKey &key)
{
    if (key.is_weapon()) {
        // 武器熟練度の種別ごとの配列を副種別で引く。
        return static_cast<int>(std::tuple_size_v<decltype(skill_table::w_max)::mapped_type>) - 1;
    }
    if (key.is_spell_book()) {
        // 魔法書の副種別で呪文のビット表を引く。
        return static_cast<int>(std::size(fake_spell_flags)) - 1;
    }
    // 魔道具術師の取り込み先の要素数に合わせる。
    switch (key.tval()) {
    case ItemKindType::STAFF:
        return SV_STAFF_MAX - 1;
    case ItemKindType::WAND:
        return SV_WAND_MAX - 1;
    case ItemKindType::ROD:
        return SV_ROD_MAX - 1;
    default:
        return 128;
    }
}
}

BaseitemReader::Session BaseitemReader::create_session(tl::optional<int> last_id)
{
    return Session(DefinitionIdOrder::STRICTLY_INCREASING, { 0, MAX_BASEITEM_ID }, _("ベースアイテム", "baseitem"), last_id);
}

BaseitemReader::BaseitemReader(const nlohmann::json &baseitem_data, Session &session)
    : baseitem_data(baseitem_data)
    , session(session)
{
}

/*!
 * @brief ベースアイテム情報(JSON Object)のパース関数
 * @note BaseitemReader が保持する baseitem_data を参照する
 * @return 成功時は空の値、失敗時は解析エラーコード
 */
tl::expected<void, ParseErrorType> BaseitemReader::read() const
{
    if (!this->baseitem_data.is_object()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }

    const auto id_result = this->session.read(get_json_value(this->baseitem_data, "id"));
    if (!id_result) {
        msg_print(id_result.error().reason);
        return tl::unexpected(id_result.error().code);
    }
    const auto item_id = *id_result;

    auto &baseitems = BaseitemList::get_instance();
    const short short_id = static_cast<short>(item_id);
    auto baseitem = item_id < static_cast<int>(baseitems.size()) ? baseitems.get_baseitem(short_id).clone_without_required_fields() : BaseitemDefinition{};
    if (const auto result = info_set_string(get_json_value(this->baseitem_data, "name"), baseitem.name, true); !result) {
        msg_print(_("アイテムの名称読込失敗。", "Failed to load item name."));
        return result;
    }
    if (const auto result = info_set_string(get_json_value(this->baseitem_data, "flavor_name"), baseitem.flavor_name, false); !result) {
        msg_print(_("アイテム未識別名の読込失敗。", "Failed to load item unidentified name."));
        return result;
    }
    if (const auto result = info_set_string(get_json_value(this->baseitem_data, "flavor"), baseitem.text, false); !result) {
        msg_print(_("アイテムのフレーバーテキスト読込失敗。", "Failed to load flavor text of item."));
        return result;
    }
    if (const auto result = this->set_baseitem_symbol(baseitem); !result) {
        msg_print(_("アイテムのシンボル読込失敗。", "Failed to load symbol of item."));
        return result;
    }
    if (const auto result = this->set_baseitem_kind(baseitem); !result) {
        msg_print(_("アイテム種別の読込失敗。", "Failed to load kind of item."));
        return result;
    }
    if (const auto result = this->set_baseitem_parameter_value(baseitem); !result) {
        msg_print(_("アイテムのパラメータ値読込失敗。", "Failed to load parameter value of item."));
        return result;
    }
    // level は基本の生成階 (発動の基本難易度などにも使う) で、たどり着ける最深の階 (MAX_DEPTH - 1) までとする。
    if (const auto result = info_set_integer(get_json_value(this->baseitem_data, "level"), baseitem.level, true, Range(0, MAX_DEPTH - 1)); !result) {
        msg_print(_("アイテムのレベル読込失敗。", "Failed to load level of item."));
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(this->baseitem_data, "weight"), baseitem.weight, true, Range(0, 9999)); !result) {
        msg_print(_("アイテムの重量読込失敗。", "Failed to load weight of item."));
        return result;
    }
    const auto cost_max = baseitem.bi_key.tval() == ItemKindType::GOLD ? GOLD_COST_MAX : BASEITEM_COST_MAX;
    if (const auto result = info_set_integer(get_json_value(this->baseitem_data, "cost"), baseitem.cost, true, Range(0, cost_max)); !result) {
        msg_print(_("アイテムの売値読込失敗。", "Failed to load cost of item."));
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(this->baseitem_data, "base_ac"), baseitem.ac, false, Range(0, ITEM_BASE_AC_MAX)); !result) {
        msg_print(_("アイテムのベースAC読込失敗。", "Failed to load base AC of item."));
        return result;
    }
    // セーブではダイスの個数・面数をそれぞれ1バイトで書くため、0〜255に限る。
    if (const auto result = info_set_dice(get_json_value(this->baseitem_data, "base_dice"), baseitem.damage_dice, false, Range(0, 255)); !result) {
        msg_print(_("アイテムのベースダイス読込失敗。", "Failed to load base dice of item."));
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(this->baseitem_data, "hit_bonus"), baseitem.to_h, false, Range(-ITEM_BONUS_MAX, ITEM_BONUS_MAX)); !result) {
        msg_print(_("アイテムの命中補正値読込失敗。", "Failed to load hit bonus of item."));
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(this->baseitem_data, "damage_bonus"), baseitem.to_d, false, Range(-ITEM_BONUS_MAX, ITEM_BONUS_MAX)); !result) {
        msg_print(_("アイテムのダメージ補正値読込失敗。", "Failed to load damage bonus of item."));
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(this->baseitem_data, "ac_bonus"), baseitem.to_a, false, Range(-ITEM_BONUS_MAX, ITEM_BONUS_MAX)); !result) {
        msg_print(_("アイテムのAC補正値読込失敗。", "Failed to load AC bonus of item."));
        return result;
    }
    if (const auto result = this->set_baseitem_allocations(baseitem); !result) {
        msg_print(_("アイテムの生成情報読込失敗。", "Failed to load generation info of item."));
        return result;
    }
    if (const auto result = this->set_baseitem_activate(baseitem); !result) {
        msg_print(_("アイテムの発動能力読込失敗。", "Failed to load activation of item."));
        return result;
    }
    if (const auto result = this->set_baseitem_flags(baseitem); !result) {
        msg_print(_("アイテムのフラグ読込失敗。", "Failed to load flags of item."));
        return result;
    }
    if (const auto result = this->set_baseitem_natural_generation(baseitem); !result) {
        msg_print(_("アイテムの自然生成の指定の読込失敗。", "Failed to load natural generation of item."));
        return result;
    }

    // 自然生成しないベースアイテムは生成テーブルを持たない (生成の側は natural_generation を見ず、生成テーブルの有無だけで決まる)。
    // 同じ ID を読み直したときに引き継いだ生成テーブルも含めて判定するため、読み込んだ後の定義で確かめる。
    if (!baseitem.natural_generation && baseitem.has_allocation()) {
        msg_print(_("自然生成しないアイテムに生成情報がある。", "Item without natural generation has generation info."));
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }

    // 種別・副種別から ID を引く表では後の ID が先の ID を隠し、先のベースアイテムが店の品揃えや
    // 初期の持ち物などで使われなくなるため、有効なベースアイテムどうしの重複は拒否する。
    const auto previous_end = baseitems.begin() + std::min(static_cast<size_t>(item_id), baseitems.size());
    if (baseitem.is_valid()) {
        const auto duplicated = std::find_if(baseitems.begin(), previous_end, [&baseitem](const BaseitemDefinition &previous) {
            return previous.is_valid() && (previous.bi_key == baseitem.bi_key);
        });
        if (duplicated != previous_end) {
            const auto duplicated_id = std::distance(baseitems.begin(), duplicated);
            msg_print(_("ベースアイテムの種別・副種別が重複している。重複先ID: '{}'。", "Duplicated kind of baseitem. duplicated ID: '{}'."), duplicated_id);
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }
    }

    if (item_id >= static_cast<int>(baseitems.size())) {
        baseitems.resize(item_id + 1);
    }
    baseitems.replace_baseitem(short_id, std::move(baseitem));
    return {};
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
 * @return 成功、またはパースエラー
 */
tl::expected<void, ParseErrorType> BaseitemReader::set_baseitem_symbol(BaseitemDefinition &baseitem) const
{
    const auto &symbol_data = get_json_value(this->baseitem_data, "symbol");
    if (symbol_data.is_null()) {
        return tl::unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }
    if (!symbol_data.is_object()) {
        return tl::unexpected(ParseErrorType::INVALID_TYPE);
    }

    const auto &character_obj = get_json_value(symbol_data, "character");
    if (!character_obj.is_string()) {
        return tl::unexpected(character_obj.is_null() ? ParseErrorType::TOO_FEW_ARGUMENTS : ParseErrorType::INVALID_TYPE);
    }
    const auto &character = character_obj.get_ref<const std::string &>();
    if (character.size() != 1) {
        return tl::unexpected(ParseErrorType::INVALID_VALUE);
    }

    const auto &color_obj = get_json_value(symbol_data, "color");
    if (!color_obj.is_string()) {
        return tl::unexpected(color_obj.is_null() ? ParseErrorType::TOO_FEW_ARGUMENTS : ParseErrorType::INVALID_TYPE);
    }

    const auto color = color_list.find(color_obj.get<std::string>());
    if (color == color_list.end()) {
        return tl::unexpected(ParseErrorType::INVALID_FLAG);
    }
    if (color->second > 127) {
        return tl::unexpected(ParseErrorType::INVALID_VALUE);
    }

    baseitem.init_symbol(DisplaySymbol(color->second, character.front()));
    return {};
}

/*!
 * @brief JSON Objectからベースアイテム種別をセットする
 * @note BaseitemReader が保持する baseitem_data を参照する
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return 成功、またはパースエラー
 */
tl::expected<void, ParseErrorType> BaseitemReader::set_baseitem_kind(BaseitemDefinition &baseitem) const
{
    const auto &itemkind_data = get_json_value(this->baseitem_data, "itemkind");
    if (itemkind_data.is_null()) {
        return tl::unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }
    if (!itemkind_data.is_object()) {
        return tl::unexpected(ParseErrorType::INVALID_TYPE);
    }

    // 失敗時は直後に返るが、gcc の最適化で未初期化と誤検出されないよう初期化しておく。
    ItemKindType type_value{};
    // 種別をそのまま添え字にする色の表は128要素。
    if (const auto result = info_set_integer(get_json_value(itemkind_data, "type_value"), type_value, true, Range(0, 127)); !result) {
        return result;
    }
    if (std::find(std::begin(DEFINED_ITEM_KINDS), std::end(DEFINED_ITEM_KINDS), type_value) == std::end(DEFINED_ITEM_KINDS)) {
        return tl::unexpected(ParseErrorType::INVALID_VALUE);
    }
    int subtype_value = 0;
    if (const auto result = info_set_integer(get_json_value(itemkind_data, "subtype_value"), subtype_value, true, Range(0, subtype_max({ type_value, 0 }))); !result) {
        return result;
    }

    baseitem.bi_key = { type_value, subtype_value };
    return {};
}

/*!
 * @brief JSON Objectからベースアイテムのpvalをセットする
 * @note BaseitemReader が保持する baseitem_data を参照する
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return 成功、またはパースエラー
 */
tl::expected<void, ParseErrorType> BaseitemReader::set_baseitem_parameter_value(BaseitemDefinition &baseitem) const
{
    const auto &pval_data = get_json_value(this->baseitem_data, "parameter_value");
    const auto range = pval_range(baseitem.bi_key);
    auto pval = static_cast<int>(baseitem.pval);
    if (const auto result = info_set_integer(pval_data, pval, false); !result) {
        return result;
    }
    // 省略・nullでは既存の値（新規なら0）を保つため、種別を変えた再読込もここで検査する。
    if (pval < range.first || pval > range.second) {
        return tl::unexpected(ParseErrorType::INVALID_PVAL);
    }

    baseitem.pval = static_cast<short>(pval);
    return {};
}

/*!
 * @brief JSON Objectからアイテムの階層/希少度情報をセットする
 * @note BaseitemReader が保持する baseitem_data を参照する
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return 成功、またはパースエラー
 */
tl::expected<void, ParseErrorType> BaseitemReader::set_baseitem_allocations(BaseitemDefinition &baseitem) const
{
    const auto &allocations_data = get_json_value(this->baseitem_data, "allocations");
    if (allocations_data.is_null()) {
        return {};
    }
    if (!allocations_data.is_array()) {
        return tl::unexpected(ParseErrorType::INVALID_TYPE);
    }

    for (auto i = 0U; const auto &alloc : allocations_data) {
        if (i >= baseitem.alloc_tables.size()) {
            return tl::unexpected(ParseErrorType::INVALID_VALUE);
        }

        if (!alloc.is_object()) {
            return tl::unexpected(ParseErrorType::INVALID_TYPE);
        }

        auto &table = baseitem.alloc_tables[i];
        // 出現階層は、生成の表を作るときに階層ごとの配列 (要素数 MAX_DEPTH) の添え字に使われる
        if (const auto result = info_set_integer(get_json_value(alloc, "depth"), table.level, true, Range(0, MAX_DEPTH - 1)); !result) {
            return result;
        }
        // 生成の重みは 100÷希少度 の整数除算なので、101 以上はどれも 0 になり、0 は欄が無いのと同じに扱われる。
        // どちらも書く意味が無いため 1〜100 に限る (自然生成させないベースアイテムは natural_generation: false で表す)
        if (const auto result = info_set_integer(get_json_value(alloc, "rarity"), table.chance, true, Range(1, 100)); !result) {
            return result;
        }
        i++;
    }
    return {};
}

/*!
 * @brief JSON Objectからベースアイテムの発動能力をセットする
 * @note BaseitemReader が保持する baseitem_data を参照する
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return 成功、またはパースエラー
 */
tl::expected<void, ParseErrorType> BaseitemReader::set_baseitem_activate(BaseitemDefinition &baseitem) const
{
    const auto &act_data = get_json_value(this->baseitem_data, "activate");
    if (act_data.is_null()) {
        return {};
    }
    if (!act_data.is_string()) {
        return tl::unexpected(ParseErrorType::INVALID_TYPE);
    }

    const auto activation = grab_one_activation_flag(act_data.get<std::string>());
    if (activation <= RandomArtActType::NONE) {
        return tl::unexpected(ParseErrorType::INVALID_FLAG);
    }

    baseitem.act_idx = activation;
    baseitem.flags.set(tr_type::TR_ACTIVATE);
    return {};
}

/*!
 * @brief JSON Objectからベースアイテムの自然生成の指定をセットする
 * @note BaseitemReader が保持する baseitem_data を参照する
 * @details 省略時は既存の値 (新しい定義なら true) のままにする。null は省略と同じ扱い (ほかの省略可能な項目と同じ)。
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return 成功、またはパースエラー
 */
tl::expected<void, ParseErrorType> BaseitemReader::set_baseitem_natural_generation(BaseitemDefinition &baseitem) const
{
    const auto &natural_generation_data = get_json_value(this->baseitem_data, "natural_generation");
    if (natural_generation_data.is_null()) {
        return {};
    }
    if (!natural_generation_data.is_boolean()) {
        return tl::unexpected(ParseErrorType::INVALID_TYPE);
    }

    baseitem.natural_generation = natural_generation_data.get<bool>();
    return {};
}

/*!
 * @brief JSON Objectからベースアイテムフラグをセットする
 * @note BaseitemReader が保持する baseitem_data を参照する
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return 成功、またはパースエラー
 */
tl::expected<void, ParseErrorType> BaseitemReader::set_baseitem_flags(BaseitemDefinition &baseitem) const
{
    const auto &flag_data = get_json_value(this->baseitem_data, "flags");
    if (flag_data.is_null()) {
        return {};
    }
    if (!flag_data.is_array()) {
        return tl::unexpected(ParseErrorType::INVALID_TYPE);
    }

    for (auto &flag : flag_data) {
        if (!flag.is_string()) {
            return tl::unexpected(ParseErrorType::INVALID_TYPE);
        }
        if (!this->grab_one_baseitem_flag(baseitem, flag.get<std::string>())) {
            return tl::unexpected(ParseErrorType::INVALID_FLAG);
        }
    }
    return {};
}
