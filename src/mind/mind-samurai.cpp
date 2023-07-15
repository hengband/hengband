/*!
 * @brief 剣術家のレイシャルパワー処理
 * @date 2020/05/16
 * @author Hourier
 */

#include "mind/mind-samurai.h"
#include "action/action-limited.h"
#include "avatar/avatar.h"
#include "cmd-action/cmd-attack.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-acceptor.h"
#include "mind/stances-table.h"
#include "monster-attack/monster-attack-player.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-resistance-mask.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "object-enchant/tr-types.h"
#include "pet/pet-util.h"
#include "player-attack/player-attack.h"
#include "player-base/player-class.h"
#include "player-info/samurai-data-type.h"
#include "player/attack-defense-types.h"
#include "status/action-setter.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "timed-effect/player-cut.h"
#include "timed-effect/player-fear.h"
#include "timed-effect/player-stun.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"

struct samurai_slaying_type {
    samurai_slaying_type(MULTIPLY mult, const TrFlags &flags, MonsterEntity *m_ptr, combat_options mode, MonsterRaceInfo *r_ptr);
    MULTIPLY mult;
    TrFlags flags;
    MonsterEntity *m_ptr;
    combat_options mode;
    MonsterRaceInfo *r_ptr;
};

samurai_slaying_type::samurai_slaying_type(MULTIPLY mult, const TrFlags &flags, MonsterEntity *m_ptr, combat_options mode, MonsterRaceInfo *r_ptr)
    : mult(mult)
    , flags(flags)
    , m_ptr(m_ptr)
    , mode(mode)
    , r_ptr(r_ptr)
{
}

/*!
 * @nrief 焔霊 (焼棄スレイ)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param samurai_slaying_ptr スレイ計算に必要なパラメータ群への参照ポインタ
 */
static void hissatsu_burning_strike(PlayerType *player_ptr, samurai_slaying_type *samurai_slaying_ptr)
{
    if (samurai_slaying_ptr->mode != HISSATSU_FIRE) {
        return;
    }

    /* Notice immunity */
    if (samurai_slaying_ptr->r_ptr->resistance_flags.has_any_of(RFR_EFF_IM_FIRE_MASK)) {
        if (is_original_ap_and_seen(player_ptr, samurai_slaying_ptr->m_ptr)) {
            samurai_slaying_ptr->r_ptr->r_resistance_flags.set(samurai_slaying_ptr->r_ptr->resistance_flags & RFR_EFF_IM_FIRE_MASK);
        }

        return;
    }

    /* Otherwise, take the damage */
    if (samurai_slaying_ptr->flags.has(TR_BRAND_FIRE)) {
        if (samurai_slaying_ptr->r_ptr->resistance_flags.has(MonsterResistanceType::HURT_FIRE)) {
            if (samurai_slaying_ptr->mult < 70) {
                samurai_slaying_ptr->mult = 70;
            }

            if (is_original_ap_and_seen(player_ptr, samurai_slaying_ptr->m_ptr)) {
                samurai_slaying_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::HURT_FIRE);
            }

        } else if (samurai_slaying_ptr->mult < 35) {
            samurai_slaying_ptr->mult = 35;
        }

        return;
    }

    if (samurai_slaying_ptr->r_ptr->resistance_flags.has(MonsterResistanceType::HURT_FIRE)) {
        if (samurai_slaying_ptr->mult < 50) {
            samurai_slaying_ptr->mult = 50;
        }

        if (is_original_ap_and_seen(player_ptr, samurai_slaying_ptr->m_ptr)) {
            samurai_slaying_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::HURT_FIRE);
        }
    } else if (samurai_slaying_ptr->mult < 25) {
        samurai_slaying_ptr->mult = 25;
    }
}

/*!
 * @brief サーペンツタン (毒殺スレイ)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param samurai_slaying_ptr スレイ計算に必要なパラメータ群への参照ポインタ
 */
