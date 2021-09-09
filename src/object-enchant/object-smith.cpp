#include "object-enchant/object-smith.h"
#include "artifact/random-art-effects.h"
#include "object-enchant/smith-tables.h"
#include "object-enchant/smith-types.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-flags.h"
#include "object-enchant/tr-types.h"
#include "object/item-tester-hooker.h"
#include "object/object-flags.h"
#include "perception/object-perception.h"
#include "system/object-type-definition.h"
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
int addable_count(const player_type *player_ptr, std::vector<SmithEssence> essence_list, int consumption)
{
    if (consumption <= 0) {
        return 0;
    }

    std::vector<int> addable_count;
    for (auto essence : essence_list) {
        int own_amount = player_ptr->magic_num1[enum2i(essence)];
        addable_count.push_back(own_amount / consumption);
    }
    return *std::min_element(addable_count.begin(), addable_count.end());
}

}

/*!
 * @brief 鍛冶クラスコンストラクタ
 */
Smith::Smith(player_type *player_ptr)
    : player_ptr(player_ptr)
{
}

/*!
 * @brief 引数で指定した鍛冶効果の鍛冶情報を得る
 *
 * @param effect 情報を得る鍛冶効果
 * @return 鍛冶情報構造体へのポインタを保持する std::optional オブジェクトを返す
 */
