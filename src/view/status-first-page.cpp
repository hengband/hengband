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
#include "mind/monk-attack.h"
#include "mutation/mutation-flag-types.h"
#include "object-enchant/tr-types.h"
#include "object/tval-types.h"
#include "perception/object-perception.h"
#include "player-base/player-class.h"
#include "player-info/equipment-info.h"
#include "player-info/monk-data-type.h"
#include "player-status/player-hand-types.h"
#include "player/player-status-flags.h"
#include "player/special-defense-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/item/item-entity.h"
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
void calc_player_shot_params(PlayerType *player_ptr, ItemEntity *o_ptr, int *shots, int *shot_frac)
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
    bool impact = player_ptr->impact != 0;
    WEIGHT weight = player_ptr->lev * calc_monk_attack_weight(player_ptr);
    int to_h = player_ptr->lev * 7 / 10; // 命中計算が煩雑なのでおよその値を使用する

    *basedam = calc_expect_crit(player_ptr, weight, to_h, *basedam, player_ptr->to_h[0], false, impact, 100);

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
 * @brief 技能値を評価段階に分類する
 * @param x 技能値
 * @param y 技能値に対するランク基準比
 * @return 評価段階と、伝説的の場合に表示する段位 (伝説的以外は0)
 */
std::pair<SkillRating, int> classify_skill_rating(int x, int y)
{
    if (x < 0) {
        return { SkillRating::VERY_BAD, 0 };
    }

    if (y <= 0) {
        y = 1;
    }

    switch ((x / y)) {
    case 0:
    case 1:
        return { SkillRating::BAD, 0 };
    case 2:
        return { SkillRating::POOR, 0 };
    case 3:
    case 4:
        return { SkillRating::FAIR, 0 };
    case 5:
        return { SkillRating::GOOD, 0 };
    case 6:
        return { SkillRating::VERY_GOOD, 0 };
    case 7:
    case 8:
        return { SkillRating::EXCELLENT, 0 };
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
        return { SkillRating::SUPERB, 0 };
    case 14:
    case 15:
    case 16:
    case 17:
        return { SkillRating::HEROIC, 0 };
    default:
        return { SkillRating::LEGENDARY, (((x / y) - 17) * 5) / 2 };
    }
}

/*!
 * @brief 技能ランクの表示基準を定める
 * Returns a "rating" of x depending on y
 * @param x 技能値
 * @param y 技能値に対するランク基準比
 * @return スキル レベルのテキスト説明とその説明のカラー インデックスのペア
 */
std::pair<std::string, TERM_COLOR> describe_skill_rating(int x, int y)
{
    std::string desc;

    if (show_actual_value) {
        desc = format("%3d-", x);
    }

    const auto [rating, legendary_level] = classify_skill_rating(x, y);
    switch (rating) {
    case SkillRating::VERY_BAD:
        return make_pair(desc.append(_("最低", "Very Bad")), TERM_L_DARK);
    case SkillRating::BAD:
        return make_pair(desc.append(_("悪い", "Bad")), TERM_RED);
    case SkillRating::POOR:
        return make_pair(desc.append(_("劣る", "Poor")), TERM_L_RED);
    case SkillRating::FAIR:
        return make_pair(desc.append(_("普通", "Fair")), TERM_ORANGE);
    case SkillRating::GOOD:
        return make_pair(desc.append(_("良い", "Good")), TERM_YELLOW);
    case SkillRating::VERY_GOOD:
        return make_pair(desc.append(_("大変良い", "Very Good")), TERM_YELLOW);
    case SkillRating::EXCELLENT:
        return make_pair(desc.append(_("卓越", "Excellent")), TERM_L_GREEN);
    case SkillRating::SUPERB:
        return make_pair(desc.append(_("超越", "Superb")), TERM_GREEN);
    case SkillRating::HEROIC:
        return make_pair(desc.append(_("英雄的", "Heroic")), TERM_BLUE);
    case SkillRating::LEGENDARY:
    default:
        desc.append(format(_("伝説的[%d]", "Legendary[%d]"), legendary_level));
        return make_pair(desc, TERM_VIOLET);
    }
}

