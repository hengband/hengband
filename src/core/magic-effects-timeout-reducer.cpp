#include "core/magic-effects-timeout-reducer.h"
#include "game-option/birth-options.h"
#include "mind/mind-force-trainer.h"
#include "mind/mind-magic-resistance.h"
#include "mind/mind-mirror-master.h"
#include "player/player-status-table.h"
#include "racial/racial-kutar.h"
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
#include "system/player-type-definition.h"
#include "timed-effect/player-acceleration.h"
#include "timed-effect/player-confusion.h"
#include "timed-effect/player-cut.h"
#include "timed-effect/player-fear.h"
#include "timed-effect/player-hallucination.h"
#include "timed-effect/player-paralysis.h"
#include "timed-effect/player-stun.h"
#include "timed-effect/timed-effects.h"

/*!
 * @brief 10ゲームターンが進行するごとに魔法効果の残りターンを減らしていく処理
 * / Handle timeout every 10 game turns
 */
void reduce_magic_effects_timeout(PlayerType *player_ptr)
{
    if (player_ptr->tim_mimic) {
        (void)set_mimic(player_ptr, player_ptr->tim_mimic - 1, player_ptr->mimic_form, true);
    }

    BadStatusSetter bss(player_ptr);
    auto effects = player_ptr->effects();
    if (effects->hallucination()->is_hallucinated()) {
        (void)bss.mod_hallucination(-1);
    }

    if (player_ptr->blind) {
        (void)bss.mod_blindness(-1);
    }

    if (player_ptr->tim_invis) {
        (void)set_tim_invis(player_ptr, player_ptr->tim_invis - 1, true);
    }

    if (player_ptr->suppress_multi_reward) {
        player_ptr->suppress_multi_reward = false;
    }

    if (player_ptr->tim_esp) {
        (void)set_tim_esp(player_ptr, player_ptr->tim_esp - 1, true);
    }

    if (player_ptr->ele_attack) {
        player_ptr->ele_attack--;
        if (!player_ptr->ele_attack) {
            set_ele_attack(player_ptr, 0, 0);
        }
    }

    if (player_ptr->ele_immune) {
        player_ptr->ele_immune--;
        if (!player_ptr->ele_immune) {
            set_ele_immune(player_ptr, 0, 0);
        }
    }

    if (player_ptr->tim_infra) {
        (void)set_tim_infra(player_ptr, player_ptr->tim_infra - 1, true);
    }

    if (player_ptr->tim_stealth) {
        (void)set_tim_stealth(player_ptr, player_ptr->tim_stealth - 1, true);
    }

    if (player_ptr->tim_levitation) {
        (void)set_tim_levitation(player_ptr, player_ptr->tim_levitation - 1, true);
    }

    if (player_ptr->tim_sh_touki) {
        (void)set_tim_sh_force(player_ptr, player_ptr->tim_sh_touki - 1, true);
    }

    if (player_ptr->tim_sh_fire) {
        (void)set_tim_sh_fire(player_ptr, player_ptr->tim_sh_fire - 1, true);
    }

    if (player_ptr->tim_sh_holy) {
        (void)set_tim_sh_holy(player_ptr, player_ptr->tim_sh_holy - 1, true);
    }

    if (player_ptr->tim_eyeeye) {
        (void)set_tim_eyeeye(player_ptr, player_ptr->tim_eyeeye - 1, true);
    }

    if (player_ptr->resist_magic) {
        (void)set_resist_magic(player_ptr, player_ptr->resist_magic - 1, true);
    }

    if (player_ptr->tim_regen) {
        (void)set_tim_regen(player_ptr, player_ptr->tim_regen - 1, true);
    }

    if (player_ptr->tim_res_nether) {
        (void)set_tim_res_nether(player_ptr, player_ptr->tim_res_nether - 1, true);
    }

    if (player_ptr->tim_res_time) {
        (void)set_tim_res_time(player_ptr, player_ptr->tim_res_time - 1, true);
    }

    if (player_ptr->tim_reflect) {
        (void)set_tim_reflect(player_ptr, player_ptr->tim_reflect - 1, true);
    }

    if (player_ptr->multishadow) {
        (void)set_multishadow(player_ptr, player_ptr->multishadow - 1, true);
    }

    if (player_ptr->dustrobe) {
        (void)set_dustrobe(player_ptr, player_ptr->dustrobe - 1, true);
    }

    if (player_ptr->tim_pass_wall) {
        (void)set_pass_wall(player_ptr, player_ptr->tim_pass_wall - 1, true);
    }

    if (effects->paralysis()->is_paralyzed()) {
        (void)bss.mod_paralysis(-1);
    }

    if (player_ptr->effects()->confusion()->is_confused()) {
        (void)bss.mod_confusion(-1);
    }

    if (effects->fear()->is_fearful()) {
        (void)bss.mod_fear(-1);
    }

    if (effects->acceleration()->is_fast()) {
        (void)mod_acceleration(player_ptr, -1, true);
    }

    if (player_ptr->slow) {
        (void)bss.mod_slowness(-1, true);
    }

    if (player_ptr->protevil) {
        (void)set_protevil(player_ptr, player_ptr->protevil - 1, true);
    }

    if (player_ptr->invuln) {
        (void)set_invuln(player_ptr, player_ptr->invuln - 1, true);
    }

    if (player_ptr->wraith_form) {
        (void)set_wraith_form(player_ptr, player_ptr->wraith_form - 1, true);
    }

    if (player_ptr->hero) {
        (void)set_hero(player_ptr, player_ptr->hero - 1, true);
    }

    if (player_ptr->shero) {
        (void)set_shero(player_ptr, player_ptr->shero - 1, true);
    }

    if (player_ptr->blessed) {
        (void)set_blessed(player_ptr, player_ptr->blessed - 1, true);
    }

    if (player_ptr->shield) {
        (void)set_shield(player_ptr, player_ptr->shield - 1, true);
    }

    if (player_ptr->tsubureru) {
        (void)set_leveling(player_ptr, player_ptr->tsubureru - 1, true);
    }

    if (player_ptr->magicdef) {
        (void)set_magicdef(player_ptr, player_ptr->magicdef - 1, true);
    }

    if (player_ptr->tsuyoshi) {
        (void)set_tsuyoshi(player_ptr, player_ptr->tsuyoshi - 1, true);
    }

    if (player_ptr->oppose_acid) {
        (void)set_oppose_acid(player_ptr, player_ptr->oppose_acid - 1, true);
    }

    if (player_ptr->oppose_elec) {
        (void)set_oppose_elec(player_ptr, player_ptr->oppose_elec - 1, true);
    }

    if (player_ptr->oppose_fire) {
        (void)set_oppose_fire(player_ptr, player_ptr->oppose_fire - 1, true);
    }

    if (player_ptr->oppose_cold) {
        (void)set_oppose_cold(player_ptr, player_ptr->oppose_cold - 1, true);
    }

    if (player_ptr->oppose_pois) {
        (void)set_oppose_pois(player_ptr, player_ptr->oppose_pois - 1, true);
    }

    if (player_ptr->ult_res) {
        (void)set_ultimate_res(player_ptr, player_ptr->ult_res - 1, true);
    }

    if (player_ptr->poisoned) {
        int adjust = adj_con_fix[player_ptr->stat_index[A_CON]] + 1;
        (void)bss.mod_poison(-adjust);
    }

    auto player_stun = effects->stun();
    if (player_stun->is_stunned()) {
        int adjust = adj_con_fix[player_ptr->stat_index[A_CON]] + 1;
        (void)bss.mod_stun(-adjust);
    }

    auto player_cut = effects->cut();
    if (player_cut->is_cut()) {
        short adjust = adj_con_fix[player_ptr->stat_index[A_CON]] + 1;
        if (player_cut->get_rank() == PlayerCutRank::MORTAL) {
            adjust = 0;
        }

        (void)bss.mod_cut(-adjust);
    }
}
