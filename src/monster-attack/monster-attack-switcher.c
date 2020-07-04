/*!
 * @brief モンスターからプレーヤーへの直接攻撃をその種別において振り分ける
 * @date 2020/05/31
 * @author Hourier
 * @details 長い処理はインクルード先の別ファイルにて行っている
 */

#include "monster-attack/monster-attack-switcher.h"
#include "inventory/inventory-slot-types.h"
#include "monster-attack/monster-attack-status.h"
#include "monster-attack/monster-eating.h"
#include "mind/drs-types.h"
#include "mind/mind-mirror-master.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "player/player-damage.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/spells-equipment.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "status/element-resistance.h"
#include "status/experience.h"
#include "system/object-type-definition.h"
#include "view/display-messages.h"

void switch_monster_blow_to_player(player_type *target_ptr, monap_type *monap_ptr)
{
    switch (monap_ptr->effect) {
    case RBE_NONE: {
        monap_ptr->obvious = TRUE;
        monap_ptr->damage = 0;
        break;
    }
    case RBE_SUPERHURT: { /* AC軽減あり / Player armor reduces total damage */
        if (((randint1(monap_ptr->rlev * 2 + 300) > (monap_ptr->ac + 200)) || one_in_(13)) && !check_multishadow(target_ptr)) {
            int tmp_damage = monap_ptr->damage - (monap_ptr->damage * ((monap_ptr->ac < 150) ? monap_ptr->ac : 150) / 250);
            msg_print(_("痛恨の一撃！", "It was a critical hit!"));
            tmp_damage = MAX(monap_ptr->damage, tmp_damage * 2);
            monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, tmp_damage, monap_ptr->ddesc, -1);
            break;
        }
    }
        /* Fall through */
    case RBE_HURT: { /* AC軽減あり / Player armor reduces total damage */
        monap_ptr->obvious = TRUE;
        monap_ptr->damage -= (monap_ptr->damage * ((monap_ptr->ac < 150) ? monap_ptr->ac : 150) / 250);
        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        break;
    }
    case RBE_POISON: {
        if (monap_ptr->explode)
            break;

        if (!(target_ptr->resist_pois || is_oppose_pois(target_ptr)) && !check_multishadow(target_ptr)) {
            if (set_poisoned(target_ptr, target_ptr->poisoned + randint1(monap_ptr->rlev) + 5)) {
                monap_ptr->obvious = TRUE;
            }
        }

        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        update_smart_learn(target_ptr, monap_ptr->m_idx, DRS_POIS);
        break;
    }
    case RBE_UN_BONUS: {
        if (monap_ptr->explode)
            break;

        if (!target_ptr->resist_disen && !check_multishadow(target_ptr)) {
            if (apply_disenchant(target_ptr, 0)) {
                update_creature(target_ptr);
                monap_ptr->obvious = TRUE;
            }
        }

        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        update_smart_learn(target_ptr, monap_ptr->m_idx, DRS_DISEN);
        break;
    }
    case RBE_UN_POWER: {
        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        if (target_ptr->is_dead || check_multishadow(target_ptr))
            break;

        for (int i = 0; i < 10; i++) {
            INVENTORY_IDX i_idx = (INVENTORY_IDX)randint0(INVEN_PACK);
            monap_ptr->o_ptr = &target_ptr->inventory_list[i_idx];
            if (monap_ptr->o_ptr->k_idx == 0)
                continue;

            if (process_un_power(target_ptr, monap_ptr))
                break;
        }

        break;
    }
    case RBE_EAT_GOLD: {
        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        if (monster_confused_remaining(monap_ptr->m_ptr))
            break;

        if (target_ptr->is_dead || check_multishadow(target_ptr))
            break;

        monap_ptr->obvious = TRUE;
        process_eat_gold(target_ptr, monap_ptr);
        break;
    }
    case RBE_EAT_ITEM: {
        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        if (!check_eat_item(target_ptr, monap_ptr))
            break;

        process_eat_item(target_ptr, monap_ptr);
        break;
    }

    case RBE_EAT_FOOD: {
        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        if (target_ptr->is_dead || check_multishadow(target_ptr))
            break;

        process_eat_food(target_ptr, monap_ptr);
        break;
    }
    case RBE_EAT_LITE: {
        monap_ptr->o_ptr = &target_ptr->inventory_list[INVEN_LITE];
        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        if (target_ptr->is_dead || check_multishadow(target_ptr))
            break;

        process_eat_lite(target_ptr, monap_ptr);
        break;
    }
    case RBE_ACID: {
        if (monap_ptr->explode)
            break;

        monap_ptr->obvious = TRUE;
        msg_print(_("酸を浴びせられた！", "You are covered in acid!"));
        monap_ptr->get_damage += acid_dam(target_ptr, monap_ptr->damage, monap_ptr->ddesc, -1, FALSE);
        update_creature(target_ptr);
        update_smart_learn(target_ptr, monap_ptr->m_idx, DRS_ACID);
        break;
    }
    case RBE_ELEC: {
        if (monap_ptr->explode)
            break;
        monap_ptr->obvious = TRUE;
        msg_print(_("電撃を浴びせられた！", "You are struck by electricity!"));
        monap_ptr->get_damage += elec_dam(target_ptr, monap_ptr->damage, monap_ptr->ddesc, -1, FALSE);
        update_smart_learn(target_ptr, monap_ptr->m_idx, DRS_ELEC);
        break;
    }
    case RBE_FIRE: {
        if (monap_ptr->explode)
            break;
        monap_ptr->obvious = TRUE;
        msg_print(_("全身が炎に包まれた！", "You are enveloped in flames!"));
        monap_ptr->get_damage += fire_dam(target_ptr, monap_ptr->damage, monap_ptr->ddesc, -1, FALSE);
        update_smart_learn(target_ptr, monap_ptr->m_idx, DRS_FIRE);
        break;
    }
    case RBE_COLD: {
        if (monap_ptr->explode)
            break;
        monap_ptr->obvious = TRUE;
        msg_print(_("全身が冷気で覆われた！", "You are covered with frost!"));
        monap_ptr->get_damage += cold_dam(target_ptr, monap_ptr->damage, monap_ptr->ddesc, -1, FALSE);
        update_smart_learn(target_ptr, monap_ptr->m_idx, DRS_COLD);
        break;
    }
    case RBE_BLIND: {
        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        if (target_ptr->is_dead)
            break;

        process_blind_attack(target_ptr, monap_ptr);
        update_smart_learn(target_ptr, monap_ptr->m_idx, DRS_BLIND);
        break;
    }
    case RBE_CONFUSE: {
        if (monap_ptr->explode)
            break;

        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        if (target_ptr->is_dead)
            break;

        if (!target_ptr->resist_conf && !check_multishadow(target_ptr)) {
            if (set_confused(target_ptr, target_ptr->confused + 3 + randint1(monap_ptr->rlev))) {
                monap_ptr->obvious = TRUE;
            }
        }

        update_smart_learn(target_ptr, monap_ptr->m_idx, DRS_CONF);
        break;
    }
    case RBE_TERRIFY: {
        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        if (target_ptr->is_dead)
            break;

        process_terrify_attack(target_ptr, monap_ptr);
        update_smart_learn(target_ptr, monap_ptr->m_idx, DRS_FEAR);
        break;
    }
    case RBE_PARALYZE: {
        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);

        if (target_ptr->is_dead)
            break;

        process_paralyze_attack(target_ptr, monap_ptr);
        update_smart_learn(target_ptr, monap_ptr->m_idx, DRS_FREE);
        break;
    }
    case RBE_LOSE_STR: {
        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        if (target_ptr->is_dead || check_multishadow(target_ptr))
            break;

        if (do_dec_stat(target_ptr, A_STR))
            monap_ptr->obvious = TRUE;

        break;
    }
    case RBE_LOSE_INT: {
        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        if (target_ptr->is_dead || check_multishadow(target_ptr))
            break;

        if (do_dec_stat(target_ptr, A_INT))
            monap_ptr->obvious = TRUE;

        break;
    }
    case RBE_LOSE_WIS: {
        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        if (target_ptr->is_dead || check_multishadow(target_ptr))
            break;

        if (do_dec_stat(target_ptr, A_WIS))
            monap_ptr->obvious = TRUE;

        break;
    }
    case RBE_LOSE_DEX: {
        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        if (target_ptr->is_dead || check_multishadow(target_ptr))
            break;

        if (do_dec_stat(target_ptr, A_DEX))
            monap_ptr->obvious = TRUE;

        break;
    }
    case RBE_LOSE_CON: {
        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        if (target_ptr->is_dead || check_multishadow(target_ptr))
            break;

        if (do_dec_stat(target_ptr, A_CON))
            monap_ptr->obvious = TRUE;

        break;
    }
    case RBE_LOSE_CHR: {
        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        if (target_ptr->is_dead || check_multishadow(target_ptr))
            break;

        if (do_dec_stat(target_ptr, A_CHR))
            monap_ptr->obvious = TRUE;

        break;
    }
    case RBE_LOSE_ALL: {
        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        if (target_ptr->is_dead || check_multishadow(target_ptr))
            break;

        process_lose_all_attack(target_ptr, monap_ptr);
        break;
    }
    case RBE_SHATTER: { /* AC軽減あり / Player armor reduces total damage */
        monap_ptr->obvious = TRUE;
        monap_ptr->damage -= (monap_ptr->damage * ((monap_ptr->ac < 150) ? monap_ptr->ac : 150) / 250);
        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        if (monap_ptr->damage > 23 || monap_ptr->explode)
            earthquake(target_ptr, monap_ptr->m_ptr->fy, monap_ptr->m_ptr->fx, 8, monap_ptr->m_idx);

        break;
    }
    case RBE_EXP_10: {
        s32b d = damroll(10, 6) + (target_ptr->exp / 100) * MON_DRAIN_LIFE;
        monap_ptr->obvious = TRUE;
        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        if (target_ptr->is_dead || check_multishadow(target_ptr))
            break;

        (void)drain_exp(target_ptr, d, d / 10, 95);
        break;
    }
    case RBE_EXP_20: {
        s32b d = damroll(20, 6) + (target_ptr->exp / 100) * MON_DRAIN_LIFE;
        monap_ptr->obvious = TRUE;
        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        if (target_ptr->is_dead || check_multishadow(target_ptr))
            break;

        (void)drain_exp(target_ptr, d, d / 10, 90);
        break;
    }
    case RBE_EXP_40: {
        s32b d = damroll(40, 6) + (target_ptr->exp / 100) * MON_DRAIN_LIFE;
        monap_ptr->obvious = TRUE;
        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        if (target_ptr->is_dead || check_multishadow(target_ptr))
            break;

        (void)drain_exp(target_ptr, d, d / 10, 75);
        break;
    }
    case RBE_EXP_80: {
        s32b d = damroll(80, 6) + (target_ptr->exp / 100) * MON_DRAIN_LIFE;
        monap_ptr->obvious = TRUE;
        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        if (target_ptr->is_dead || check_multishadow(target_ptr))
            break;

        (void)drain_exp(target_ptr, d, d / 10, 50);
        break;
    }
    case RBE_DISEASE: {
        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        if (target_ptr->is_dead || check_multishadow(target_ptr))
            break;

        if (!(target_ptr->resist_pois || is_oppose_pois(target_ptr))) {
            if (set_poisoned(target_ptr, target_ptr->poisoned + randint1(monap_ptr->rlev) + 5)) {
                monap_ptr->obvious = TRUE;
            }
        }

        if ((randint1(100) < 11) && (target_ptr->prace != RACE_ANDROID)) {
            bool perm = one_in_(10);
            if (dec_stat(target_ptr, A_CON, randint1(10), perm)) {
                msg_print(_("病があなたを蝕んでいる気がする。", "You feel sickly."));
                monap_ptr->obvious = TRUE;
            }
        }

        break;
    }
    case RBE_TIME: {
        if (monap_ptr->explode)
            break;

        process_monster_attack_time(target_ptr, monap_ptr);
        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        break;
    }
    case RBE_DR_LIFE: {
        s32b d = damroll(60, 6) + (target_ptr->exp / 100) * MON_DRAIN_LIFE;
        monap_ptr->obvious = TRUE;
        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        if (target_ptr->is_dead || check_multishadow(target_ptr))
            break;

        bool resist_drain = check_drain_hp(target_ptr, d);
        process_drain_life(target_ptr, monap_ptr, resist_drain);
        break;
    }
    case RBE_DR_MANA: {
        monap_ptr->obvious = TRUE;
        process_drain_mana(target_ptr, monap_ptr);
        update_smart_learn(target_ptr, monap_ptr->m_idx, DRS_MANA);
        break;
    }
    case RBE_INERTIA: {
        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        if (target_ptr->is_dead)
            break;

        if (check_multishadow(target_ptr)) {
            /* Do nothing */
        } else {
            if (set_slow(target_ptr, (target_ptr->slow + 4 + randint0(monap_ptr->rlev / 10)), FALSE)) {
                monap_ptr->obvious = TRUE;
            }
        }

        break;
    }
    case RBE_STUN: {
        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        if (target_ptr->is_dead)
            break;

        process_stun_attack(target_ptr, monap_ptr);
        break;
    }
    }
}
