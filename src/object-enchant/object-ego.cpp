/*!
 * @brief エゴアイテムに関する処理
 * @date 2019/05/02
 * @author deskull
 * @details Ego-Item indexes (see "lib/edit/e_info.txt")
 */
#include <vector>

#include "object-enchant/object-ego.h"
#include "object-enchant/object-boost.h"
#include "object-enchant/object-curse.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/trc-types.h"
#include "object-hook/hook-weapon.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/object-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/probability-table.h"

/*
 * The ego-item arrays
 */
std::vector<ego_item_type> e_info;

/*
 * Maximum number of ego-items in e_info.txt
 */
EGO_IDX max_e_idx;

/*!
 * @brief アイテムのエゴをレア度の重みに合わせてランダムに選択する
 * Choose random ego type
 * @param slot 取得したいエゴの装備部位
 * @param good TRUEならば通常のエゴ、FALSEならば呪いのエゴが選択対象となる。
 * @return 選択されたエゴ情報のID、万一選択できなかった場合は0が返る。
 */
byte get_random_ego(byte slot, bool good)
{
    ProbabilityTable<EGO_IDX> prob_table;
    for (const auto &e_ref : e_info) {
        if (e_ref.idx == 0 || e_ref.slot != slot || e_ref.rarity <= 0)
            continue;

        bool worthless = e_ref.rating == 0 || e_ref.gen_flags.has_any_of({ TRG::CURSED, TRG::HEAVY_CURSE, TRG::PERMA_CURSE });

        if (good != worthless) {
            prob_table.entry_item(e_ref.idx, (255 / e_ref.rarity));
        }
    }

    if (!prob_table.empty()) {
        return prob_table.pick_one_at_random();
    }

    return (EGO_IDX)0;
}

/*!
 * @brief エゴオブジェクトに呪いを付加する
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param o_ptr オブジェクト情報への参照ポインタ
 * @param gen_flags 生成フラグ(参照渡し)
 */
static void ego_invest_curse(object_type *o_ptr, EnumClassFlagGroup<TRG> &gen_flags)
{
    if (gen_flags.has(TRG::CURSED))
        o_ptr->curse_flags.set(TRC::CURSED);
    if (gen_flags.has(TRG::HEAVY_CURSE))
        o_ptr->curse_flags.set(TRC::HEAVY_CURSE);
    if (gen_flags.has(TRG::PERMA_CURSE))
        o_ptr->curse_flags.set(TRC::PERMA_CURSE);
    if (gen_flags.has(TRG::RANDOM_CURSE0))
        o_ptr->curse_flags.set(get_curse(0, o_ptr));
    if (gen_flags.has(TRG::RANDOM_CURSE1))
        o_ptr->curse_flags.set(get_curse(1, o_ptr));
    if (gen_flags.has(TRG::RANDOM_CURSE2))
        o_ptr->curse_flags.set(get_curse(2, o_ptr));
}

/*!
 * @brief エゴオブジェクトに追加能力/耐性を付加する
 * @param o_ptr オブジェクト情報への参照ポインタ
 * @param gen_flags 生成フラグ(参照渡し)
 */
static void ego_invest_extra_abilities(object_type *o_ptr, EnumClassFlagGroup<TRG> &gen_flags)
{
    if (gen_flags.has(TRG::ONE_SUSTAIN))
        one_sustain(o_ptr);
    if (gen_flags.has(TRG::XTRA_POWER))
        one_ability(o_ptr);
    if (gen_flags.has(TRG::XTRA_H_RES))
        one_high_resistance(o_ptr);
    if (gen_flags.has(TRG::XTRA_E_RES))
        one_ele_resistance(o_ptr);
    if (gen_flags.has(TRG::XTRA_D_RES))
        one_dragon_ele_resistance(o_ptr);
    if (gen_flags.has(TRG::XTRA_L_RES))
        one_lordly_high_resistance(o_ptr);
    if (gen_flags.has(TRG::XTRA_RES))
        one_resistance(o_ptr);
    if (gen_flags.has(TRG::LIGHT_WEIGHT))
        make_weight_ligten(o_ptr);
    if (gen_flags.has(TRG::HEAVY_WEIGHT))
        make_weight_heavy(o_ptr);
    if (gen_flags.has(TRG::XTRA_AC))
        add_xtra_ac(o_ptr);
    if (gen_flags.has(TRG::HIGH_TELEPATHY))
        add_high_telepathy(o_ptr);
    if (gen_flags.has(TRG::LOW_TELEPATHY))
        add_low_telepathy(o_ptr);
    if (gen_flags.has(TRG::XTRA_L_ESP))
        one_low_esp(o_ptr);
    if (gen_flags.has(TRG::ADD_DICE))
        o_ptr->dd++;
    if (gen_flags.has(TRG::DOUBLED_DICE))
        o_ptr->dd *= 2;
    else {
        if (gen_flags.has(TRG::XTRA_DICE)) {
            do {
                o_ptr->dd++;
            } while (one_in_(o_ptr->dd));
        }
        if (gen_flags.has(TRG::XTRA_DICE_SIDE)) {
            do {
                o_ptr->ds++;
            } while (one_in_(o_ptr->ds));
        }
    }

    if (o_ptr->dd > 9)
        o_ptr->dd = 9;
}

