/*!
 * @brief プレーヤーのステータス (麻痺等)に影響を与えるモンスターの打撃処理
 * @date 2020/05/31
 * @author Hourier
 */

#include "monster-attack/monster-attack-status.h"
#include "core/player-update-types.h"
#include "mind/mind-mirror-master.h"
#include "monster-attack/monster-attack-util.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-indice-types.h"
#include "player/player-status-flags.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "status/experience.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

void process_blind_attack(player_type *target_ptr, monap_type *monap_ptr)
{
    if (has_resist_blind(target_ptr) || check_multishadow(target_ptr)) {
        return;
    }

    auto is_dio = monap_ptr->m_ptr->r_idx == MON_DIO;
    auto dio_msg = _("どうだッ！この血の目潰しはッ！", "How is it! This blood-blinding!");
    if (is_dio && target_ptr->prace == player_race_type::SKELETON) {
        msg_print(dio_msg);
        msg_print(_("しかし、あなたには元々目はなかった！", "However, you don't have eyes!"));
        return;
    }

    if (!set_blind(target_ptr, target_ptr->blind + 10 + randint1(monap_ptr->rlev))) {
        return;
    }

    if (is_dio) {
        msg_print(dio_msg);
    }

    monap_ptr->obvious = true;
}

void process_terrify_attack(player_type *target_ptr, monap_type *monap_ptr)
{
    if (check_multishadow(target_ptr)) {
        return;
    }

    auto *r_ptr = &r_info[monap_ptr->m_ptr->r_idx];
    if (has_resist_fear(target_ptr)) {
        msg_print(_("しかし恐怖に侵されなかった！", "You stand your ground!"));
        monap_ptr->obvious = true;
        return;
    }

    if (randint0(100 + r_ptr->level / 2) < target_ptr->skill_sav) {
        msg_print(_("しかし恐怖に侵されなかった！", "You stand your ground!"));
        monap_ptr->obvious = true;
        return;
    }

    if (set_afraid(target_ptr, target_ptr->afraid + 3 + randint1(monap_ptr->rlev))) {
        monap_ptr->obvious = true;
    }
}

void process_paralyze_attack(player_type *target_ptr, monap_type *monap_ptr)
{
    if (check_multishadow(target_ptr)) {
        return;
    }

    auto *r_ptr = &r_info[monap_ptr->m_ptr->r_idx];
    if (target_ptr->free_act) {
        msg_print(_("しかし効果がなかった！", "You are unaffected!"));
        monap_ptr->obvious = true;
        return;
    }

    if (randint0(100 + r_ptr->level / 2) < target_ptr->skill_sav) {
        msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
        monap_ptr->obvious = true;
        return;
    }

    if (!target_ptr->paralyzed && set_paralyzed(target_ptr, 3 + randint1(monap_ptr->rlev))) {
        monap_ptr->obvious = true;
    }
}

void process_lose_all_attack(player_type *target_ptr, monap_type *monap_ptr)
{
    if (do_dec_stat(target_ptr, A_STR)) {
        monap_ptr->obvious = true;
    }

    if (do_dec_stat(target_ptr, A_DEX)) {
        monap_ptr->obvious = true;
    }

    if (do_dec_stat(target_ptr, A_CON)) {
        monap_ptr->obvious = true;
    }

    if (do_dec_stat(target_ptr, A_INT)) {
        monap_ptr->obvious = true;
    }

    if (do_dec_stat(target_ptr, A_WIS)) {
        monap_ptr->obvious = true;
    }

    if (do_dec_stat(target_ptr, A_CHR)) {
        monap_ptr->obvious = true;
    }
}

void process_stun_attack(player_type *target_ptr, monap_type *monap_ptr)
{
    if (has_resist_sound(target_ptr) || check_multishadow(target_ptr)) {
        return;
    }

    auto *r_ptr = &r_info[monap_ptr->m_ptr->r_idx];
    if (set_stun(target_ptr, target_ptr->stun + 10 + randint1(r_ptr->level / 4)))
        monap_ptr->obvious = true;
}

/*!
 * @brief 時間逆転攻撃による能力低下
 * @param target_ptr プレーヤーへの参照ポインタ
 * @monap_ptr モンスターからモンスターへの直接攻撃構造体への参照ポインタ
 */
static void describe_disability(player_type *target_ptr, monap_type *monap_ptr)
{
    int stat = randint0(6);
    switch (stat) {
    case A_STR:
        monap_ptr->act = _("強く", "strong");
        break;
    case A_INT:
        monap_ptr->act = _("聡明で", "bright");
        break;
    case A_WIS:
        monap_ptr->act = _("賢明で", "wise");
        break;
    case A_DEX:
        monap_ptr->act = _("器用で", "agile");
        break;
    case A_CON:
        monap_ptr->act = _("健康で", "hale");
        break;
    case A_CHR:
        monap_ptr->act = _("美しく", "beautiful");
        break;
    }

    msg_format(_("あなたは以前ほど%sなくなってしまった...。", "You're not as %s as you used to be..."), monap_ptr->act);
    target_ptr->stat_cur[stat] = (target_ptr->stat_cur[stat] * 3) / 4;
    if (target_ptr->stat_cur[stat] < 3) {
        target_ptr->stat_cur[stat] = 3;
    }
}

void process_monster_attack_time(player_type *target_ptr, monap_type *monap_ptr)
{
    if (has_resist_time(target_ptr) || check_multishadow(target_ptr)) {
        return;
    }

    switch (randint1(10)) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        if (target_ptr->prace == player_race_type::ANDROID) {
            break;
        }

        msg_print(_("人生が逆戻りした気がする。", "You feel like a chunk of the past has been ripped away."));
        lose_exp(target_ptr, 100 + (target_ptr->exp / 100) * MON_DRAIN_LIFE);
        break;
    case 6:
    case 7:
    case 8:
    case 9:
        describe_disability(target_ptr, monap_ptr);
        target_ptr->update |= (PU_BONUS);
        break;
    case 10:
        msg_print(_("あなたは以前ほど力強くなくなってしまった...。", "You're not as powerful as you used to be..."));
        for (auto i = 0; i < A_MAX; i++) {
            target_ptr->stat_cur[i] = (target_ptr->stat_cur[i] * 7) / 8;
            if (target_ptr->stat_cur[i] < 3) {
                target_ptr->stat_cur[i] = 3;
            }
        }

        target_ptr->update |= PU_BONUS;
        break;
    }
}
