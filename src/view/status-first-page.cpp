/*!
 * @file status-first-page.c
 * @brief キャラ基本情報及び技能値の表示
 * @date 2020/02/23
 * @author Hourier
 */

#include "view/status-first-page.h"
#include "artifact/fixed-art-types.h"
#include "combat/attack-power-table.h"
#include "combat/shoot.h"
#include "game-option/text-display-options.h"
#include "inventory/inventory-slot-types.h"
#include "mutation/mutation-flag-types.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object/object-flags.h"
#include "object/tval-types.h"
#include "perception/object-perception.h"
#include "player-base/player-class.h"
#include "player-info/equipment-info.h"
#include "player-info/monk-data-type.h"
#include "player-status/player-hand-types.h"
#include "player/player-status-flags.h"
#include "player/special-defense-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "util/bit-flags-calculator.h"
#include "view/display-util.h"

/*!
 * @brief
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 装備中の弓への参照ポインタ
 * @param shots 射撃回数
 * @param shot_frac 射撃速度
 */
static void calc_shot_params(PlayerType *player_ptr, ItemEntity *o_ptr, int *shots, int *shot_frac)
{
    if (!o_ptr->is_valid()) {
        return;
    }

    const auto energy_fire = o_ptr->get_bow_energy();
    *shots = player_ptr->num_fire * 100;
    *shot_frac = ((*shots) * 100 / energy_fire) % 100;
    *shots = (*shots) / energy_fire;
    if (!o_ptr->is_specific_artifact(FixedArtifactId::CRIMSON)) {
        return;
    }

    *shots = 1;
    *shot_frac = 0;
    if (!PlayerClass(player_ptr).equals(PlayerClassType::ARCHER)) {
        return;
    }

    if (player_ptr->lev >= 10) {
        (*shots)++;
    }
    if (player_ptr->lev >= 30) {
        (*shots)++;
    }
    if (player_ptr->lev >= 45) {
        (*shots)++;
    }
}

/*!
 * @brief 武器装備に制限のあるクラスで、直接攻撃のダメージを計算する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param hand 手 (利き手が0、反対の手が1…のはず)
 * @param damage 直接攻撃のダメージ
 * @param basedam 素手における直接攻撃のダメージ
 * @param o_ptr 装備中の武器への参照ポインタ
 * @return 利き手ならTRUE、反対の手ならFALSE
 */
static bool calc_weapon_damage_limit(PlayerType *player_ptr, int hand, int *damage, int *basedam, ItemEntity *o_ptr)
{
    PLAYER_LEVEL level = player_ptr->lev;
    if (hand > 0) {
        damage[hand] = 0;
        return false;
    }

    PlayerClass pc(player_ptr);
    if (pc.equals(PlayerClassType::FORCETRAINER)) {
        level = std::max<short>(1, level - 3);
    }

    if (pc.monk_stance_is(MonkStanceType::BYAKKO)) {
        *basedam = monk_ave_damage[level][1];
    } else if (pc.monk_stance_is(MonkStanceType::GENBU) || pc.monk_stance_is(MonkStanceType::SUZAKU)) {
        *basedam = monk_ave_damage[level][2];
    } else {
        *basedam = monk_ave_damage[level][0];
    }

    damage[hand] += *basedam;
    if (o_ptr->bi_key == BaseitemKey(ItemKindType::SWORD, SV_POISON_NEEDLE)) {
        damage[hand] = 1;
    }
    if (damage[hand] < 0) {
        damage[hand] = 0;
    }

    return true;
}

/*!
 * @brief 片手あたりのダメージ量を計算する
 * @param o_ptr 装備中の武器への参照ポインタ
 * @param hand 手
 * @param damage 直接攻撃のダメージ
 * @param basedam 素手における直接攻撃のダメージ
 * @return 素手ならFALSE、武器を持っていればTRUE
 */
static bool calc_weapon_one_hand(ItemEntity *o_ptr, int hand, int *damage, int *basedam)
{
    if (!o_ptr->is_valid()) {
        return false;
    }

    *basedam = 0;
    damage[hand] += *basedam;
    if (o_ptr->bi_key == BaseitemKey(ItemKindType::SWORD, SV_POISON_NEEDLE)) {
        damage[hand] = 1;
    }

    if (damage[hand] < 0) {
        damage[hand] = 0;
    }

    return true;
}

/*!
 * @brief ヴォーパル武器等によるダメージ強化
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 装備中の武器への参照ポインタ
 * @param basedam 素手における直接攻撃のダメージ
 * @param flags オブジェクトフラグ群
 * @return 強化後の素手ダメージ
 * @todo ヴォーパル計算処理が building-craft-weapon::compare_weapon_aux() と多重実装.
 */
