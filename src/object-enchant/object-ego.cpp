/*!
 * @brief エゴアイテムに関する処理
 * @date 2019/05/02
 * @author deskull
 */
#include "object-enchant/object-ego.h"
#include "artifact/random-art-effects.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/object-boost.h"
#include "object-enchant/object-curse.h"
#include "object-enchant/trc-types.h"
#include "object-hook/hook-weapon.h"
#include "object/tval-types.h"
#include "sv-definition/sv-armor-types.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/baseitem/baseitem-key.h"
#include "system/item/item-entity.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#include "util/probability-table.h"
#include <algorithm>
#include <cstdint>
#include <limits>
#include <vector>

std::map<EgoType, EgoItemDefinition> egos_info;

/*!
 * @brief アイテムのエゴをレア度の重みに合わせてランダムに選択する
 * Choose random ego type
 * @param slot 取得したいエゴの装備部位
 * @param good TRUEならば通常のエゴ、FALSEならば呪いのエゴが選択対象となる。
 * @return 選択されたエゴ情報のID、万一選択できなかった場合は0が返る。
 */
EgoType get_random_ego(byte slot, bool good)
{
    ProbabilityTable<EgoType> prob_table;
    for (const auto &[e_idx, ego] : egos_info) {
        if (ego.idx == EgoType::NONE || ego.slot != slot || ego.rarity <= 0) {
            continue;
        }

        const auto worthless = is_worthless_ego(ego);
        if (good != worthless) {
            prob_table.entry_item(ego.idx, (255 / ego.rarity));
        }
    }

    if (!prob_table.empty()) {
        return prob_table.pick_one_at_random();
    }

    return EgoType::NONE;
}

/*!
 * @brief エゴオブジェクトに呪いを付加する
 * @param o_ptr オブジェクト情報への参照ポインタ
 * @param gen_flags 生成フラグ(参照渡し)
 */
static void ego_invest_curse(ItemEntity *o_ptr, EnumClassFlagGroup<ItemGenerationTraitType> &gen_flags)
{
    if (gen_flags.has(ItemGenerationTraitType::CURSED)) {
        o_ptr->curse_flags.set(CurseTraitType::CURSED);
    }
    if (gen_flags.has(ItemGenerationTraitType::HEAVY_CURSE)) {
        o_ptr->curse_flags.set(CurseTraitType::HEAVY_CURSE);
    }
    if (gen_flags.has(ItemGenerationTraitType::PERMA_CURSE)) {
        o_ptr->curse_flags.set(CurseTraitType::PERMA_CURSE);
    }
    if (gen_flags.has(ItemGenerationTraitType::RANDOM_CURSE0)) {
        o_ptr->curse_flags.set(get_curse(0, o_ptr));
    }
    if (gen_flags.has(ItemGenerationTraitType::RANDOM_CURSE1)) {
        o_ptr->curse_flags.set(get_curse(1, o_ptr));
    }
    if (gen_flags.has(ItemGenerationTraitType::RANDOM_CURSE2)) {
        o_ptr->curse_flags.set(get_curse(2, o_ptr));
    }
}

/*!
 * @brief エゴオブジェクトに追加能力/耐性を付加する
 * @param o_ptr オブジェクト情報への参照ポインタ
 * @param gen_flags 生成フラグ(参照渡し)
 */
