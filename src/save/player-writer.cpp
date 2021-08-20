#include "save/player-writer.h"
#include "cmd-building/cmd-building.h"
#include "dungeon/dungeon.h"
#include "game-option/birth-options.h"
#include "save/info-writer.h"
#include "save/save-util.h"
#include "system/building-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/player-type-definition.h"
#include "world/world.h"

/*!
 * @brief セーブデータに領域情報を書き込む / Write player realms
 * @param creature_ptr プレーヤーへの参照ポインタ
 */
static void wr_relams(player_type *creature_ptr)
{
    if (creature_ptr->pclass == CLASS_ELEMENTALIST)
        wr_byte((byte)creature_ptr->element);
    else
        wr_byte((byte)creature_ptr->realm1);
    wr_byte((byte)creature_ptr->realm2);
}

/*!
 * @brief セーブデータにプレーヤー情報を書き込む / Write some "player" info
 * @param creature_ptr プレーヤーへの参照ポインタ
 */
void wr_player(player_type *creature_ptr)
{
    wr_string(creature_ptr->name);
    wr_string(creature_ptr->died_from);
    wr_string(creature_ptr->last_message ? creature_ptr->last_message : "");

    save_quick_start();
    for (int i = 0; i < 4; i++)
        wr_string(creature_ptr->history[i]);

    wr_byte((byte)creature_ptr->prace);
    wr_byte((byte)creature_ptr->pclass);
    wr_byte((byte)creature_ptr->pseikaku);
    wr_byte((byte)creature_ptr->psex);
    wr_relams(creature_ptr);
    wr_byte(0);

    wr_byte((byte)creature_ptr->hitdie);
    wr_u16b(creature_ptr->expfact);

    wr_s16b(creature_ptr->age);
    wr_s16b(creature_ptr->ht);
    wr_s16b(creature_ptr->wt);

    for (int i = 0; i < A_MAX; ++i)
        wr_s16b(creature_ptr->stat_max[i]);

    for (int i = 0; i < A_MAX; ++i)
        wr_s16b(creature_ptr->stat_max_max[i]);

    for (int i = 0; i < A_MAX; ++i)
        wr_s16b(creature_ptr->stat_cur[i]);

    for (int i = 0; i < 12; ++i)
        wr_s16b(0);

    wr_u32b(creature_ptr->au);
    wr_u32b(creature_ptr->max_exp);
    wr_u32b(creature_ptr->max_max_exp);
    wr_u32b(creature_ptr->exp);
    wr_u32b(creature_ptr->exp_frac);
    wr_s16b(creature_ptr->lev);

    for (int i = 0; i < 64; i++)
        wr_s16b(creature_ptr->spell_exp[i]);

    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 64; j++)
            wr_s16b(creature_ptr->weapon_exp[i][j]);

    for (int i = 0; i < MAX_SKILLS; i++)
        wr_s16b(creature_ptr->skill_exp[i]);

    for (int i = 0; i < MAX_SPELLS; i++)
        wr_s32b(creature_ptr->magic_num1[i]);

    for (int i = 0; i < MAX_SPELLS; i++)
        wr_byte(creature_ptr->magic_num2[i]);

    wr_byte((byte)creature_ptr->start_race);
    wr_s32b(creature_ptr->old_race1);
    wr_s32b(creature_ptr->old_race2);
    wr_s16b(creature_ptr->old_realm);
    for (int i = 0; i < MAX_MANE; i++) {
        wr_s16b((int16_t)creature_ptr->mane_spell[i]);
        wr_s16b((int16_t)creature_ptr->mane_dam[i]);
    }

    wr_s16b(creature_ptr->mane_num);
    for (int i = 0; i < MAX_BOUNTY; i++)
        wr_s16b(current_world_ptr->bounty_r_idx[i]);

    for (int i = 0; i < 4; i++) {
        wr_s16b(battle_mon[i]);
        wr_u32b(mon_odds[i]);
    }

    wr_s16b(creature_ptr->town_num);

    wr_s16b(creature_ptr->arena_number);
    wr_s16b(creature_ptr->current_floor_ptr->inside_arena);
    wr_s16b(creature_ptr->current_floor_ptr->inside_quest);
    wr_s16b(creature_ptr->phase_out);
    wr_byte(creature_ptr->exit_bldg);
    wr_byte(0); /* Unused */

    wr_s16b((int16_t)creature_ptr->oldpx);
    wr_s16b((int16_t)creature_ptr->oldpy);

    wr_s16b(0);
    wr_s32b(creature_ptr->mhp);
    wr_s32b(creature_ptr->chp);
    wr_u32b(creature_ptr->chp_frac);
    wr_s32b(creature_ptr->msp);
    wr_s32b(creature_ptr->csp);
    wr_u32b(creature_ptr->csp_frac);
    wr_s16b(creature_ptr->max_plv);

    byte tmp8u = (byte)current_world_ptr->max_d_idx;
    wr_byte(tmp8u);
    for (int i = 0; i < tmp8u; i++)
        wr_s16b((int16_t)max_dlv[i]);

    wr_s16b(0);
    wr_s16b(0);
    wr_s16b(0);
    wr_s16b(0);
    wr_s16b(creature_ptr->sc);
    wr_s16b(creature_ptr->concent);

    wr_s16b(0); /* old "rest" */
    wr_s16b(creature_ptr->blind);
    wr_s16b(creature_ptr->paralyzed);
    wr_s16b(creature_ptr->confused);
    wr_s16b(creature_ptr->food);
    wr_s16b(0); /* old "food_digested" */
    wr_s16b(0); /* old "protection" */
    wr_s16b(creature_ptr->energy_need);
    wr_s16b(creature_ptr->enchant_energy_need);
    wr_s16b(creature_ptr->fast);
    wr_s16b(creature_ptr->slow);
    wr_s16b(creature_ptr->afraid);
    wr_s16b(creature_ptr->cut);
    wr_s16b(creature_ptr->stun);
    wr_s16b(creature_ptr->poisoned);
    wr_s16b(creature_ptr->image);
    wr_s16b(creature_ptr->protevil);
    wr_s16b(creature_ptr->invuln);
    wr_s16b(creature_ptr->ult_res);
    wr_s16b(creature_ptr->hero);
    wr_s16b(creature_ptr->shero);
    wr_s16b(creature_ptr->shield);
    wr_s16b(creature_ptr->blessed);
    wr_s16b(creature_ptr->tim_invis);
    wr_s16b(creature_ptr->word_recall);
    wr_s16b(creature_ptr->recall_dungeon);
    wr_s16b(creature_ptr->alter_reality);
    wr_s16b(creature_ptr->see_infra);
    wr_s16b(creature_ptr->tim_infra);
    wr_s16b(creature_ptr->oppose_fire);
    wr_s16b(creature_ptr->oppose_cold);
    wr_s16b(creature_ptr->oppose_acid);
    wr_s16b(creature_ptr->oppose_elec);
    wr_s16b(creature_ptr->oppose_pois);
    wr_s16b(creature_ptr->tsuyoshi);
    wr_s16b(creature_ptr->tim_esp);
    wr_s16b(creature_ptr->wraith_form);
    wr_s16b(creature_ptr->resist_magic);
    wr_s16b(creature_ptr->tim_regen);
    wr_s16b(creature_ptr->tim_pass_wall);
    wr_s16b(creature_ptr->tim_stealth);
    wr_s16b(creature_ptr->tim_levitation);
    wr_s16b(creature_ptr->tim_sh_touki);
    wr_s16b(creature_ptr->lightspeed);
    wr_s16b(creature_ptr->tsubureru);
    wr_s16b(creature_ptr->magicdef);
    wr_s16b(creature_ptr->tim_res_nether);
    wr_s16b(creature_ptr->tim_res_time);
    wr_byte((byte)creature_ptr->mimic_form);
    wr_s16b(creature_ptr->tim_mimic);
    wr_s16b(creature_ptr->tim_sh_fire);
    wr_s16b(creature_ptr->tim_sh_holy);
    wr_s16b(creature_ptr->tim_eyeeye);

    wr_s16b(creature_ptr->tim_reflect);
    wr_s16b(creature_ptr->multishadow);
    wr_s16b(creature_ptr->dustrobe);

    wr_s16b(creature_ptr->chaos_patron);
    wr_FlagGroup(creature_ptr->muta, wr_byte);

    for (int i = 0; i < 8; i++)
        wr_s16b(creature_ptr->virtues[i]);

    for (int i = 0; i < 8; i++)
        wr_s16b(creature_ptr->vir_types[i]);

    wr_s16b(creature_ptr->ele_attack);
    wr_u32b(creature_ptr->special_attack);
    wr_s16b(creature_ptr->ele_immune);
    wr_u32b(creature_ptr->special_defense);
    wr_byte(creature_ptr->knowledge);
    wr_byte(creature_ptr->autopick_autoregister);
    wr_byte(0);
    wr_byte((byte)creature_ptr->action);
    wr_byte(0);
    wr_byte(preserve_mode);
    wr_byte(creature_ptr->wait_report_score);

    for (int i = 0; i < 12; i++)
        wr_u32b(0L);

    /* Ignore some flags */
    wr_u32b(0L);
    wr_u32b(0L);
    wr_u32b(0L);

    wr_u32b(current_world_ptr->seed_flavor);
    wr_u32b(current_world_ptr->seed_town);
    wr_u16b(creature_ptr->panic_save);
    wr_u16b(current_world_ptr->total_winner);
    wr_u16b(current_world_ptr->noscore);
    wr_byte(creature_ptr->is_dead);
    wr_byte(creature_ptr->feeling);
    wr_s32b(creature_ptr->current_floor_ptr->generated_turn);
    wr_s32b(creature_ptr->feeling_turn);
    wr_s32b(current_world_ptr->game_turn);
    wr_s32b(current_world_ptr->dungeon_turn);
    wr_s32b(current_world_ptr->arena_start_turn);
    wr_s16b(current_world_ptr->today_mon);
    wr_s16b(creature_ptr->today_mon);
    wr_s16b(creature_ptr->riding);
    wr_s16b(creature_ptr->floor_id);

    /* Save temporary preserved pets (obsolated) */
    wr_s16b(0);
    wr_u32b(current_world_ptr->play_time);
    wr_s32b(creature_ptr->visit);
    wr_u32b(creature_ptr->count);
}