static int strengthen_basedam(PlayerType *player_ptr, ItemEntity *o_ptr, int basedam, const TrFlags &flags)
{
    const auto is_vorpal_blade = o_ptr->is_specific_artifact(FixedArtifactId::VORPAL_BLADE);
    const auto is_chainsword = o_ptr->is_specific_artifact(FixedArtifactId::CHAINSWORD);
    if (o_ptr->is_fully_known() && (is_vorpal_blade || is_chainsword)) {
        /* vorpal blade */
        basedam *= 5;
        basedam /= 3;
    } else if (flags.has(TR_VORPAL)) {
        /* vorpal flag only */
        basedam *= 11;
        basedam /= 9;
    }

    // 理力
    bool is_force = !PlayerClass(player_ptr).equals(PlayerClassType::SAMURAI);
    is_force &= flags.has(TR_FORCE_WEAPON);
    is_force &= player_ptr->csp > (o_ptr->dd * o_ptr->ds / 5);
    if (is_force) {
        basedam = basedam * 7 / 2;
    }

    return basedam;
}

/*!
 * @brief 技能ランクの表示基準を定める
 * Returns a "rating" of x depending on y
 * @param x 技能値
 * @param y 技能値に対するランク基準比
 * @return スキル レベルのテキスト説明とその説明のカラー インデックスのペア
 */
static std::pair<std::string, TERM_COLOR> likert(int x, int y)
{
    std::string desc;

    if (show_actual_value) {
        desc = format("%3d-", x);
    }

    if (x < 0) {
        return make_pair(desc.append(_("最低", "Very Bad")), TERM_L_DARK);
    }

    if (y <= 0) {
        y = 1;
    }

    switch ((x / y)) {
    case 0:
    case 1: {
        return make_pair(desc.append(_("悪い", "Bad")), TERM_RED);
    }
    case 2: {
        return make_pair(desc.append(_("劣る", "Poor")), TERM_L_RED);
    }
    case 3:
    case 4: {
        return make_pair(desc.append(_("普通", "Fair")), TERM_ORANGE);
    }
    case 5: {
        return make_pair(desc.append(_("良い", "Good")), TERM_YELLOW);
    }
    case 6: {
        return make_pair(desc.append(_("大変良い", "Very Good")), TERM_YELLOW);
    }
    case 7:
    case 8: {
        return make_pair(desc.append(_("卓越", "Excellent")), TERM_L_GREEN);
    }
    case 9:
    case 10:
    case 11:
    case 12:
    case 13: {
        return make_pair(desc.append(_("超越", "Superb")), TERM_GREEN);
    }
    case 14:
    case 15:
    case 16:
    case 17: {
        return make_pair(desc.append(_("英雄的", "Heroic")), TERM_BLUE);
    }
    default: {
        desc.append(format(_("伝説的[%d]", "Legendary[%d]"), (int)((((x / y) - 17) * 5) / 2)));
        return make_pair(desc, TERM_VIOLET);
    }
    }
}

/*!
 * @brief 弓＋両手の武器それぞれについてダメージを計算する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param damage 直接攻撃のダメージ
 * @param to_h 命中補正
 */
static void calc_two_hands(PlayerType *player_ptr, int *damage, int *to_h)
{
    ItemEntity *o_ptr;
    o_ptr = &player_ptr->inventory_list[INVEN_BOW];

    for (int i = 0; i < 2; i++) {
        int basedam;
        damage[i] = player_ptr->dis_to_d[i] * 100;
        PlayerClass pc(player_ptr);
        if (pc.is_martial_arts_pro() && (empty_hands(player_ptr, true) & EMPTY_HAND_MAIN)) {
            if (!calc_weapon_damage_limit(player_ptr, i, damage, &basedam, o_ptr)) {
                break;
            }

            continue;
        }

        o_ptr = &player_ptr->inventory_list[INVEN_MAIN_HAND + i];
        if (!calc_weapon_one_hand(o_ptr, i, damage, &basedam)) {
            continue;
        }

        to_h[i] = 0;
        auto poison_needle = false;
        if (o_ptr->bi_key == BaseitemKey(ItemKindType::SWORD, SV_POISON_NEEDLE)) {
            poison_needle = true;
        }

        if (o_ptr->is_known()) {
            damage[i] += o_ptr->to_d * 100;
            to_h[i] += o_ptr->to_h;
        }

        basedam = ((o_ptr->dd + player_ptr->to_dd[i]) * (o_ptr->ds + player_ptr->to_ds[i] + 1)) * 50;
        auto flags = object_flags_known(o_ptr);

        bool impact = player_ptr->impact != 0;
        basedam = calc_expect_crit(player_ptr, o_ptr->weight, to_h[i], basedam, player_ptr->dis_to_h[i], poison_needle, impact);
        basedam = strengthen_basedam(player_ptr, o_ptr, basedam, flags);
        damage[i] += basedam;
        if (o_ptr->bi_key == BaseitemKey(ItemKindType::SWORD, SV_POISON_NEEDLE)) {
            damage[i] = 1;
        }
        if (damage[i] < 0) {
            damage[i] = 0;
        }
    }
}

