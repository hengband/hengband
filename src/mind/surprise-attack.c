#include "system/angband.h"
#include "mind/surprise-attack.h"
#include "object-enchant/trc-types.h"

/*!
 * @brief 盗賊と忍者における不意打ち
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @return なし
 */
void process_surprise_attack(player_type *attacker_ptr, player_attack_type *pa_ptr)
{
    monster_race *r_ptr = &r_info[pa_ptr->m_ptr->r_idx];
    if (!has_melee_weapon(attacker_ptr, INVEN_RARM + pa_ptr->hand) || attacker_ptr->icky_wield[pa_ptr->hand])
        return;

    int tmp = attacker_ptr->lev * 6 + (attacker_ptr->skill_stl + 10) * 4;
    if (attacker_ptr->monlite && (pa_ptr->mode != HISSATSU_NYUSIN))
        tmp /= 3;
    if (attacker_ptr->cursed & TRC_AGGRAVATE)
        tmp /= 2;
    if (r_ptr->level > (attacker_ptr->lev * attacker_ptr->lev / 20 + 10))
        tmp /= 3;
    if (MON_CSLEEP(pa_ptr->m_ptr) && pa_ptr->m_ptr->ml) {
        /* Can't backstab creatures that we can't see, right? */
        pa_ptr->backstab = TRUE;
    } else if ((attacker_ptr->special_defense & NINJA_S_STEALTH) && (randint0(tmp) > (r_ptr->level + 20)) && pa_ptr->m_ptr->ml && !(r_ptr->flagsr & RFR_RES_ALL)) {
        pa_ptr->surprise_attack = TRUE;
    } else if (MON_MONFEAR(pa_ptr->m_ptr) && pa_ptr->m_ptr->ml) {
        pa_ptr->stab_fleeing = TRUE;
    }
}

void print_surprise_attack(player_attack_type *pa_ptr)
{
    if (pa_ptr->backstab)
        msg_format(_("あなたは冷酷にも眠っている無力な%sを突き刺した！", "You cruelly stab the helpless, sleeping %s!"), pa_ptr->m_name);
    else if (pa_ptr->surprise_attack)
        msg_format(_("不意を突いて%sに強烈な一撃を喰らわせた！", "You make surprise attack, and hit %s with a powerful blow!"), pa_ptr->m_name);
    else if (pa_ptr->stab_fleeing)
        msg_format(_("逃げる%sを背中から突き刺した！", "You backstab the fleeing %s!"), pa_ptr->m_name);
    else if (!pa_ptr->monk_attack)
        msg_format(_("%sを攻撃した。", "You hit %s."), pa_ptr->m_name);
}

/*!
 * @brief 盗賊と忍者における不意打ちのダメージ計算
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @return なし
 */
void calc_surprise_attack_damage(player_type *attacker_ptr, player_attack_type *pa_ptr)
{
    if (pa_ptr->backstab) {
        pa_ptr->attack_damage *= (3 + (attacker_ptr->lev / 20));
        return;
    }

    if (pa_ptr->surprise_attack) {
        pa_ptr->attack_damage = pa_ptr->attack_damage * (5 + (attacker_ptr->lev * 2 / 25)) / 2;
        return;
    }

    if (pa_ptr->stab_fleeing)
        pa_ptr->attack_damage = (3 * pa_ptr->attack_damage) / 2;
}
