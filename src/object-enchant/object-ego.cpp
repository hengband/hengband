/*!
 * @brief エゴアイテムに関する処理
 * @date 2019/05/02
 * @author deskull
 */
#include "object-enchant/object-ego.h"
#include "artifact/random-art-effects.h"
#include "object-enchant/object-boost.h"
#include "object-enchant/object-curse.h"
#include "object-enchant/trc-types.h"
#include "object-hook/hook-weapon.h"
#include "object/tval-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-weapon-types.h"
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

        const auto curses = {
            ItemGenerationTraitType::CURSED,
            ItemGenerationTraitType::HEAVY_CURSE,
            ItemGenerationTraitType::PERMA_CURSE
        };
        const auto worthless = ego.rating == 0 || ego.gen_flags.has_any_of(curses);
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