static void hissatsu_serpent_tongue(PlayerType *player_ptr, samurai_slaying_type *samurai_slaying_ptr)
{
    if (samurai_slaying_ptr->mode != HISSATSU_POISON) {
        return;
    }

    /* Notice immunity */
    if (samurai_slaying_ptr->r_ptr->resistance_flags.has_any_of(RFR_EFF_IM_POISON_MASK)) {
        if (is_original_ap_and_seen(player_ptr, samurai_slaying_ptr->m_ptr)) {
            samurai_slaying_ptr->r_ptr->r_resistance_flags.set(samurai_slaying_ptr->r_ptr->resistance_flags & RFR_EFF_IM_POISON_MASK);
        }

        return;
    }

    /* Otherwise, take the damage */
    if (samurai_slaying_ptr->flags.has(TR_BRAND_POIS)) {
        if (samurai_slaying_ptr->mult < 35) {
            samurai_slaying_ptr->mult = 35;
        }
    } else if (samurai_slaying_ptr->mult < 25) {
        samurai_slaying_ptr->mult = 25;
    }
}

/*!
 * @brief 二重の極み^h^h^h^h^h 斬魔剣弐の太刀 (邪悪無生命スレイ)
 * @param samurai_slaying_ptr スレイ計算に必要なパラメータ群への参照ポインタ
 */
static void hissatsu_zanma_ken(samurai_slaying_type *samurai_slaying_ptr)
{
    if (samurai_slaying_ptr->mode != HISSATSU_ZANMA) {
        return;
    }

    if (!samurai_slaying_ptr->m_ptr->has_living_flag() && samurai_slaying_ptr->r_ptr->kind_flags.has(MonsterKindType::EVIL)) {
        if (samurai_slaying_ptr->mult < 15) {
            samurai_slaying_ptr->mult = 25;
        } else if (samurai_slaying_ptr->mult < 50) {
            samurai_slaying_ptr->mult = std::min<short>(50, samurai_slaying_ptr->mult + 20);
        }
    }
}

/*!
 * @brief 破岩斬 (岩石スレイ)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param samurai_slaying_ptr スレイ計算に必要なパラメータ群への参照ポインタ
 */
static void hissatsu_rock_smash(PlayerType *player_ptr, samurai_slaying_type *samurai_slaying_ptr)
{
    if (samurai_slaying_ptr->mode != HISSATSU_HAGAN) {
        return;
    }

    if (samurai_slaying_ptr->r_ptr->resistance_flags.has(MonsterResistanceType::HURT_ROCK)) {
        if (is_original_ap_and_seen(player_ptr, samurai_slaying_ptr->m_ptr)) {
            samurai_slaying_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::HURT_ROCK);
        }

        if (samurai_slaying_ptr->mult == 10) {
            samurai_slaying_ptr->mult = 40;
        } else if (samurai_slaying_ptr->mult < 60) {
            samurai_slaying_ptr->mult = 60;
        }
    }
}

/*!
 * @brief 乱れ雪月花 (冷気スレイ)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param samurai_slaying_ptr スレイ計算に必要なパラメータ群への参照ポインタ
 */
static void hissatsu_midare_setsugetsuka(PlayerType *player_ptr, samurai_slaying_type *samurai_slaying_ptr)
{
    if (samurai_slaying_ptr->mode != HISSATSU_COLD) {
        return;
    }

    /* Notice immunity */
    if (samurai_slaying_ptr->r_ptr->resistance_flags.has_any_of(RFR_EFF_IM_COLD_MASK)) {
        if (is_original_ap_and_seen(player_ptr, samurai_slaying_ptr->m_ptr)) {
            samurai_slaying_ptr->r_ptr->r_resistance_flags.set(samurai_slaying_ptr->r_ptr->resistance_flags & RFR_EFF_IM_COLD_MASK);
        }

        return;
    }

    /* Otherwise, take the damage */
    if (samurai_slaying_ptr->flags.has(TR_BRAND_COLD)) {
        if (samurai_slaying_ptr->r_ptr->resistance_flags.has(MonsterResistanceType::HURT_COLD)) {
            if (samurai_slaying_ptr->mult < 70) {
                samurai_slaying_ptr->mult = 70;
            }

            if (is_original_ap_and_seen(player_ptr, samurai_slaying_ptr->m_ptr)) {
                samurai_slaying_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::HURT_COLD);
            }
        } else if (samurai_slaying_ptr->mult < 35) {
            samurai_slaying_ptr->mult = 35;
        }

        return;
    }

    if (samurai_slaying_ptr->r_ptr->resistance_flags.has(MonsterResistanceType::HURT_COLD)) {
        if (samurai_slaying_ptr->mult < 50) {
            samurai_slaying_ptr->mult = 50;
        }

        if (is_original_ap_and_seen(player_ptr, samurai_slaying_ptr->m_ptr)) {
            samurai_slaying_ptr->r_ptr->r_resistance_flags.set(MonsterResistanceType::HURT_COLD);
        }
    } else if (samurai_slaying_ptr->mult < 25) {
        samurai_slaying_ptr->mult = 25;
    }
}