/*!
 * @brief エゴアイテムの追加能力/耐性フラグを解釈する
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param o_ptr オブジェクト情報への参照ポインタ
 * @param e_ptr エゴアイテム情報への参照ポインタ
 * @param gen_flags 生成フラグ(参照渡し)
 */
static void ego_interpret_extra_abilities(object_type *o_ptr, ego_item_type *e_ptr, EnumClassFlagGroup<TRG> &gen_flags)
{
    for (auto& xtra : e_ptr->xtra_flags) {
        if (xtra.mul == 0 || xtra.dev == 0)
            continue;

        if (randint0(xtra.dev) >= xtra.mul) //! @note mul/devで適用
            continue;

        auto n = xtra.tr_flags.size();
        if (n > 0) {
            auto f = xtra.tr_flags[randint0(n)];
            auto except = (f == TR_VORPAL && o_ptr->tval != TV_SWORD);
            if (!except)
                o_ptr->art_flags.set(f);
        }

        for (auto f : xtra.trg_flags)
            gen_flags.set(f);
    }
}

/*!
 * @brief 0 および負数に対応した randint1()
 * @param n
 *
 * n == 0 のとき、常に 0 を返す。
 * n >  0 のとき、[1, n] の乱数を返す。
 * n <  0 のとき、[n,-1] の乱数を返す。
 */
static int randint1_signed(const int n)
{
    if (n == 0)
        return 0;

    return n > 0 ? randint1(n) : -randint1(-n);
}

/*!
 * @brief 追加込みでエゴがフラグを保持しているか判定する
 * @param o_ptr オブジェクト情報への参照ポインタ
 * @param e_ptr エゴアイテム情報への参照ポインタ
 * @param flag フラグ
 * @return 持つならtrue、持たないならfalse
 */
static bool ego_has_flag(object_type *o_ptr, ego_item_type *e_ptr, tr_type flag)
{
    if (o_ptr->art_flags.has(flag))
        return true;
    if (e_ptr->flags.has(flag))
        return true;
    return false;
}

/*!
 * @brief エゴに追加攻撃のpvalを付加する
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param o_ptr オブジェクト情報への参照ポインタ
 * @param e_ptr エゴアイテム情報への参照ポインタ
 * @param lev 生成階
 */
void ego_invest_extra_attack(object_type *o_ptr, ego_item_type *e_ptr, DEPTH lev)
{
    if (!o_ptr->is_weapon()) {
        o_ptr->pval = e_ptr->max_pval >= 0 ? 1 : randint1_signed(e_ptr->max_pval);
        return;
    }

    if (o_ptr->name2 == EGO_ATTACKS) {
        o_ptr->pval = randint1(e_ptr->max_pval * lev / 100 + 1);
        if (o_ptr->pval > 3)
            o_ptr->pval = 3;
        if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_HAYABUSA))
            o_ptr->pval += randint1(2);
        return;
    }

    if (ego_has_flag(o_ptr, e_ptr, TR_EARTHQUAKE)) {
        o_ptr->pval += randint1(e_ptr->max_pval);
        return;
    }

    if (ego_has_flag(o_ptr, e_ptr, TR_SLAY_EVIL) || ego_has_flag(o_ptr, e_ptr, TR_KILL_EVIL)) {
        o_ptr->pval++;
        if ((lev > 60) && one_in_(3) && ((o_ptr->dd * (o_ptr->ds + 1)) < 15))
            o_ptr->pval++;
        return;
    }

    o_ptr->pval += randint1(2);
}