/*!
 * @brief 弓＋両手の武器それぞれについてダメージを計算する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param damage 直接攻撃のダメージ
 * @param to_h 命中補正
 */
void calc_player_two_hands(PlayerType *player_ptr, int *damage, int *to_h)
{
    ItemEntity *o_ptr;
    o_ptr = player_ptr->inventory[INVEN_BOW].get();

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

        o_ptr = player_ptr->inventory[INVEN_MAIN_HAND + i].get();
        if (!calc_weapon_one_hand(o_ptr, i, damage, &basedam)) {
            continue;
        }

        to_h[i] = 0;

        if (o_ptr->is_known()) {
            damage[i] += o_ptr->to_d * 100;
            to_h[i] += o_ptr->to_h;
        }

        const auto mindice = (o_ptr->damage_dice.num + player_ptr->damage_dice_bonus[i].num);
        const auto maxdice = mindice * (o_ptr->damage_dice.sides + player_ptr->damage_dice_bonus[i].sides);

        basedam = calc_expect_dice(player_ptr, mindice, p_ptr->to_h[i], o_ptr);
        basedam += calc_expect_dice(player_ptr, maxdice, p_ptr->to_h[i], o_ptr);
        damage[i] += basedam * 50; // x100 for display

        if (o_ptr->bi_key == BaseitemKey(ItemKindType::SWORD, SV_POISON_NEEDLE)) {
            damage[i] = 1;
        }
        if (damage[i] < 0) {
            damage[i] = 0;
        }
    }
}

/*!
 * @brief キャラクター画面1ページ目の技能評価に使う値と基準比を求める
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 表示順の技能評価の元値
 * @details 画面表示とボット向けJSON出力が同じ値を使うために分けてある。
 */
std::vector<SkillRatingSource> calc_skill_rating_sources(PlayerType *player_ptr)
{
    const auto &bow = *player_ptr->inventory[INVEN_BOW];
    const int xthb = player_ptr->skill_thb + ((player_ptr->to_h_b + bow.to_h) * BTH_PLUS_ADJ);
    const int xthn = player_ptr->skill_thn + (player_ptr->to_h_m * BTH_PLUS_ADJ);
    const int xstl = player_ptr->skill_stl;
    return {
        { ENTRY_SKILL_FIGHT, xthn, 12 },
        { ENTRY_SKILL_SHOOT, xthb, 12 },
        { ENTRY_SKILL_SAVING, player_ptr->skill_sav, 7 },
        { ENTRY_SKILL_STEALTH, (xstl > 0) ? xstl : -1, 1 },
        { ENTRY_SKILL_PERCEP, player_ptr->skill_fos, 6 },
        { ENTRY_SKILL_SEARCH, player_ptr->skill_srh, 6 },
        { ENTRY_SKILL_DISARM, player_ptr->skill_dis, 8 },
        { ENTRY_SKILL_DEVICE, player_ptr->skill_dev, 6 },
        { ENTRY_SKILL_DIG, player_ptr->skill_dig, 4 },
    };
}

/*!
 * @brief キャラ基本情報及び技能値をメインウィンドウに表示する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param damage 打撃修正
 * @param shots 射撃回数
 * @param shot_frac 射撃速度
 */
static void display_first_page(PlayerType *player_ptr, int *damage, int shots, int shot_frac)
{
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
    for (const auto &source : calc_skill_rating_sources(player_ptr)) {
        const auto &[desc, color] = describe_skill_rating(source.value, source.divisor);
        display_player_one_line(source.entry, desc, color);
    }

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
    int shots = 0;
    int shot_frac = 0;
    calc_player_shot_params(player_ptr, player_ptr->inventory[INVEN_BOW].get(), &shots, &shot_frac);

    int damage[2];
    int to_h[2];
    calc_player_two_hands(player_ptr, damage, to_h);
    display_first_page(player_ptr, damage, shots, shot_frac);
}