/*!
 * @brief 雷撃鷲爪斬
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param samurai_slaying_ptr スレイ計算に必要なパラメータ群への参照ポインタ
 */
static void hissatsu_lightning_eagle(PlayerType *player_ptr, samurai_slaying_type *samurai_slaying_ptr)
{
    if (samurai_slaying_ptr->mode != HISSATSU_ELEC) {
        return;
    }

    /* Notice immunity */
    if (samurai_slaying_ptr->r_ptr->resistance_flags.has_any_of(RFR_EFF_IM_ELEC_MASK)) {
        if (is_original_ap_and_seen(player_ptr, samurai_slaying_ptr->m_ptr)) {
            samurai_slaying_ptr->r_ptr->r_resistance_flags.set(samurai_slaying_ptr->r_ptr->resistance_flags & RFR_EFF_IM_ELEC_MASK);
        }

        return;
    }

    /* Otherwise, take the damage */
    if (samurai_slaying_ptr->flags.has(TR_BRAND_ELEC)) {
        if (samurai_slaying_ptr->mult < 70) {
            samurai_slaying_ptr->mult = 70;
        }
    } else if (samurai_slaying_ptr->mult < 50) {
        samurai_slaying_ptr->mult = 50;
    }
}

/*!
 * @brief 赤流渦 (ペインバッカー)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param samurai_slaying_ptr スレイ計算に必要なパラメータ群への参照ポインタ
 */
static void hissatsu_bloody_maelstroem(PlayerType *player_ptr, samurai_slaying_type *samurai_slaying_ptr)
{
    auto player_cut = player_ptr->effects()->cut();
    if ((samurai_slaying_ptr->mode == HISSATSU_SEKIRYUKA) && player_cut->is_cut() && samurai_slaying_ptr->m_ptr->has_living_flag()) {
        auto tmp = std::min<short>(100, std::max<short>(10, player_cut->current() / 10));
        if (samurai_slaying_ptr->mult < tmp) {
            samurai_slaying_ptr->mult = tmp;
        }
    }
}

/*!
 * @brief 慶雲鬼忍剣 (アンデッドスレイ)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param samurai_slaying_ptr スレイ計算に必要なパラメータ群への参照ポインタ
 */
static void hissatsu_keiun_kininken(PlayerType *player_ptr, samurai_slaying_type *samurai_slaying_ptr)
{
    if (samurai_slaying_ptr->mode != HISSATSU_UNDEAD) {
        return;
    }

    if (samurai_slaying_ptr->r_ptr->kind_flags.has(MonsterKindType::UNDEAD)) {
        if (is_original_ap_and_seen(player_ptr, samurai_slaying_ptr->m_ptr)) {
            samurai_slaying_ptr->r_ptr->r_kind_flags.set(MonsterKindType::UNDEAD);

            if (samurai_slaying_ptr->mult == 10) {
                samurai_slaying_ptr->mult = 70;
            } else if (samurai_slaying_ptr->mult < 140) {
                samurai_slaying_ptr->mult = std::min<short>(140, samurai_slaying_ptr->mult + 60);
            }
        }
    }

    if (samurai_slaying_ptr->mult == 10) {
        samurai_slaying_ptr->mult = 40;
    } else if (samurai_slaying_ptr->mult < 60) {
        samurai_slaying_ptr->mult = std::min<short>(60, samurai_slaying_ptr->mult + 30);
    }
}

/*!
 * @brief 剣術のスレイ倍率計算を行う /
 * Calcurate magnification of hissatsu technics
 * @param mult 剣術のスレイ効果以前に算出している多要素の倍率(/10倍)
 * @param flags 剣術に使用する武器のスレイフラグ配列
 * @param m_ptr 目標となるモンスターの構造体参照ポインタ
 * @param mode 剣術のスレイ型ID
 * @return スレイの倍率(/10倍)
 */