static void ego_invest_extra_abilities(ItemEntity *o_ptr, EnumClassFlagGroup<ItemGenerationTraitType> &gen_flags)
{
    if (gen_flags.has(ItemGenerationTraitType::ONE_SUSTAIN)) {
        one_sustain(o_ptr);
    }
    if (gen_flags.has(ItemGenerationTraitType::XTRA_POWER)) {
        one_ability(o_ptr);
    }
    if (gen_flags.has(ItemGenerationTraitType::XTRA_H_RES)) {
        one_high_resistance(o_ptr);
    }
    if (gen_flags.has(ItemGenerationTraitType::XTRA_E_RES)) {
        one_ele_resistance(o_ptr);
    }
    if (gen_flags.has(ItemGenerationTraitType::XTRA_D_RES)) {
        one_dragon_ele_resistance(o_ptr);
    }
    if (gen_flags.has(ItemGenerationTraitType::XTRA_L_RES)) {
        one_lordly_high_resistance(o_ptr);
    }
    if (gen_flags.has(ItemGenerationTraitType::XTRA_RES)) {
        one_resistance(o_ptr);
    }
    if (gen_flags.has(ItemGenerationTraitType::LIGHT_WEIGHT)) {
        make_weight_ligten(o_ptr);
    }
    if (gen_flags.has(ItemGenerationTraitType::HEAVY_WEIGHT)) {
        make_weight_heavy(o_ptr);
    }
    if (gen_flags.has(ItemGenerationTraitType::XTRA_AC)) {
        add_xtra_ac(o_ptr);
    }
    if (gen_flags.has(ItemGenerationTraitType::HIGH_TELEPATHY)) {
        add_high_telepathy(o_ptr);
    }
    if (gen_flags.has(ItemGenerationTraitType::LOW_TELEPATHY)) {
        add_low_telepathy(o_ptr);
    }
    if (gen_flags.has(ItemGenerationTraitType::XTRA_L_ESP)) {
        one_low_esp(o_ptr);
    }
    auto &dice = o_ptr->damage_dice;
    if (gen_flags.has(ItemGenerationTraitType::ADD_DICE)) {
        dice.num++;
    }
    if (gen_flags.has(ItemGenerationTraitType::DOUBLED_DICE)) {
        dice.num *= 2;
    } else {
        if (gen_flags.has(ItemGenerationTraitType::XTRA_DICE)) {
            do {
                dice.num++;
            } while (one_in_(dice.num));
        }
        if (gen_flags.has(ItemGenerationTraitType::XTRA_DICE_SIDE)) {
            do {
                dice.sides++;
            } while (one_in_(dice.sides));
        }
    }

    if (dice.num > 9) {
        dice.num = 9;
    }
}

/*!
 * @brief エゴアイテムの追加能力/耐性フラグを解釈する
 * @param o_ptr オブジェクト情報への参照ポインタ
 * @param ego エゴアイテム情報への参照
 * @param gen_flags 生成フラグ(参照渡し)
 */
static void ego_interpret_extra_abilities(ItemEntity *o_ptr, const EgoItemDefinition &ego, EnumClassFlagGroup<ItemGenerationTraitType> &gen_flags)
{
    for (const auto &xtra : ego.xtra_flags) {
        if (xtra.chance == 0) {
            continue;
        }

        // 0%と100%は抽選用の乱数を消費しない。
        if (xtra.chance != 100 && !evaluate_percent(xtra.chance)) {
            continue;
        }

        if (!xtra.tr_flags.empty()) {
            const auto f = rand_choice(xtra.tr_flags);
            const auto except = (f == TR_VORPAL) && (o_ptr->bi_key.tval() != ItemKindType::SWORD);
            if (!except) {
                o_ptr->art_flags.set(f);
            }
        }

        for (auto f : xtra.trg_flags) {
            gen_flags.set(f);
        }
    }
}

/*!
 * @brief 追加込みでエゴがフラグを保持しているか判定する
 * @param o_ptr オブジェクト情報への参照ポインタ
 * @param ego エゴアイテム情報への参照
 * @param flag フラグ
 * @return 持つならtrue、持たないならfalse
 */
static bool ego_has_flag(const ItemEntity *o_ptr, const EgoItemDefinition &ego, tr_type flag)
{
    if (o_ptr->art_flags.has(flag)) {
        return true;
    }
    if (ego.flags.has(flag)) {
        return true;
    }
    return false;
}

/*!
 * @brief エゴの追加攻撃を適用したpvalを保存型へ縮小せず返す
 * @param o_ptr オブジェクト情報への参照ポインタ
 * @param ego エゴアイテム情報への参照
 * @param lev 生成階
 * @param pval 適用前のpval
 * @return 追加攻撃の補正後のpval
 */
