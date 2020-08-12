#include "core/status-reseter.h"
#include "core/speed-table.h"
#include "monster/monster-status-setter.h"
#include "player/attack-defense-types.h"
#include "player/player-race.h"

/*!
 * @brief プレイヤーの全ての時限効果をリセットする。 / reset timed flags
 * @return なし
 */
void reset_tim_flags(player_type *creature_ptr)
{
    creature_ptr->fast = 0; /* Timed -- Fast */
    creature_ptr->lightspeed = 0;
    creature_ptr->slow = 0; /* Timed -- Slow */
    creature_ptr->blind = 0; /* Timed -- Blindness */
    creature_ptr->paralyzed = 0; /* Timed -- Paralysis */
    creature_ptr->confused = 0; /* Timed -- Confusion */
    creature_ptr->afraid = 0; /* Timed -- Fear */
    creature_ptr->image = 0; /* Timed -- Hallucination */
    creature_ptr->poisoned = 0; /* Timed -- Poisoned */
    creature_ptr->cut = 0; /* Timed -- Cut */
    creature_ptr->stun = 0; /* Timed -- Stun */

    creature_ptr->protevil = 0; /* Timed -- Protection */
    creature_ptr->invuln = 0; /* Timed -- Invulnerable */
    creature_ptr->ult_res = 0;
    creature_ptr->hero = 0; /* Timed -- Heroism */
    creature_ptr->shero = 0; /* Timed -- Super Heroism */
    creature_ptr->shield = 0; /* Timed -- Shield Spell */
    creature_ptr->blessed = 0; /* Timed -- Blessed */
    creature_ptr->tim_invis = 0; /* Timed -- Invisibility */
    creature_ptr->tim_infra = 0; /* Timed -- Infra Vision */
    creature_ptr->tim_regen = 0; /* Timed -- Regeneration */
    creature_ptr->tim_stealth = 0; /* Timed -- Stealth */
    creature_ptr->tim_esp = 0;
    creature_ptr->wraith_form = 0; /* Timed -- Wraith Form */
    creature_ptr->tim_levitation = 0;
    creature_ptr->tim_sh_touki = 0;
    creature_ptr->tim_sh_fire = 0;
    creature_ptr->tim_sh_holy = 0;
    creature_ptr->tim_eyeeye = 0;
    creature_ptr->magicdef = 0;
    creature_ptr->resist_magic = 0;
    creature_ptr->tsuyoshi = 0;
    creature_ptr->tim_pass_wall = 0;
    creature_ptr->tim_res_nether = 0;
    creature_ptr->tim_res_time = 0;
    creature_ptr->tim_mimic = 0;
    creature_ptr->mimic_form = 0;
    creature_ptr->tim_reflect = 0;
    creature_ptr->multishadow = 0;
    creature_ptr->dustrobe = 0;
    creature_ptr->action = ACTION_NONE;

    creature_ptr->oppose_acid = 0; /* Timed -- oppose acid */
    creature_ptr->oppose_elec = 0; /* Timed -- oppose lightning */
    creature_ptr->oppose_fire = 0; /* Timed -- oppose heat */
    creature_ptr->oppose_cold = 0; /* Timed -- oppose cold */
    creature_ptr->oppose_pois = 0; /* Timed -- oppose poison */

    creature_ptr->word_recall = 0;
    creature_ptr->alter_reality = 0;
    creature_ptr->sutemi = FALSE;
    creature_ptr->counter = FALSE;
    creature_ptr->ele_attack = 0;
    creature_ptr->ele_immune = 0;
    creature_ptr->special_attack = 0L;
    creature_ptr->special_defense = 0L;

    while (creature_ptr->energy_need < 0)
        creature_ptr->energy_need += ENERGY_NEED();

    creature_ptr->timewalk = FALSE;

    if (is_specific_player_race(creature_ptr, RACE_BALROG) && (creature_ptr->lev > 44))
        creature_ptr->oppose_fire = 1;
    if ((creature_ptr->pclass == CLASS_NINJA) && (creature_ptr->lev > 44))
        creature_ptr->oppose_pois = 1;
    if (creature_ptr->pclass == CLASS_BERSERKER)
        creature_ptr->shero = 1;

    if (creature_ptr->riding) {
        (void)set_monster_fast(creature_ptr, creature_ptr->riding, 0);
        (void)set_monster_slow(creature_ptr, creature_ptr->riding, 0);
        (void)set_monster_invulner(creature_ptr, creature_ptr->riding, 0, FALSE);
    }

    if (creature_ptr->pclass == CLASS_BARD) {
        SINGING_SONG_EFFECT(creature_ptr) = 0;
        SINGING_SONG_ID(creature_ptr) = 0;
    }
}