MULTIPLY mult_hissatsu(PlayerType *player_ptr, MULTIPLY mult, const TrFlags &flags, MonsterEntity *m_ptr, combat_options mode)
{
    auto *r_ptr = &monraces_info[m_ptr->r_idx];
    samurai_slaying_type tmp_slaying(mult, flags, m_ptr, mode, r_ptr);
    samurai_slaying_type *samurai_slaying_ptr = &tmp_slaying;
    hissatsu_burning_strike(player_ptr, samurai_slaying_ptr);
    hissatsu_serpent_tongue(player_ptr, samurai_slaying_ptr);
    hissatsu_zanma_ken(samurai_slaying_ptr);
    hissatsu_rock_smash(player_ptr, samurai_slaying_ptr);
    hissatsu_midare_setsugetsuka(player_ptr, samurai_slaying_ptr);
    hissatsu_lightning_eagle(player_ptr, samurai_slaying_ptr);
    hissatsu_bloody_maelstroem(player_ptr, samurai_slaying_ptr);
    hissatsu_keiun_kininken(player_ptr, samurai_slaying_ptr);

    if (samurai_slaying_ptr->mult > 150) {
        samurai_slaying_ptr->mult = 150;
    }

    return samurai_slaying_ptr->mult;
}

void concentration(PlayerType *player_ptr)
{
    int max_csp = std::max(player_ptr->msp * 4, player_ptr->lev * 5 + 5);

    if (total_friends) {
        msg_print(_("今はペットを操ることに集中していないと。", "Your pets demand all of your attention."));
        return;
    }

    if (!PlayerClass(player_ptr).samurai_stance_is(SamuraiStanceType::NONE)) {
        msg_print(_("今は構えに集中している。", "You're already concentrating on your stance."));
        return;
    }

    msg_print(_("精神を集中して気合いを溜めた。", "You concentrate to charge your power."));

    player_ptr->csp += player_ptr->msp / 2;
    if (player_ptr->csp >= max_csp) {
        player_ptr->csp = max_csp;
        player_ptr->csp_frac = 0;
    }

    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::MP);
}

/*!
 * @brief 剣術家の型設定処理
 * @return 型を変化させたらTRUE、型の構え不能かキャンセルしたらFALSEを返す。
 */
bool choose_samurai_stance(PlayerType *player_ptr)
{
    char choice;
    char buf[80];

    if (cmd_limit_confused(player_ptr)) {
        return false;
    }

    auto effects = player_ptr->effects();
    if (effects->stun()->is_stunned()) {
        msg_print(_("意識がはっきりとしない。", "You are not clear-headed"));
        return false;
    }

    if (effects->fear()->is_fearful()) {
        msg_print(_("体が震えて構えられない！", "You are trembling with fear!"));
        return false;
    }

    screen_save();
    prt(_(" a) 型を崩す", " a) No Form"), 2, 20);
    for (auto i = 0U; i < samurai_stances.size(); i++) {
        if (player_ptr->lev >= samurai_stances[i].min_level) {
            strnfmt(buf, sizeof(buf), _(" %c) %sの型    %s", " %c) Stance of %-12s  %s"), I2A(i + 1), samurai_stances[i].desc, samurai_stances[i].info);
            prt(buf, 3 + i, 20);
        }
    }

    prt("", 1, 0);
    prt(_("        どの型で構えますか？", "        Choose Stance: "), 1, 14);

    SamuraiStanceType new_stance = SamuraiStanceType::NONE;
    while (true) {
        choice = inkey();

        if (choice == ESCAPE) {
            screen_load();
            return false;
        } else if ((choice == 'a') || (choice == 'A')) {
            if (player_ptr->action == ACTION_SAMURAI_STANCE) {
                set_action(player_ptr, ACTION_NONE);
            } else {
                msg_print(_("もともと構えていない。", "You are not in a special stance."));
            }
            screen_load();
            return true;
        } else if ((choice == 'b') || (choice == 'B')) {
            new_stance = SamuraiStanceType::IAI;
            break;
        } else if (((choice == 'c') || (choice == 'C')) && (player_ptr->lev > 29)) {
            new_stance = SamuraiStanceType::FUUJIN;
            break;
        } else if (((choice == 'd') || (choice == 'D')) && (player_ptr->lev > 34)) {
            new_stance = SamuraiStanceType::KOUKIJIN;
            break;
        } else if (((choice == 'e') || (choice == 'E')) && (player_ptr->lev > 39)) {
            new_stance = SamuraiStanceType::MUSOU;
            break;
        }
    }

    set_action(player_ptr, ACTION_SAMURAI_STANCE);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if (PlayerClass(player_ptr).samurai_stance_is(new_stance)) {
        msg_print(_("構え直した。", "You reassume a stance."));
    } else {
        static constexpr auto flags_srf = {
            StatusRecalculatingFlag::BONUS,
            StatusRecalculatingFlag::MONSTER_STATUSES,
        };
        rfu.set_flags(flags_srf);
        msg_format(_("%sの型で構えた。", "You assume the %s stance."), samurai_stances[enum2i(new_stance) - 1].desc);
        PlayerClass(player_ptr).set_samurai_stance(new_stance);
    }

    static constexpr auto flags = {
        MainWindowRedrawingFlag::ACTION,
        MainWindowRedrawingFlag::TIMED_EFFECT,
    };
    rfu.set_flags(flags);
    screen_load();
    return true;
}