std::optional<const smith_info_type *> Smith::find_smith_info(SmithEffect effect)
{
    // 何度も呼ぶので線形探索を避けるため鍛冶効果から鍛冶情報のテーブルを引けるmapを作成しておく。
    static std::unordered_map<SmithEffect, const smith_info_type *> search_map;
    if (search_map.empty()) {
        for (const auto &info : smith_info_table) {
            search_map.emplace(info.effect, &info);
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
concptr Smith::get_essence_name(SmithEssence essence)
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
concptr Smith::get_effect_name(SmithEffect effect)
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
std::string Smith::get_need_essences_desc(SmithEffect effect)
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
std::vector<SmithEssence> Smith::get_need_essences(SmithEffect effect)
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
int Smith::get_essence_consumption(SmithEffect effect, const object_type *o_ptr)
{
    auto info = find_smith_info(effect);
    if (!info.has_value()) {
        return 0;
    }

    auto consumption = info.value()->consumption;
    if (o_ptr == nullptr) {
        return consumption;
    }

    if ((o_ptr->tval >= TV_SHOT) && (o_ptr->tval <= TV_BOLT)) {
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
std::unique_ptr<ItemTester> Smith::get_item_tester(SmithEffect effect)
{
    auto category = SmithCategory::NONE;
    if (auto info = find_smith_info(effect); info.has_value()) {
        category = info.value()->category;
    }

    if (effect == SmithEffect::SLAY_GLOVE) {
        return std::make_unique<TvalItemTester>(TV_GLOVES);
    }
    if (category == SmithCategory::WEAPON_ATTR || category == SmithCategory::SLAYING) {
        return std::make_unique<FuncItemTester>(&object_type::is_melee_ammo);
    }
    if (effect == SmithEffect::ATTACK) {
        return std::make_unique<FuncItemTester>(&object_type::allow_enchant_weapon);
    }
    if (effect == SmithEffect::AC) {
        return std::make_unique<FuncItemTester>(&object_type::is_armour);
    }

    return std::make_unique<FuncItemTester>(&object_type::is_weapon_armour_ammo);
}

/*!
 * @brief 鍛冶効果により得られる特性フラグを取得する
 *
 * @param effect 鍛冶効果
 * @return 鍛冶効果により得られる特性フラグがONになったTrFlagsオブジェクト
 */
TrFlags Smith::get_effect_tr_flags(SmithEffect effect)
{
    auto info = find_smith_info(effect);
    if (!info.has_value()) {
        return {};
    }

    return info.value()->add_flags;
}

/*!
 * @brief 鍛冶効果により得られる発動IDを得る
 *
 * @param effect 鍛冶効果
 * @return 鍛冶効果により得られる発動ID(random_art_activation_type型)
 * 鍛冶効果により得られる発動効果が無い場合は std::nullopt
 */
std::optional<random_art_activation_type> Smith::get_effect_activation(SmithEffect effect)
{
    switch (effect) {
    case SmithEffect::TMP_RES_ACID:
        return ACT_RESIST_ACID;
    case SmithEffect::TMP_RES_ELEC:
        return ACT_RESIST_ELEC;
    case SmithEffect::TMP_RES_FIRE:
        return ACT_RESIST_FIRE;
    case SmithEffect::TMP_RES_COLD:
        return ACT_RESIST_COLD;
    case SmithEffect::EARTHQUAKE:
        return ACT_QUAKE;
    default:
        return std::nullopt;
    }
}

/*!
 * @brief アイテムに付与されている鍛冶効果を取得する
 *
 * @param o_ptr アイテム構造体へのポインタ
 * @return アイテムに付与されている鍛冶効果を保持する std::optional オブジェクト返す。
 * 鍛冶効果が付与できないアイテムか、何も付与されていなければ std::nullopt を返す。
 */
std::optional<SmithEffect> Smith::object_effect(const object_type *o_ptr)
{
    auto effect = static_cast<SmithEffect>(o_ptr->xtra3);
    if (!o_ptr->is_weapon_armour_ammo() || effect == SmithEffect::NONE) {
        return std::nullopt;
    }

    return effect;
}

/*!
 * @brief 指定した鍛冶カテゴリの鍛冶効果のリストを取得する
 *
 * @param category 鍛冶カテゴリ
 * @return 指定した鍛冶カテゴリの鍛冶効果のリスト
 */
std::vector<SmithEffect> Smith::get_effect_list(SmithCategory category)
{
    std::vector<SmithEffect> result;

    for (const auto &info : smith_info_table) {
        if (info.category == category) {
            result.push_back(info.effect);
        }
    }

    return result;
}

/**
 * @brief 指定した鍛冶効果のエッセンスを付与できる回数を取得する
 *
 * @param effect 鍛冶効果
 * @param item_number 同時に付与するスタックしたアイテム数。スタックしている場合アイテム数倍の数だけエッセンスが必要となる。
 * @return エッセンスを付与できる回数を返す
 */
int Smith::get_addable_count(SmithEffect effect, int item_number) const
{
    auto info = find_smith_info(effect);
    if (!info.has_value()) {
        return 0;
    }

    return addable_count(this->player_ptr, info.value()->need_essences, info.value()->consumption * item_number);
}

/*!
 * @brief 鍛冶エッセンスの全リストを取得する
 *
 * @return 鍛冶エッセンスの全リスト
 */
const std::vector<SmithEssence> &Smith::get_essence_list()
{
    return essence_list_order;
}

/*!
 * @brief 指定したエッセンスの所持量を取得する
 *
 * @param essence 所持量を取得するエッセンス
 * @return エッセンスの所持量
 */
int Smith::get_essence_num_of_posessions(SmithEssence essence) const
{
    return this->player_ptr->magic_num1[enum2i(essence)];
}

/*!
 * @brief エッセンスの抽出を行う
 *
 * @param o_ptr エッセンスの抽出を行うアイテムへのポインタ
 * @return 抽出したエッセンスと抽出した量のタプルのリストを返す
 */
Smith::DrainEssenceResult Smith::drain_essence(object_type *o_ptr)
{
    // 抽出量を揃えるためKILLフラグのみ付いている場合はSLAYフラグも付ける
    auto old_flgs = object_flags(o_ptr);
    if (old_flgs.has(TR_KILL_DRAGON))
        old_flgs.set(TR_SLAY_DRAGON);
    if (old_flgs.has(TR_KILL_ANIMAL))
        old_flgs.set(TR_SLAY_ANIMAL);
    if (old_flgs.has(TR_KILL_EVIL))
        old_flgs.set(TR_SLAY_EVIL);
    if (old_flgs.has(TR_KILL_UNDEAD))
        old_flgs.set(TR_SLAY_UNDEAD);
    if (old_flgs.has(TR_KILL_DEMON))
        old_flgs.set(TR_SLAY_DEMON);
    if (old_flgs.has(TR_KILL_ORC))
        old_flgs.set(TR_SLAY_ORC);
    if (old_flgs.has(TR_KILL_TROLL))
        old_flgs.set(TR_SLAY_TROLL);
    if (old_flgs.has(TR_KILL_GIANT))
        old_flgs.set(TR_SLAY_GIANT);
    if (old_flgs.has(TR_KILL_HUMAN))
        old_flgs.set(TR_SLAY_HUMAN);
    if (old_flgs.has(TR_KILL_GOOD))
        old_flgs.set(TR_SLAY_GOOD);

    // マイナス効果のあるアイテムから抽出する時のペナルティを計算
    int dec = 4;
    if (o_ptr->curse_flags.has_any_of({ TRC::CURSED, TRC::HEAVY_CURSE, TRC::PERMA_CURSE }))
        dec--;

    for (auto &&info : essence_drain_info_table) {
        if (info.amount < 0 && old_flgs.has(info.tr_flag)) {
            dec += info.amount;
        }
    }

    // アイテムをエッセンス抽出後の状態にする
    const object_type old_o = *o_ptr;
    o_ptr->prep(o_ptr->k_idx);

    o_ptr->iy = old_o.iy;
    o_ptr->ix = old_o.ix;
    o_ptr->marked = old_o.marked;
    o_ptr->number = old_o.number;

    if (o_ptr->tval == TV_DRAG_ARMOR)
        o_ptr->timeout = old_o.timeout;
    o_ptr->ident |= (IDENT_FULL_KNOWN);
    object_aware(player_ptr, o_ptr);
    object_known(o_ptr);

    auto new_flgs = object_flags(o_ptr);

    std::unordered_map<SmithEssence, int> drain_values;

    // 特性フラグからのエッセンス抽出
    for (auto &&info : essence_drain_info_table) {
        int pval = 0;
        if (TR_PVAL_FLAG_MASK.has(info.tr_flag) && old_o.pval > 0) {
            pval = new_flgs.has(info.tr_flag) ? old_o.pval - o_ptr->pval : old_o.pval;
        }

        if ((new_flgs.has_not(info.tr_flag) || pval) && old_flgs.has(info.tr_flag)) {
            for (auto &&essence : info.essences) {
                drain_values[essence] += info.amount * std::max(pval, 1);
            }
        }
    }

    // ダイス/命中/ダメージ/ACからの抽出
    auto diff = [](int o, int n) { return std::max(o - n, 0); };

    if (o_ptr->is_weapon_ammo()) {
        drain_values[SmithEssence::ATTACK] += diff(old_o.ds, o_ptr->ds) * 10;
        drain_values[SmithEssence::ATTACK] += diff(old_o.dd, o_ptr->dd) * 10;
    }

    drain_values[SmithEssence::ATTACK] += diff(old_o.to_h, o_ptr->to_h) * 10;
    drain_values[SmithEssence::ATTACK] += diff(old_o.to_d, o_ptr->to_d) * 10;
    drain_values[SmithEssence::AC] += diff(old_o.ac, o_ptr->ac) * 10;
    drain_values[SmithEssence::AC] += diff(old_o.to_a, o_ptr->to_a) * 10;

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
    std::vector<std::tuple<SmithEssence, int>> result;

    for (auto essence : essence_list_order) {
        auto drain_value = drain_values[essence];
        if (drain_value <= 0) {
            continue;
        }

        auto i = enum2i(essence);
        this->player_ptr->magic_num1[i] = std::min(20000, this->player_ptr->magic_num1[i] + drain_value);
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
bool Smith::add_essence(SmithEffect effect, object_type *o_ptr, int number)
{
    auto info = find_smith_info(effect);
    if (!info.has_value()) {
        return false;
    }

    const auto total_consumption = this->get_essence_consumption(effect, o_ptr) * number;
    for (auto &&essence : info.value()->need_essences) {
        this->player_ptr->magic_num1[enum2i(essence)] -= total_consumption;
    }

    if (effect == SmithEffect::SLAY_GLOVE) {
        HIT_PROB get_to_h = ((number + 1) / 2 + randint0(number / 2 + 1));
        HIT_POINT get_to_d = ((number + 1) / 2 + randint0(number / 2 + 1));
        o_ptr->xtra4 = (get_to_h << 8) + get_to_d;
        o_ptr->to_h += get_to_h;
        o_ptr->to_d += get_to_d;
    }

    if (effect == SmithEffect::ATTACK) {
        const auto max_val = this->player_ptr->lev / 5 + 5;
        if ((o_ptr->to_h >= max_val) && (o_ptr->to_d >= max_val)) {
            return false;
        } else {
            o_ptr->to_h = static_cast<HIT_PROB>(std::min(o_ptr->to_h + 1, max_val));
            o_ptr->to_d = static_cast<HIT_POINT>(std::min(o_ptr->to_d + 1, max_val));
        }
    } else if (effect == SmithEffect::AC) {
        const auto max_val = this->player_ptr->lev / 5 + 5;
        if (o_ptr->to_a >= max_val) {
            return false;
        } else {
            o_ptr->to_a = static_cast<ARMOUR_CLASS>(std::min(o_ptr->to_a + 1, max_val));
        }
    } else if (effect == SmithEffect::SUSTAIN) {
        o_ptr->art_flags.set(TR_IGNORE_ACID);
        o_ptr->art_flags.set(TR_IGNORE_ELEC);
        o_ptr->art_flags.set(TR_IGNORE_FIRE);
        o_ptr->art_flags.set(TR_IGNORE_COLD);
    } else {
        o_ptr->xtra3 = static_cast<decltype(o_ptr->xtra3)>(effect);
    }

    return true;
}

/*!
 * @brief 鍛冶効果を消去する
 *
 * @param o_ptr 鍛冶効果を消去するアイテムへのポインタ
 */
void Smith::erase_essence(object_type *o_ptr) const
{
    auto effect = Smith::object_effect(o_ptr);
    if (effect == SmithEffect::SLAY_GLOVE) {
        o_ptr->to_h -= (o_ptr->xtra4 >> 8);
        o_ptr->to_d -= (o_ptr->xtra4 & 0x000f);
        o_ptr->xtra4 = 0;
        if (o_ptr->to_h < 0)
            o_ptr->to_h = 0;
        if (o_ptr->to_d < 0)
            o_ptr->to_d = 0;
    }
    o_ptr->xtra3 = 0;
    auto flgs = object_flags(o_ptr);
    if (flgs.has_none_of(TR_PVAL_FLAG_MASK))
        o_ptr->pval = 0;
}
