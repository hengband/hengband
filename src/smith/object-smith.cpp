#include "smith/object-smith.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-flags.h"
#include "object-enchant/tr-types.h"
#include "object/item-tester-hooker.h"
#include "object/object-flags.h"
#include "perception/object-perception.h"
#include "player-base/player-class.h"
#include "player-info/smith-data-type.h"
#include "smith/smith-info.h"
#include "smith/smith-tables.h"
#include "smith/smith-types.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include <algorithm>
#include <optional>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace {

/**
 * @brief 所持しているエッセンスで付与可能な回数を取得する
 *
 * @param essence_list 使用するエッセンスのリスト
 * @param consumption 1種あたりに使用する消費量
 * @return 所持しているエッセンスで付与可能な回数を返す
 */
int addable_count(smith_data_type *smith_data, std::vector<SmithEssenceType> essence_list, int consumption)
{
    if (consumption <= 0) {
        return 0;
    }

    std::vector<int> addable_count;
    for (auto essence : essence_list) {
        int own_amount = smith_data->essences[essence];
        addable_count.push_back(own_amount / consumption);
    }
    return *std::min_element(addable_count.begin(), addable_count.end());
}

}

/*!
 * @brief 鍛冶クラスコンストラクタ
 */
Smith::Smith(PlayerType *player_ptr)
    : player_ptr(player_ptr)
    , smith_data(PlayerClass(player_ptr).get_specific_data<smith_data_type>())
{
}

/*!
 * @brief 引数で指定した鍛冶効果の鍛冶情報を得る
 *
 * @param effect 情報を得る鍛冶効果
 * @return 鍛冶情報構造体へのポインタを保持する std::optional オブジェクトを返す
 */
std::optional<const ISmithInfo *> Smith::find_smith_info(SmithEffectType effect)
{
    // 何度も呼ぶので線形探索を避けるため鍛冶効果から鍛冶情報のテーブルを引けるmapを作成しておく。
    static std::unordered_map<SmithEffectType, const ISmithInfo *> search_map;
    if (search_map.empty()) {
        for (const auto &info : smith_info_table) {
            search_map.emplace(info->effect, info.get());
        }
    }

    auto it = search_map.find(effect);
    if (it == search_map.end()) {
        return std::nullopt;
    }

    return it->second;
}

/*!
 * @brief 指定したエッセンスの表記名を取得する
 *
 * @param essence 表記名を取得するエッセンス
 * @return 表記名を表す文字列へのポインタ
 */
concptr Smith::get_essence_name(SmithEssenceType essence)
{
    auto it = essence_to_name.find(essence);
    auto essence_name = it != essence_to_name.end() ? it->second : _("不明", "Unknown");
    return essence_name;
}

/**
 * @brief 指定した鍛冶効果の表記名を取得する
 *
 * @param effect 表記名を取得する鍛冶効果
 * @return 表記名を表す文字列へのポインタ
 */
concptr Smith::get_effect_name(SmithEffectType effect)
{
    auto info = find_smith_info(effect);
    if (!info.has_value()) {
        return _("不明", "Unknown");
    }

    return info.value()->name;
}

/*!
 * @brief 指定した鍛冶効果の付与に必要なエッセンスの表記を取得する。複数のエッセンスが必要な場合は "+" で連結されたものとなる。
 *
 * @param effect 鍛冶効果
 * @return 鍛冶効果の付与に必要なエッセンスを表記する std::string オブジェクト
 */
std::string Smith::get_need_essences_desc(SmithEffectType effect)
{
    auto info = find_smith_info(effect);
    if (!info.has_value() || info.value()->need_essences.empty()) {
        return _("不明", "Unknown");
    }

    const auto &need_essences = info.value()->need_essences;
    std::stringstream ss;
    for (auto i = 0U; i < need_essences.size(); i++) {
        ss << Smith::get_essence_name(need_essences[i]);
        if (i < need_essences.size() - 1) {
            ss << _("+", " + ");
        }
    }

    return ss.str();
}

