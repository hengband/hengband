/*!
 * @brief 剣術家のレイシャルパワー処理
 * @date 2020/05/16
 * @author Hourier
 */

#include "racial-samurai.h"
#include "cmd/cmd-attack.h"
#include "cmd/cmd-basic.h"
#include "cmd/cmd-pet.h"
#include "monster/monster-status.h"
#include "player/player-effects.h"
#include "player/avatar.h"

void concentration(player_type* creature_ptr)
{
    int max_csp = MAX(creature_ptr->msp * 4, creature_ptr->lev * 5 + 5);

    if (total_friends) {
        msg_print(_("今はペットを操ることに集中していないと。", "Your pets demand all of your attention."));
        return;
    }

    if (creature_ptr->special_defense & KATA_MASK) {
        msg_print(_("今は構えに集中している。", "You're already concentrating on your stance."));
        return;
    }

    msg_print(_("精神を集中して気合いを溜めた。", "You concentrate to charge your power."));

    creature_ptr->csp += creature_ptr->msp / 2;
    if (creature_ptr->csp >= max_csp) {
        creature_ptr->csp = max_csp;
        creature_ptr->csp_frac = 0;
    }

    creature_ptr->redraw |= PR_MANA;
}

/*!
 * @brief 剣術家の型設定処理
 * @return 型を変化させたらTRUE、型の構え不能かキャンセルしたらFALSEを返す。
 */
bool choose_kata(player_type* creature_ptr)
{
    char choice;
    int new_kata = 0;
    int i;
    char buf[80];

    if (cmd_limit_confused(creature_ptr))
        return FALSE;

    if (creature_ptr->stun) {
        msg_print(_("意識がはっきりとしない。", "You are not clear headed"));
        return FALSE;
    }

    if (creature_ptr->afraid) {
        msg_print(_("体が震えて構えられない！", "You are trembling with fear!"));
        return FALSE;
    }

    screen_save();
    prt(_(" a) 型を崩す", " a) No Form"), 2, 20);
    for (i = 0; i < MAX_KATA; i++) {
        if (creature_ptr->lev >= kata_shurui[i].min_level) {
            sprintf(buf, _(" %c) %sの型    %s", " %c) Stance of %-12s  %s"), I2A(i + 1), kata_shurui[i].desc, kata_shurui[i].info);
            prt(buf, 3 + i, 20);
        }
    }

    prt("", 1, 0);
    prt(_("        どの型で構えますか？", "        Choose Stance: "), 1, 14);

    while (TRUE) {
        choice = inkey();

        if (choice == ESCAPE) {
            screen_load();
            return FALSE;
        } else if ((choice == 'a') || (choice == 'A')) {
            if (creature_ptr->action == ACTION_KATA) {
                set_action(creature_ptr, ACTION_NONE);
            } else
                msg_print(_("もともと構えていない。", "You are not in a special stance."));
            screen_load();
            return TRUE;
        } else if ((choice == 'b') || (choice == 'B')) {
            new_kata = 0;
            break;
        } else if (((choice == 'c') || (choice == 'C')) && (creature_ptr->lev > 29)) {
            new_kata = 1;
            break;
        } else if (((choice == 'd') || (choice == 'D')) && (creature_ptr->lev > 34)) {
            new_kata = 2;
            break;
        } else if (((choice == 'e') || (choice == 'E')) && (creature_ptr->lev > 39)) {
            new_kata = 3;
            break;
        }
    }

    set_action(creature_ptr, ACTION_KATA);
    if (creature_ptr->special_defense & (KATA_IAI << new_kata)) {
        msg_print(_("構え直した。", "You reassume a stance."));
    } else {
        creature_ptr->special_defense &= ~(KATA_MASK);
        creature_ptr->update |= (PU_BONUS | PU_MONSTERS);
        msg_format(_("%sの型で構えた。", "You assume the %s stance."), kata_shurui[new_kata].desc);
        creature_ptr->special_defense |= (KATA_IAI << new_kata);
    }

    creature_ptr->redraw |= (PR_STATE | PR_STATUS);
    screen_load();
    return TRUE;
}

/*!
 * @brief 剣術家限定で、型等に応じて命中率を高める
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @return 上昇後の命中率
 */
int calc_attack_quality(player_type *attacker_ptr, player_attack_type *pa_ptr)
{
    object_type *o_ptr = &attacker_ptr->inventory_list[INVEN_RARM + pa_ptr->hand];
    int bonus = attacker_ptr->to_h[pa_ptr->hand] + o_ptr->to_h;
    int chance = (attacker_ptr->skill_thn + (bonus * BTH_PLUS_ADJ));
    if (pa_ptr->mode == HISSATSU_IAI)
        chance += 60;

    if (attacker_ptr->special_defense & KATA_KOUKIJIN)
        chance += 150;

    if (attacker_ptr->sutemi)
        chance = MAX(chance * 3 / 2, chance + 60);

    int vir = virtue_number(attacker_ptr, V_VALOUR);
    if (vir != 0)
        chance += (attacker_ptr->virtues[vir - 1] / 10);

    return chance;
}

/*!
 * @brief 峰打ちの効果処理
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @return なし
 */
void mineuchi(player_type *attacker_ptr, player_attack_type *pa_ptr)
{
    if (pa_ptr->mode != HISSATSU_MINEUCHI)
        return;

    pa_ptr->attack_damage = 0;
    anger_monster(attacker_ptr, pa_ptr->m_ptr);

    monster_race *r_ptr = &r_info[pa_ptr->m_ptr->r_idx];
    if ((r_ptr->flags3 & (RF3_NO_STUN))) {
        msg_format(_("%s には効果がなかった。", "%s is not effected."), pa_ptr->m_name);
        return;
    }

    int tmp = (10 + randint1(15) + attacker_ptr->lev / 5);
    if (MON_STUNNED(pa_ptr->m_ptr)) {
        msg_format(_("%sはひどくもうろうとした。", "%s is more dazed."), pa_ptr->m_name);
        tmp /= 2;
    } else
        msg_format(_("%s はもうろうとした。", "%s is dazed."), pa_ptr->m_name);

    (void)set_monster_stunned(attacker_ptr, pa_ptr->g_ptr->m_idx, MON_STUNNED(pa_ptr->m_ptr) + tmp);
}

/*!
 * @brief 無想による反撃処理
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @return なし
 */
void musou_counterattack(player_type *attacker_ptr, monap_type *monap_ptr)
{
    if ((!attacker_ptr->counter && ((attacker_ptr->special_defense & KATA_MUSOU) == 0)) || !monap_ptr->alive || attacker_ptr->is_dead || !monap_ptr->m_ptr->ml
        || (attacker_ptr->csp <= 7))
        return;

    char m_target_name[MAX_NLEN];
    monster_desc(attacker_ptr, m_target_name, monap_ptr->m_ptr, 0);
    attacker_ptr->csp -= 7;
    msg_format(_("%^sに反撃した！", "You counterattacked %s!"), m_target_name);
    do_cmd_attack(attacker_ptr, monap_ptr->m_ptr->fy, monap_ptr->m_ptr->fx, HISSATSU_COUNTER);
    monap_ptr->fear = FALSE;
    attacker_ptr->redraw |= (PR_MANA);
}
