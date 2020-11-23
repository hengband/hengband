#include "core/magic-effects-timeout-reducer.h"
#include "game-option/birth-options.h"
#include "mind/mind-force-trainer.h"
#include "mind/mind-magic-resistance.h"
#include "mind/mind-mirror-master.h"
#include "racial/racial-kutar.h"
#include "player/player-status-table.h"
#include "spell-realm/spells-craft.h"
#include "spell-realm/spells-crusade.h"
#include "spell-realm/spells-demon.h"
#include "spell-realm/spells-song.h"
#include "status/bad-status-setter.h"
#include "status/body-improvement.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "status/shape-changer.h"
#include "status/sight-setter.h"
#include "status/temporary-resistance.h"

/*!
 * @brief 10ゲームターンが進行するごとに魔法効果の残りターンを減らしていく処理
 * / Handle timeout every 10 game turns
 * @return なし
 */
void reduce_magic_effects_timeout(player_type *creature_ptr)
{
    const int dec_count = (easy_band ? 2 : 1);
    if (creature_ptr->tim_mimic) {
        (void)set_mimic(creature_ptr, creature_ptr->tim_mimic - 1, creature_ptr->mimic_form, TRUE);
    }

    if (creature_ptr->image) {
        (void)set_image(creature_ptr, creature_ptr->image - dec_count);
    }

    if (creature_ptr->blind) {
        (void)set_blind(creature_ptr, creature_ptr->blind - dec_count);
    }

    if (creature_ptr->tim_invis) {
        (void)set_tim_invis(creature_ptr, creature_ptr->tim_invis - 1, TRUE);
    }

    if (creature_ptr->suppress_multi_reward) {
        creature_ptr->suppress_multi_reward = FALSE;
    }

    if (creature_ptr->tim_esp) {
        (void)set_tim_esp(creature_ptr, creature_ptr->tim_esp - 1, TRUE);
    }

    if (creature_ptr->ele_attack) {
        creature_ptr->ele_attack--;
        if (!creature_ptr->ele_attack)
            set_ele_attack(creature_ptr, 0, 0);
    }

    if (creature_ptr->ele_immune) {
        creature_ptr->ele_immune--;
        if (!creature_ptr->ele_immune)
            set_ele_immune(creature_ptr, 0, 0);
    }

    if (creature_ptr->tim_infra) {
        (void)set_tim_infra(creature_ptr, creature_ptr->tim_infra - 1, TRUE);
    }

    if (creature_ptr->tim_stealth) {
        (void)set_tim_stealth(creature_ptr, creature_ptr->tim_stealth - 1, TRUE);
    }

    if (creature_ptr->tim_levitation) {
        (void)set_tim_levitation(creature_ptr, creature_ptr->tim_levitation - 1, TRUE);
    }

    if (creature_ptr->tim_sh_touki) {
        (void)set_tim_sh_force(creature_ptr, creature_ptr->tim_sh_touki - 1, TRUE);
    }

    if (creature_ptr->tim_sh_fire) {
        (void)set_tim_sh_fire(creature_ptr, creature_ptr->tim_sh_fire - 1, TRUE);
    }

    if (creature_ptr->tim_sh_holy) {
        (void)set_tim_sh_holy(creature_ptr, creature_ptr->tim_sh_holy - 1, TRUE);
    }

    if (creature_ptr->tim_eyeeye) {
        (void)set_tim_eyeeye(creature_ptr, creature_ptr->tim_eyeeye - 1, TRUE);
    }

    if (creature_ptr->resist_magic) {
        (void)set_resist_magic(creature_ptr, creature_ptr->resist_magic - 1, TRUE);
    }

    if (creature_ptr->tim_regen) {
        (void)set_tim_regen(creature_ptr, creature_ptr->tim_regen - 1, TRUE);
    }

    if (creature_ptr->tim_res_nether) {
        (void)set_tim_res_nether(creature_ptr, creature_ptr->tim_res_nether - 1, TRUE);
    }

    if (creature_ptr->tim_res_time) {
        (void)set_tim_res_time(creature_ptr, creature_ptr->tim_res_time - 1, TRUE);
    }

    if (creature_ptr->tim_reflect) {
        (void)set_tim_reflect(creature_ptr, creature_ptr->tim_reflect - 1, TRUE);
    }

    if (creature_ptr->multishadow) {
        (void)set_multishadow(creature_ptr, creature_ptr->multishadow - 1, TRUE);
    }

    if (creature_ptr->dustrobe) {
        (void)set_dustrobe(creature_ptr, creature_ptr->dustrobe - 1, TRUE);
    }

    if (creature_ptr->tim_pass_wall) {
        (void)set_pass_wall(creature_ptr, creature_ptr->tim_pass_wall - 1, TRUE);
    }

    if (creature_ptr->paralyzed) {
        (void)set_paralyzed(creature_ptr, creature_ptr->paralyzed - dec_count);
    }

    if (creature_ptr->confused) {
        (void)set_confused(creature_ptr, creature_ptr->confused - dec_count);
    }

    if (creature_ptr->afraid) {
        (void)set_afraid(creature_ptr, creature_ptr->afraid - dec_count);
    }

    if (creature_ptr->fast) {
        (void)set_fast(creature_ptr, creature_ptr->fast - 1, TRUE);
    }

    if (creature_ptr->slow) {
        (void)set_slow(creature_ptr, creature_ptr->slow - dec_count, TRUE);
    }

    if (creature_ptr->protevil) {
        (void)set_protevil(creature_ptr, creature_ptr->protevil - 1, TRUE);
    }

    if (creature_ptr->invuln) {
        (void)set_invuln(creature_ptr, creature_ptr->invuln - 1, TRUE);
    }

    if (creature_ptr->wraith_form) {
        (void)set_wraith_form(creature_ptr, creature_ptr->wraith_form - 1, TRUE);
    }

    if (creature_ptr->hero) {
        (void)set_hero(creature_ptr, creature_ptr->hero - 1, TRUE);
    }

    if (creature_ptr->shero) {
        (void)set_shero(creature_ptr, creature_ptr->shero - 1, TRUE);
    }

    if (creature_ptr->blessed) {
        (void)set_blessed(creature_ptr, creature_ptr->blessed - 1, TRUE);
    }

    if (creature_ptr->shield) {
        (void)set_shield(creature_ptr, creature_ptr->shield - 1, TRUE);
    }

    if (creature_ptr->tsubureru) {
        (void)set_leveling(creature_ptr, creature_ptr->tsubureru - 1, TRUE);
    }

    if (creature_ptr->magicdef) {
        (void)set_magicdef(creature_ptr, creature_ptr->magicdef - 1, TRUE);
    }

    if (creature_ptr->tsuyoshi) {
        (void)set_tsuyoshi(creature_ptr, creature_ptr->tsuyoshi - 1, TRUE);
    }

    if (creature_ptr->oppose_acid) {
        (void)set_oppose_acid(creature_ptr, creature_ptr->oppose_acid - 1, TRUE);
    }

    if (creature_ptr->oppose_elec) {
        (void)set_oppose_elec(creature_ptr, creature_ptr->oppose_elec - 1, TRUE);
    }

    if (creature_ptr->oppose_fire) {
        (void)set_oppose_fire(creature_ptr, creature_ptr->oppose_fire - 1, TRUE);
    }

    if (creature_ptr->oppose_cold) {
        (void)set_oppose_cold(creature_ptr, creature_ptr->oppose_cold - 1, TRUE);
    }

    if (creature_ptr->oppose_pois) {
        (void)set_oppose_pois(creature_ptr, creature_ptr->oppose_pois - 1, TRUE);
    }

    if (creature_ptr->ult_res) {
        (void)set_ultimate_res(creature_ptr, creature_ptr->ult_res - 1, TRUE);
    }

    if (creature_ptr->poisoned) {
        int adjust = adj_con_fix[creature_ptr->stat_ind[A_CON]] + 1;
        (void)set_poisoned(creature_ptr, creature_ptr->poisoned - adjust);
    }

    if (creature_ptr->stun) {
        int adjust = adj_con_fix[creature_ptr->stat_ind[A_CON]] + 1;
        (void)set_stun(creature_ptr, creature_ptr->stun - adjust);
    }

    if (creature_ptr->cut) {
        int adjust = adj_con_fix[creature_ptr->stat_ind[A_CON]] + 1;
        if (creature_ptr->cut > 1000)
            adjust = 0;
        (void)set_cut(creature_ptr, creature_ptr->cut - adjust);
    }
}