/*!
 * @brief 鍛冶効果を付与するのに必要なエッセンスのリストを返す
 *
 * @param effect 鍛冶効果
 * @return 鍛冶効果を付与するのに必要なエッセンスのリスト
 */
std::vector<SmithEssenceType> Smith::get_need_essences(SmithEffectType effect)
{
    auto info = find_smith_info(effect);
    if (!info.has_value()) {
        return {};
    }

    return info.value()->need_essences;
}

/*!
 * @brief 鍛冶効果を付与する時のエッセンス消費量を取得する
 * 複数のエッセンスを消費する鍛冶効果の場合は全てのエッセンスからこの量が消費される
 * アイテムが複数ある場合はアイテムの個数倍の消費量となる
 *
 * @param effect 鍛冶効果
 * @param o_ptr 鍛冶効果を付与するアイテムへのポインタ。nullptrの場合はデフォルトの消費量が返される。
 * @return 鍛冶効果を付与する時のエッセンス消費量
 */
int Smith::get_essence_consumption(SmithEffectType effect, const ItemEntity *o_ptr)
{
    auto info = find_smith_info(effect);
    if (!info.has_value()) {
        return 0;
    }

    auto consumption = info.value()->consumption;
    if (o_ptr == nullptr) {
        return consumption;
    }

    if ((o_ptr->tval >= ItemKindType::SHOT) && (o_ptr->tval <= ItemKindType::BOLT)) {
        consumption = (consumption + 9) / 10;
    }

    consumption *= o_ptr->number;

    return consumption;
}

/*!
 * @brief 鍛冶効果の対象となるアイテムを絞り込むための ItemTester クラスを取得する
 *
 * @param effect 鍛冶効果
 * @return ItemTesterクラスのオブジェクトをstd::unique_ptrに格納したものを返す
 */
std::unique_ptr<ItemTester> Smith::get_item_tester(SmithEffectType effect)
{
    auto info = find_smith_info(effect);
    if (!info.has_value()) {
        return std::make_unique<TvalItemTester>(ItemKindType::NONE);
    }

    auto tester_func = [i = info.value()](const ItemEntity *o_ptr) {
        return i->can_give_smith_effect(o_ptr);
    };
    return std::make_unique<FuncItemTester>(tester_func);
}

/*!
 * @brief 鍛冶効果により得られる特性フラグを取得する
 *
 * @param effect 鍛冶効果
 * @return 鍛冶効果により得られる特性フラグがONになったTrFlagsオブジェクト
 */
TrFlags Smith::get_effect_tr_flags(SmithEffectType effect)
{
    auto info = find_smith_info(effect);
    if (!info.has_value()) {
        return {};
    }

    return info.value()->tr_flags();
}

/*!
 * @brief アイテムに付与されている発動効果の発動IDを得る
 *
 * @param o_ptr アイテム構造体へのポインタ
 * @return アイテムに付与されている発動効果の発動ID(random_art_activation_type型)
 * 付与されている発動効果が無い場合は std::nullopt
 */
std::optional<RandomArtActType> Smith::object_activation(const ItemEntity *o_ptr)
{
    return o_ptr->smith_act_idx;
}

/*!
 * @brief アイテムに付与されている鍛冶効果を取得する
 *
 * @param o_ptr アイテム構造体へのポインタ
 * @return アイテムに付与されている鍛冶効果を保持する std::optional オブジェクト返す。
 * 鍛冶効果が付与できないアイテムか、何も付与されていなければ std::nullopt を返す。
 */
std::optional<SmithEffectType> Smith::object_effect(const ItemEntity *o_ptr)
{
    return o_ptr->smith_effect;
}

/*!
 * @brief 指定した鍛冶カテゴリの鍛冶効果のリストを取得する
 *
 * @param category 鍛冶カテゴリ
 * @return 指定した鍛冶カテゴリの鍛冶効果のリスト
 */
std::vector<SmithEffectType> Smith::get_effect_list(SmithCategoryType category)
{
    std::vector<SmithEffectType> result;

    for (const auto &info : smith_info_table) {
        if (info->category == category) {
            result.push_back(info->effect);
        }
    }

    return result;
}