/*!
 * @brief 剣術家限定で、型等に応じて命中率を高める
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @return 上昇後の命中率
 */
int calc_attack_quality(PlayerType *player_ptr, player_attack_type *pa_ptr)
{
    auto *o_ptr = &player_ptr->inventory_list[INVEN_MAIN_HAND + pa_ptr->hand];
    int bonus = player_ptr->to_h[pa_ptr->hand] + o_ptr->to_h;
    int chance = (player_ptr->skill_thn + (bonus * BTH_PLUS_ADJ));
    if (pa_ptr->mode == HISSATSU_IAI) {
        chance += 60;
    }

    if (PlayerClass(player_ptr).samurai_stance_is(SamuraiStanceType::KOUKIJIN)) {
        chance += 150;
    }

    if (player_ptr->sutemi) {
        chance = std::max(chance * 3 / 2, chance + 60);
    }

    int vir = virtue_number(player_ptr, Virtue::VALOUR);
    if (vir != 0) {
        chance += (player_ptr->virtues[vir - 1] / 10);
    }

    return chance;
}

/*!
 * @brief 峰打ちの効果処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 */
void mineuchi(PlayerType *player_ptr, player_attack_type *pa_ptr)
{
    if (pa_ptr->mode != HISSATSU_MINEUCHI) {
        return;
    }

    pa_ptr->attack_damage = 0;
    anger_monster(player_ptr, pa_ptr->m_ptr);

    auto *r_ptr = &monraces_info[pa_ptr->m_ptr->r_idx];
    if ((r_ptr->flags3 & (RF3_NO_STUN))) {
        msg_format(_("%s には効果がなかった。", "%s is not effected."), pa_ptr->m_name);
        return;
    }

    int tmp = (10 + randint1(15) + player_ptr->lev / 5);
    if (pa_ptr->m_ptr->get_remaining_stun()) {
        msg_format(_("%sはひどくもうろうとした。", "%s is more dazed."), pa_ptr->m_name);
        tmp /= 2;
    } else {
        msg_format(_("%s はもうろうとした。", "%s is dazed."), pa_ptr->m_name);
    }

    (void)set_monster_stunned(player_ptr, pa_ptr->g_ptr->m_idx, pa_ptr->m_ptr->get_remaining_stun() + tmp);
}

/*!
 * @brief 無想による反撃処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 */
void musou_counterattack(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr)
{
    const auto is_musou = PlayerClass(player_ptr).samurai_stance_is(SamuraiStanceType::MUSOU);
    if ((!player_ptr->counter && !is_musou) || !monap_ptr->alive || player_ptr->is_dead || !monap_ptr->m_ptr->ml || (player_ptr->csp <= 7)) {
        return;
    }

    const auto m_target_name = monster_desc(player_ptr, monap_ptr->m_ptr, 0);
    player_ptr->csp -= 7;
    msg_format(_("%s^に反撃した！", "You counterattacked %s!"), m_target_name.data());
    do_cmd_attack(player_ptr, monap_ptr->m_ptr->fy, monap_ptr->m_ptr->fx, HISSATSU_COUNTER);
    monap_ptr->fear = false;
    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::MP);
}