static int ego_extra_attack_pval(const ItemEntity *o_ptr, const EgoItemDefinition &ego, DEPTH lev, int pval)
{
    if (!o_ptr->is_weapon()) {
        return ego.max_pval >= 0 ? 1 : randint1(ego.max_pval);
    }
    if (o_ptr->ego_idx == EgoType::ATTACKS) {
        // ウィザード操作では荒野の基本生成レベル（最大INT_MAX）も渡る。
        // 乗算を先に広げ、乱数APIのint引数へ戻す前に検証する。
        const auto maximum = static_cast<int>(std::clamp<int64_t>(int64_t{ ego.max_pval } * lev / 100 + 1,
            -std::numeric_limits<int>::max(), std::numeric_limits<int>::max()));
        pval = std::min(randint1(maximum), 3);
        if (o_ptr->bi_key == BaseitemKey(ItemKindType::SWORD, SV_HAYABUSA)) {
            pval += randint1(2);
        }
        return pval;
    }
    if (ego_has_flag(o_ptr, ego, TR_EARTHQUAKE)) {
        return pval + randint1(ego.max_pval);
    }
    if (ego_has_flag(o_ptr, ego, TR_SLAY_EVIL) || ego_has_flag(o_ptr, ego, TR_KILL_EVIL)) {
        pval++;
        if ((lev > 60) && one_in_(3) && (o_ptr->damage_dice.floored_expected_value_multiplied_by(2) < 15)) {
            pval++;
        }
        return pval;
    }
    return pval + randint1(2);
}

/*!
 * @brief オブジェクトをエゴアイテムにする
 * @param o_ptr オブジェクト情報への参照ポインタ
 * @param lev 生成階
 */
void apply_ego(ItemEntity *o_ptr, DEPTH lev)
{
    const auto &ego = o_ptr->get_ego();
    auto gen_flags = ego.gen_flags;

    ego_interpret_extra_abilities(o_ptr, ego, gen_flags);

    if (!ego.cost) {
        o_ptr->set_identification_flag(IdentificationFlag::BROKEN);
    }

    ego_invest_curse(o_ptr, gen_flags);
    ego_invest_extra_abilities(o_ptr, gen_flags);

    if (ego.act_idx > RandomArtActType::NONE) {
        o_ptr->activation_id = ego.act_idx;
    }

    // 合成途中は広い型で計算し、最後に保存形式の範囲へ飽和する。
    auto to_h = int64_t{ o_ptr->to_h };
    auto to_d = int64_t{ o_ptr->to_d };
    auto to_a = int64_t{ o_ptr->to_a };
    auto pval = int{ o_ptr->pval };
    to_h += ego.base_to_h;
    to_d += ego.base_to_d;
    to_a += ego.base_to_a;

    auto is_powerful = ego.gen_flags.has(ItemGenerationTraitType::POWERFUL);
    auto is_cursed = (o_ptr->is_cursed() || o_ptr->is_broken()) && !is_powerful;
    if (is_cursed) {
        if (ego.max_to_h) {
            to_h -= randint1(ego.max_to_h);
        }
        if (ego.max_to_d) {
            to_d -= randint1(ego.max_to_d);
        }
        if (ego.max_to_a) {
            to_a -= randint1(ego.max_to_a);
        }
        if (ego.max_pval) {
            pval -= randint1(ego.max_pval);
        }
    } else {
        if (is_powerful) {
            if (ego.max_to_h > 0 && to_h < 0) {
                to_h = 0 - to_h;
            }
            if (ego.max_to_d > 0 && to_d < 0) {
                to_d = 0 - to_d;
            }
            if (ego.max_to_a > 0 && to_a < 0) {
                to_a = 0 - to_a;
            }
        }

        to_h += ego.max_to_h == 0 ? 0 : randint1(ego.max_to_h);
        to_d += ego.max_to_d == 0 ? 0 : randint1(ego.max_to_d);
        to_a += ego.max_to_a == 0 ? 0 : randint1(ego.max_to_a);

        if (gen_flags.has(ItemGenerationTraitType::MOD_ACCURACY)) {
            if (to_h < to_d + 10) {
                const auto steps = (to_d + 10 - to_h + 9) / 10;
                to_h += 5 * steps;
                to_d -= 5 * steps;
            }
            to_h = std::max<int64_t>(to_h, 15);
        }

        if (gen_flags.has(ItemGenerationTraitType::MOD_VELOCITY)) {
            if (to_d < to_h + 10) {
                const auto steps = (to_h + 10 - to_d + 9) / 10;
                to_d += 5 * steps;
                to_h -= 5 * steps;
            }
            to_d = std::max<int64_t>(to_d, 15);
        }

        if ((o_ptr->ego_idx == EgoType::PROTECTION) || (o_ptr->ego_idx == EgoType::S_PROTECTION) || (o_ptr->ego_idx == EgoType::H_PROTECTION)) {
            to_a = std::max<int64_t>(to_a, 15);
        }

        if (ego.max_pval) {
            if (o_ptr->ego_idx == EgoType::BAT) {
                pval = randint1(ego.max_pval);
                if (o_ptr->bi_key.sval() == SV_ELVEN_CLOAK) {
                    pval += randint1(2);
                }
            } else {
                if (ego_has_flag(o_ptr, ego, TR_BLOWS)) {
                    pval = ego_extra_attack_pval(o_ptr, ego, lev, pval);
                } else {
                    if (ego.max_pval > 0) {
                        pval += randint1(ego.max_pval);
                    } else if (ego.max_pval < 0) {
                        pval -= randint1(0 - ego.max_pval);
                    }
                }
            }
        }

        if ((o_ptr->ego_idx == EgoType::SPEED) && (lev < 50)) {
            pval = randint1(pval);
        }

        if ((o_ptr->bi_key == BaseitemKey(ItemKindType::SWORD, SV_HAYABUSA)) && (pval > 2) && (o_ptr->ego_idx != EgoType::ATTACKS)) {
            pval = 2;
        }
    }
    constexpr auto low = std::numeric_limits<int16_t>::min();
    constexpr auto high = std::numeric_limits<int16_t>::max();
    o_ptr->to_h = static_cast<HIT_PROB>(std::clamp<int64_t>(to_h, low, high));
    o_ptr->to_d = static_cast<int>(std::clamp<int64_t>(to_d, low, high));
    o_ptr->to_a = static_cast<ARMOUR_CLASS>(std::clamp<int64_t>(to_a, low, high));
    o_ptr->pval = static_cast<PARAMETER_VALUE>(std::clamp(pval, int{ low }, int{ high }));
}

