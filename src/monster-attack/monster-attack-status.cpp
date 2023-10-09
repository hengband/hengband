/*!
 * @brief プレイヤーのステータス (麻痺等)に影響を与えるモンスターの打撃処理
 * @date 2020/05/31
 * @author Hourier
 */

#include "monster-attack/monster-attack-status.h"
#include "mind/mind-mirror-master.h"
#include "monster-attack/monster-attack-player.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-indice-types.h"
#include "player-base/player-race.h"
#include "player/player-status-flags.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "status/experience.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "timed-effect/player-paralysis.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"

void process_blind_attack(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr)
{
    if (has_resist_blind(player_ptr) || check_multishadow(player_ptr)) {
        return;
    }

    auto is_dio = monap_ptr->m_ptr->r_idx == MonsterRaceId::DIO;
    auto dio_msg = _("どうだッ！この血の目潰しはッ！", "How is it! This blood-blinding!");
    if (is_dio && PlayerRace(player_ptr).equals(PlayerRaceType::SKELETON)) {
        msg_print(dio_msg);
        msg_print(_("しかし、あなたには元々目はなかった！", "However, you don't have eyes!"));
        return;
    }

    if (!BadStatusSetter(player_ptr).mod_blindness(10 + randint1(monap_ptr->rlev))) {
        return;
    }

    if (is_dio) {
        msg_print(dio_msg);
    }

    monap_ptr->obvious = true;
}

void process_terrify_attack(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr)
{
    if (check_multishadow(player_ptr)) {
        return;
    }

    auto *r_ptr = &monap_ptr->m_ptr->get_monrace();
    if (has_resist_fear(player_ptr)) {
        msg_print(_("しかし恐怖に侵されなかった！", "You stand your ground!"));
        monap_ptr->obvious = true;
        return;
    }

    if (randint0(100 + r_ptr->level / 2) < player_ptr->skill_sav) {
        msg_print(_("しかし恐怖に侵されなかった！", "You stand your ground!"));
        monap_ptr->obvious = true;
        return;
    }

    if (BadStatusSetter(player_ptr).mod_fear(3 + randint1(monap_ptr->rlev))) {
        monap_ptr->obvious = true;
    }
}

void process_paralyze_attack(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr)
{
    if (check_multishadow(player_ptr)) {
        return;
    }

    auto *r_ptr = &monap_ptr->m_ptr->get_monrace();
    if (player_ptr->free_act) {
        msg_print(_("しかし効果がなかった！", "You are unaffected!"));
        monap_ptr->obvious = true;
        return;
    }

    if (randint0(100 + r_ptr->level / 2) < player_ptr->skill_sav) {
        msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
        monap_ptr->obvious = true;
        return;
    }

    auto is_paralyzed = player_ptr->effects()->paralysis()->is_paralyzed();
    if (!is_paralyzed && BadStatusSetter(player_ptr).set_paralysis(3 + randint1(monap_ptr->rlev))) {
        monap_ptr->obvious = true;
    }
}

void process_lose_all_attack(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr)
{
    if (do_dec_stat(player_ptr, A_STR)) {
        monap_ptr->obvious = true;
    }

    if (do_dec_stat(player_ptr, A_DEX)) {
        monap_ptr->obvious = true;
    }

    if (do_dec_stat(player_ptr, A_CON)) {
        monap_ptr->obvious = true;
    }

    if (do_dec_stat(player_ptr, A_INT)) {
        monap_ptr->obvious = true;
    }

    if (do_dec_stat(player_ptr, A_WIS)) {
        monap_ptr->obvious = true;
    }

    if (do_dec_stat(player_ptr, A_CHR)) {
        monap_ptr->obvious = true;
    }
}

void process_stun_attack(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr)
{
    if (has_resist_sound(player_ptr) || check_multishadow(player_ptr)) {
        return;
    }

    auto *r_ptr = &monap_ptr->m_ptr->get_monrace();
    if (BadStatusSetter(player_ptr).mod_stun(10 + randint1(r_ptr->level / 4))) {
        monap_ptr->obvious = true;
    }
}

void process_monster_attack_time(PlayerType *player_ptr)
{
    if (has_resist_time(player_ptr) || check_multishadow(player_ptr)) {
        return;
    }

    switch (randint1(10)) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        if (PlayerRace(player_ptr).equals(PlayerRaceType::ANDROID)) {
            break;
        }

        msg_print(_("人生が逆戻りした気がする。", "You feel like a chunk of the past has been ripped away."));
        lose_exp(player_ptr, 100 + (player_ptr->exp / 100) * MON_DRAIN_LIFE);
        break;
    case 6:
    case 7:
    case 8:
    case 9:
        msg_print(player_ptr->decrease_ability_random());
        break;
    case 10:
        msg_print(player_ptr->decrease_ability_all());
        break;
    }
}