/**
 * @brief 指定した鍛冶効果のエッセンスを付与できる回数を取得する
 *
 * @param effect 鍛冶効果
 * @param o_ptr 鍛冶効果を付与するアイテムへのポインタ。nullptrの場合はデフォルトの消費量での回数が返される。
 * @return エッセンスを付与できる回数を返す
 */
int Smith::get_addable_count(SmithEffectType effect, const ItemEntity *o_ptr) const
{
    auto info = find_smith_info(effect);
    if (!info.has_value()) {
        return 0;
    }

    auto consumption = Smith::get_essence_consumption(effect, o_ptr);

    return addable_count(this->smith_data.get(), info.value()->need_essences, consumption);
}

/*!
 * @brief 鍛冶エッセンスの全リストを取得する
 *
 * @return 鍛冶エッセンスの全リスト
 */
const std::vector<SmithEssenceType> &Smith::get_essence_list()
{
    return essence_list_order;
}

/*!
 * @brief 指定したエッセンスの所持量を取得する
 *
 * @param essence 所持量を取得するエッセンス
 * @return エッセンスの所持量
 */
int Smith::get_essence_num_of_posessions(SmithEssenceType essence) const
{
    return this->smith_data->essences[essence];
}

/*!
 * @brief エッセンスの抽出を行う
 *
 * @param o_ptr エッセンスの抽出を行うアイテムへのポインタ
 * @return 抽出したエッセンスと抽出した量のタプルのリストを返す
 */