/*!
 * @brief オブジェクトをエゴアイテムにする
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param o_ptr オブジェクト情報への参照ポインタ
 * @param lev 生成階
 */
void apply_ego(object_type *o_ptr, DEPTH lev)
{
    auto e_ptr = &e_info[o_ptr->name2];
    auto gen_flags = e_ptr->gen_flags;

    ego_interpret_extra_abilities(o_ptr, e_ptr, gen_flags);

    if (!e_ptr->cost)
        o_ptr->ident |= (IDENT_BROKEN);

    ego_invest_curse(o_ptr, gen_flags);
    ego_invest_extra_abilities(o_ptr, gen_flags);

    if (e_ptr->act_idx)
        o_ptr->xtra2 = (XTRA8)e_ptr->act_idx;

    o_ptr->to_h += (HIT_PROB)e_ptr->base_to_h;
    o_ptr->to_d += (HIT_POINT)e_ptr->base_to_d;
    o_ptr->to_a += (ARMOUR_CLASS)e_ptr->base_to_a;

    auto is_powerful = e_ptr->gen_flags.has(TRG::POWERFUL);
    auto is_cursed = (o_ptr->is_cursed() || o_ptr->is_broken()) && !is_powerful;
    if (is_cursed) {
        if (e_ptr->max_to_h)
            o_ptr->to_h -= randint1(e_ptr->max_to_h);
        if (e_ptr->max_to_d)
            o_ptr->to_d -= randint1(e_ptr->max_to_d);
        if (e_ptr->max_to_a)
            o_ptr->to_a -= randint1(e_ptr->max_to_a);
        if (e_ptr->max_pval)
            o_ptr->pval -= randint1(e_ptr->max_pval);
    } else {
        if (is_powerful) {
            if (e_ptr->max_to_h > 0 && o_ptr->to_h < 0)
                o_ptr->to_h = 0 - o_ptr->to_h;
            if (e_ptr->max_to_d > 0 && o_ptr->to_d < 0)
                o_ptr->to_d = 0 - o_ptr->to_d;
            if (e_ptr->max_to_a > 0 && o_ptr->to_a < 0)
                o_ptr->to_a = 0 - o_ptr->to_a;
        }

        o_ptr->to_h += (HIT_PROB)randint1_signed(e_ptr->max_to_h);
        o_ptr->to_d += (HIT_POINT)randint1_signed(e_ptr->max_to_d);
        o_ptr->to_a += (ARMOUR_CLASS)randint1_signed(e_ptr->max_to_a);

        if (gen_flags.has(TRG::MOD_ACCURACY)) {
            while (o_ptr->to_h < o_ptr->to_d + 10) {
                o_ptr->to_h += 5;
                o_ptr->to_d -= 5;
            }
            o_ptr->to_h = MAX(o_ptr->to_h, 15);
        }

        if (gen_flags.has(TRG::MOD_VELOCITY)) {
            while (o_ptr->to_d < o_ptr->to_h + 10) {
                o_ptr->to_d += 5;
                o_ptr->to_h -= 5;
            }
            o_ptr->to_d = MAX(o_ptr->to_d, 15);
        }

        if ((o_ptr->name2 == EGO_PROTECTION) || (o_ptr->name2 == EGO_S_PROTECTION) || (o_ptr->name2 == EGO_H_PROTECTION)) {
            o_ptr->to_a = MAX(o_ptr->to_a, 15);
        }

        if (e_ptr->max_pval) {
            if (o_ptr->name2 == EGO_BAT) {
                o_ptr->pval = randint1(e_ptr->max_pval);
                if (o_ptr->sval == SV_ELVEN_CLOAK)
                    o_ptr->pval += randint1(2);
            } else {
                if (ego_has_flag(o_ptr, e_ptr, TR_BLOWS))
                    ego_invest_extra_attack(o_ptr, e_ptr, lev);
                else {
                    if (e_ptr->max_pval > 0)
                        o_ptr->pval += randint1(e_ptr->max_pval);
                    else if (e_ptr->max_pval < 0)
                        o_ptr->pval -= randint1(0 - e_ptr->max_pval);
                }
            }
        }

        if ((o_ptr->name2 == EGO_SPEED) && (lev < 50)) {
            o_ptr->pval = randint1(o_ptr->pval);
        }

        if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_HAYABUSA) && (o_ptr->pval > 2) && (o_ptr->name2 != EGO_ATTACKS))
            o_ptr->pval = 2;
    }
}