/*!
 * @brief 評価値または生成時の呪いから悪いエゴか判定する。
 * @param ego 判定するエゴの定義
 * @return 評価値が0か、生成時に呪われるエゴなら true
 */
bool is_worthless_ego(const EgoItemDefinition &ego)
{
    const auto curses = { ItemGenerationTraitType::CURSED, ItemGenerationTraitType::HEAVY_CURSE, ItemGenerationTraitType::PERMA_CURSE };
    return ego.rating == 0 || ego.gen_flags.has_any_of(curses);
}

/*!
 * @brief 兜に付くよいエゴか判定する。
 * @param ego 判定するエゴ
 * @return 兜に付くよいエゴなら true
 * @details HelmEnchanter::select_ego() が抽選したエゴを選ぶときと、can_apply_ego() で使う。
 */
bool is_helm_ego(EgoType ego)
{
    switch (ego) {
    case EgoType::BRILLIANCE:
    case EgoType::DARK:
    case EgoType::INFRAVISION:
    case EgoType::H_PROTECTION:
    case EgoType::LITE:
    case EgoType::SEEING:
        return true;
    default:
        return false;
    }
}

/*!
 * @brief 冠に付くよいエゴか判定する。
 * @param ego 判定するエゴ
 * @return 冠に付くよいエゴなら true
 * @details CrownEnchanter::select_ego() が抽選したエゴを選ぶときと、can_apply_ego() で使う。
 */
bool is_crown_ego(EgoType ego)
{
    switch (ego) {
    case EgoType::TELEPATHY:
    case EgoType::MAGI:
    case EgoType::MIGHT:
    case EgoType::REGENERATION:
    case EgoType::LORDLINESS:
    case EgoType::BASILISK:
    case EgoType::SEEING:
        return true;
    default:
        return false;
    }
}