Smith::DrainEssenceResult Smith::drain_essence(ItemEntity *o_ptr)
{
    // 抽出量を揃えるためKILLフラグのみ付いている場合はSLAYフラグも付ける
    auto old_flgs = object_flags(o_ptr);
    if (old_flgs.has(TR_KILL_DRAGON)) {
        old_flgs.set(TR_SLAY_DRAGON);
    }
    if (old_flgs.has(TR_KILL_ANIMAL)) {
        old_flgs.set(TR_SLAY_ANIMAL);
    }
    if (old_flgs.has(TR_KILL_EVIL)) {
        old_flgs.set(TR_SLAY_EVIL);
    }
    if (old_flgs.has(TR_KILL_UNDEAD)) {
        old_flgs.set(TR_SLAY_UNDEAD);
    }
    if (old_flgs.has(TR_KILL_DEMON)) {
        old_flgs.set(TR_SLAY_DEMON);
    }
    if (old_flgs.has(TR_KILL_ORC)) {
        old_flgs.set(TR_SLAY_ORC);
    }
    if (old_flgs.has(TR_KILL_TROLL)) {
        old_flgs.set(TR_SLAY_TROLL);
    }
    if (old_flgs.has(TR_KILL_GIANT)) {
        old_flgs.set(TR_SLAY_GIANT);
    }
    if (old_flgs.has(TR_KILL_HUMAN)) {
        old_flgs.set(TR_SLAY_HUMAN);
    }
    if (old_flgs.has(TR_KILL_GOOD)) {
        old_flgs.set(TR_SLAY_GOOD);
    }

    // マイナス効果のあるアイテムから抽出する時のペナルティを計算
    int dec = 4;
    if (o_ptr->curse_flags.has_any_of({ CurseTraitType::CURSED, CurseTraitType::HEAVY_CURSE, CurseTraitType::PERMA_CURSE })) {
        dec--;
    }

    for (auto &&info : essence_drain_info_table) {
        if (info.amount < 0 && old_flgs.has(info.tr_flag)) {
            dec += info.amount;
        }
    }

    const auto is_artifact = o_ptr->is_artifact();

    // アイテムをエッセンス抽出後の状態にする
    const ItemEntity old_o = *o_ptr;
    o_ptr->prep(o_ptr->k_idx);

    o_ptr->iy = old_o.iy;
    o_ptr->ix = old_o.ix;
    o_ptr->marked = old_o.marked;
    o_ptr->number = old_o.number;
    o_ptr->discount = old_o.discount;

    if (o_ptr->tval == ItemKindType::DRAG_ARMOR) {
        o_ptr->timeout = old_o.timeout;
    }
    o_ptr->ident |= (IDENT_FULL_KNOWN);
    object_aware(player_ptr, o_ptr);
    object_known(o_ptr);

    auto new_flgs = object_flags(o_ptr);

    std::unordered_map<SmithEssenceType, int> drain_values;

    // 特性フラグからのエッセンス抽出
    for (auto &&info : essence_drain_info_table) {
        int pval = 0;
        if (TR_PVAL_FLAG_MASK.has(info.tr_flag) && old_o.pval > 0) {
            pval = new_flgs.has(info.tr_flag) ? old_o.pval - o_ptr->pval : old_o.pval;
        }

        if ((new_flgs.has_not(info.tr_flag) || pval) && old_flgs.has(info.tr_flag)) {
            for (auto &&essence : info.essences) {
                auto mult = TR_PVAL_FLAG_MASK.has(info.tr_flag) ? pval : 1;
                drain_values[essence] += info.amount * mult;
            }
        }
    }

    if (is_artifact) {
        drain_values[SmithEssenceType::UNIQUE] += 10;
    }

    // ダイス/命中/ダメージ/ACからの抽出
    auto diff = [](int o, int n) { return std::max(o - n, 0); };

    if (o_ptr->is_weapon_ammo()) {
        drain_values[SmithEssenceType::ATTACK] += diff(old_o.ds, o_ptr->ds) * 10;
        drain_values[SmithEssenceType::ATTACK] += diff(old_o.dd, o_ptr->dd) * 10;
    }

    drain_values[SmithEssenceType::ATTACK] += diff(old_o.to_h, o_ptr->to_h) * 10;
    drain_values[SmithEssenceType::ATTACK] += diff(old_o.to_d, o_ptr->to_d) * 10;
    drain_values[SmithEssenceType::AC] += diff(old_o.ac, o_ptr->ac) * 10;
    drain_values[SmithEssenceType::AC] += diff(old_o.to_a, o_ptr->to_a) * 10;

    // 個数/矢弾/マイナス効果のペナルティによる抽出量の調整
    for (auto &&[unuse, val] : drain_values) {
        val *= o_ptr->number;
        val = val * dec / 4;
        val = std::max(val, 0);
        if (o_ptr->is_ammo()) {
            val /= 10;
        }
    }

    // 所持エッセンスに追加
    std::vector<std::tuple<SmithEssenceType, int>> result;

    for (auto essence : essence_list_order) {
        auto drain_value = drain_values[essence];
        if (drain_value <= 0) {
            continue;
        }

        this->smith_data->essences[essence] = std::min<int16_t>(Smith::ESSENCE_AMOUNT_MAX, this->smith_data->essences[essence] + drain_value);
        result.emplace_back(essence, drain_value);
    }

    return result;
}

/*!
 * @brief 鍛冶効果を付与する
 *
 * @param effect 付与する鍛冶効果
 * @param o_ptr 鍛冶効果を付与するアイテムへのポインタ
 * @param number エッセンス付与数
 * @return 鍛冶効果の付与に成功したら ture、失敗したら false を返す
 */
bool Smith::add_essence(SmithEffectType effect, ItemEntity *o_ptr, int number)
{
    auto info = find_smith_info(effect);
    if (!info.has_value()) {
        return false;
    }

    const auto total_consumption = this->get_essence_consumption(effect, o_ptr) * number;
    for (auto &&essence : info.value()->need_essences) {
        this->smith_data->essences[essence] -= static_cast<int16_t>(total_consumption);
    }

    return info.value()->add_essence(this->player_ptr, o_ptr, number);
}

/*!
 * @brief 鍛冶効果を消去する
 *
 * @param o_ptr 鍛冶効果を消去するアイテムへのポインタ
 */
void Smith::erase_essence(ItemEntity *o_ptr) const
{
    o_ptr->smith_act_idx = std::nullopt;

    auto effect = Smith::object_effect(o_ptr);
    if (!effect.has_value()) {
        return;
    }
    auto info = find_smith_info(effect.value());
    if (!info.has_value()) {
        return;
    }

    info.value()->erase_essence(o_ptr);
}