/*!
 * @brief キャラ基本情報及び技能値をメインウィンドウに表示する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param xthb 武器等を含めた最終命中率
 * @param damage 打撃修正
 * @param shots 射撃回数
 * @param shot_frac 射撃速度
 * @param display_player_one_line 1行表示用のコールバック関数
 */
static void display_first_page(PlayerType *player_ptr, int xthb, int *damage, int shots, int shot_frac)
{
    int xthn = player_ptr->skill_thn + (player_ptr->to_h_m * BTH_PLUS_ADJ);

    int muta_att = 0;
    if (player_ptr->muta.has(PlayerMutationType::HORNS)) {
        muta_att++;
    }
    if (player_ptr->muta.has(PlayerMutationType::SCOR_TAIL)) {
        muta_att++;
    }
    if (player_ptr->muta.has(PlayerMutationType::BEAK)) {
        muta_att++;
    }
    if (player_ptr->muta.has(PlayerMutationType::TRUNK)) {
        muta_att++;
    }
    if (player_ptr->muta.has(PlayerMutationType::TENTACLES)) {
        muta_att++;
    }

    int blows1 = can_attack_with_main_hand(player_ptr) ? player_ptr->num_blow[0] : 0;
    int blows2 = can_attack_with_sub_hand(player_ptr) ? player_ptr->num_blow[1] : 0;
    int xdis = player_ptr->skill_dis;
    int xdev = player_ptr->skill_dev;
    int xsav = player_ptr->skill_sav;
    int xstl = player_ptr->skill_stl;
    int xsrh = player_ptr->skill_srh;
    int xfos = player_ptr->skill_fos;
    int xdig = player_ptr->skill_dig;

    auto sd = likert(xthn, 12);
    display_player_one_line(ENTRY_SKILL_FIGHT, sd.first, sd.second);

    sd = likert(xthb, 12);
    display_player_one_line(ENTRY_SKILL_SHOOT, sd.first, sd.second);

    sd = likert(xsav, 7);
    display_player_one_line(ENTRY_SKILL_SAVING, sd.first, sd.second);

    sd = likert((xstl > 0) ? xstl : -1, 1);
    display_player_one_line(ENTRY_SKILL_STEALTH, sd.first, sd.second);

    sd = likert(xfos, 6);
    display_player_one_line(ENTRY_SKILL_PERCEP, sd.first, sd.second);

    sd = likert(xsrh, 6);
    display_player_one_line(ENTRY_SKILL_SEARCH, sd.first, sd.second);

    sd = likert(xdis, 8);
    display_player_one_line(ENTRY_SKILL_DISARM, sd.first, sd.second);

    sd = likert(xdev, 6);
    display_player_one_line(ENTRY_SKILL_DEVICE, sd.first, sd.second);

    sd = likert(xdig, 4);
    display_player_one_line(ENTRY_SKILL_DIG, sd.first, sd.second);

    if (!muta_att) {
        display_player_one_line(ENTRY_BLOWS, format("%d+%d", blows1, blows2), TERM_L_BLUE);
    } else {
        display_player_one_line(ENTRY_BLOWS, format("%d+%d+%d", blows1, blows2, muta_att), TERM_L_BLUE);
    }

    display_player_one_line(ENTRY_SHOTS, format("%d.%02d", shots, shot_frac), TERM_L_BLUE);

    std::string desc;
    if ((damage[0] + damage[1]) == 0) {
        desc = "nil!";
    } else {
        desc = format("%d+%d", blows1 * damage[0] / 100, blows2 * damage[1] / 100);
    }

    display_player_one_line(ENTRY_AVG_DMG, desc, TERM_L_BLUE);
    display_player_one_line(ENTRY_INFRA, format("%d feet", player_ptr->see_infra * 10), TERM_WHITE);
}

/*!
 * @brief プレイヤーステータスの1ページ目各種詳細をまとめて表示する
 * Prints ratings on certain abilities
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param display_player_one_line 1行表示用のコールバック関数
 * @details
 * This code is "imitated" elsewhere to "dump" a character sheet.
 */
void display_player_various(PlayerType *player_ptr)
{
    ItemEntity *o_ptr;
    o_ptr = &player_ptr->inventory_list[INVEN_BOW];
    int tmp = player_ptr->to_h_b + o_ptr->to_h;
    int xthb = player_ptr->skill_thb + (tmp * BTH_PLUS_ADJ);
    int shots = 0;
    int shot_frac = 0;
    calc_shot_params(player_ptr, o_ptr, &shots, &shot_frac);

    int damage[2];
    int to_h[2];
    calc_two_hands(player_ptr, damage, to_h);
    display_first_page(player_ptr, xthb, damage, shots, shot_frac);
}