/*!
 * @brief 部位と種類別の制限から、指定したよいエゴを付けられるか判定する。
 * @param key エゴを付けるベースアイテムの種類
 * @param ego 付けたいエゴ
 * @return エゴが定義済みのよいエゴで、部位が合い、種類別の除外にも当たらなければ true
 * @details 兜と冠は、それぞれの enchanter が選ぶエゴだけを通す。
 * 穴掘りのエゴは利き手の部位だが、希少度 0 で抽選されず掘削道具にだけ付くので、剣・鈍器・長柄には通さない。
 * ドラゴンの兜・盾・靴・籠手は、自然生成では 3 回に 1 回エゴが付くが、エゴを要求しても enchanter が
 * エゴを付けずに戻ることがあるので拒む。
 */
bool can_apply_ego(const BaseitemKey &key, EgoType ego)
{
    if (ego == EgoType::NONE) {
        return false;
    }
    const auto it = egos_info.find(ego);
    if ((it == egos_info.end()) || is_worthless_ego(it->second)) {
        return false;
    }
    const auto &ego_definition = it->second;
    const auto tval = key.tval();
    const auto sval = key.sval();
    INVENTORY_IDX slot{};
    if (ego == EgoType::DIGGING && tval != ItemKindType::DIGGING) {
        return false;
    }
    switch (tval) {
    case ItemKindType::SWORD:
        if (sval == SV_POISON_NEEDLE || sval == SV_DIAMOND_EDGE || ego == EgoType::EARTHQUAKES) {
            return false;
        }
        slot = INVEN_MAIN_HAND;
        break;
    case ItemKindType::HAFTED:
        if (ego == EgoType::SHARPNESS) {
            return false;
        }
        slot = INVEN_MAIN_HAND;
        break;
    case ItemKindType::POLEARM:
        if (sval == SV_DEATH_SCYTHE || ego == EgoType::SHARPNESS || ego == EgoType::EARTHQUAKES) {
            return false;
        }
        slot = INVEN_MAIN_HAND;
        break;
    case ItemKindType::DIGGING:
        if (ego != EgoType::DIGGING) {
            return false;
        }
        slot = INVEN_MAIN_HAND;
        break;
    case ItemKindType::BOW:
        slot = INVEN_BOW;
        break;
    case ItemKindType::SHOT:
    case ItemKindType::ARROW:
    case ItemKindType::BOLT:
        slot = INVEN_AMMO;
        break;
    case ItemKindType::SOFT_ARMOR:
    case ItemKindType::HARD_ARMOR:
        if ((ego == EgoType::DWARVEN && tval != ItemKindType::HARD_ARMOR) || (ego == EgoType::DRUID && tval != ItemKindType::SOFT_ARMOR) || ((ego == EgoType::PERMANENCE || ego == EgoType::TWILIGHT) && (tval != ItemKindType::SOFT_ARMOR || sval != SV_ROBE))) {
            return false;
        }
        slot = INVEN_BODY;
        break;
    case ItemKindType::SHIELD:
        if (sval == SV_DRAGON_SHIELD || (ego == EgoType::S_DWARVEN && sval != SV_SMALL_METAL_SHIELD && sval != SV_LARGE_METAL_SHIELD) || (ego == EgoType::REFLECTION && sval == SV_MIRROR_SHIELD)) {
            return false;
        }
        slot = INVEN_SUB_HAND;
        break;
    case ItemKindType::HELM:
        if (sval == SV_DRAGON_HELM || !is_helm_ego(ego)) {
            return false;
        }
        slot = INVEN_HEAD;
        break;
    case ItemKindType::CROWN:
        if (!is_crown_ego(ego)) {
            return false;
        }
        slot = INVEN_HEAD;
        break;
    case ItemKindType::BOOTS:
        if (sval == SV_PAIR_OF_DRAGON_GREAVE) {
            return false;
        }
        slot = INVEN_FEET;
        break;
    case ItemKindType::GLOVES:
        if (sval == SV_SET_OF_DRAGON_GLOVES) {
            return false;
        }
        slot = INVEN_ARMS;
        break;
    case ItemKindType::CLOAK:
        slot = INVEN_OUTER;
        break;
    case ItemKindType::LITE:
        if (sval == SV_LITE_FEANOR && ego == EgoType::LITE_LONG) {
            return false;
        }
        slot = INVEN_LITE;
        break;
    default:
        return false;
    }
    return ego_definition.slot == slot;
}
