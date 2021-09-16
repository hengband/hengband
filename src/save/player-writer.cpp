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
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void wr_relams(player_type *player_ptr)
{
    if (player_ptr->pclass == CLASS_ELEMENTALIST)
        wr_byte((byte)player_ptr->element);
    else
        wr_byte((byte)player_ptr->realm1);
    wr_byte((byte)player_ptr->realm2);
}

/*!
 * @brief セーブデータにプレイヤー情報を書き込む / Write some "player" info
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void wr_player(player_type *player_ptr)
{
    wr_string(player_ptr->name);
    wr_string(player_ptr->died_from);
    wr_string(player_ptr->last_message ? player_ptr->last_message : "");

    save_quick_start();
    for (int i = 0; i < 4; i++)
        wr_string(player_ptr->history[i]);

    wr_byte((byte)player_ptr->prace);
    wr_byte((byte)player_ptr->pclass);
    wr_byte((byte)player_ptr->pseikaku);
    wr_byte((byte)player_ptr->psex);
    wr_relams(player_ptr);
    wr_byte(0);

    wr_byte((byte)player_ptr->hitdie);
    wr_u16b(player_ptr->expfact);

    wr_s16b(player_ptr->age);
    wr_s16b(player_ptr->ht);
    wr_s16b(player_ptr->wt);

    for (int i = 0; i < A_MAX; ++i)
        wr_s16b(player_ptr->stat_max[i]);

    for (int i = 0; i < A_MAX; ++i)
        wr_s16b(player_ptr->stat_max_max[i]);

    for (int i = 0; i < A_MAX; ++i)
        wr_s16b(player_ptr->stat_cur[i]);

    for (int i = 0; i < 12; ++i)
        wr_s16b(0);

    wr_u32b(player_ptr->au);
    wr_u32b(player_ptr->max_exp);
    wr_u32b(player_ptr->max_max_exp);
    wr_u32b(player_ptr->exp);
    wr_u32b(player_ptr->exp_frac);
    wr_s16b(player_ptr->lev);

    for (int i = 0; i < 64; i++)
        wr_s16b(player_ptr->spell_exp[i]);

    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 64; j++)
            wr_s16b(player_ptr->weapon_exp[i][j]);

    for (int i = 0; i < MAX_SKILLS; i++)
        wr_s16b(player_ptr->skill_exp[i]);

    for (int i = 0; i < MAX_SPELLS; i++)
        wr_s32b(player_ptr->magic_num1[i]);

    for (int i = 0; i < MAX_SPELLS; i++)
        wr_byte(player_ptr->magic_num2[i]);

    wr_byte((byte)player_ptr->start_race);
    wr_s32b(player_ptr->old_race1);
    wr_s32b(player_ptr->old_race2);
    wr_s16b(player_ptr->old_realm);
    for (int i = 0; i < MAX_MANE; i++) {
        wr_s16b((int16_t)player_ptr->mane_spell[i]);
        wr_s16b((int16_t)player_ptr->mane_dam[i]);
    }

    wr_s16b(player_ptr->mane_num);
    for (int i = 0; i < MAX_BOUNTY; i++)
        wr_s16b(w_ptr->bounty_r_idx[i]);

    for (int i = 0; i < 4; i++) {
        wr_s16b(battle_mon[i]);
        wr_u32b(mon_odds[i]);
    }

    wr_s16b(player_ptr->town_num);

    wr_s16b(player_ptr->arena_number);
    wr_s16b(player_ptr->current_floor_ptr->inside_arena);
    wr_s16b(player_ptr->current_floor_ptr->inside_quest);
    wr_s16b(player_ptr->phase_out);
    wr_byte(player_ptr->exit_bldg);
    wr_byte(0); /* Unused */

    wr_s16b((int16_t)player_ptr->oldpx);
    wr_s16b((int16_t)player_ptr->oldpy);

    wr_s16b(0);
    wr_s32b(player_ptr->mhp);
    wr_s32b(player_ptr->chp);
    wr_u32b(player_ptr->chp_frac);
    wr_s32b(player_ptr->msp);
    wr_s32b(player_ptr->csp);
    wr_u32b(player_ptr->csp_frac);
    wr_s16b(player_ptr->max_plv);

    byte tmp8u = (byte)w_ptr->max_d_idx;
    wr_byte(tmp8u);
    for (int i = 0; i < tmp8u; i++)
        wr_s16b((int16_t)max_dlv[i]);

    wr_s16b(0);
    wr_s16b(0);
    wr_s16b(0);
    wr_s16b(0);
    wr_s16b(player_ptr->sc);
    wr_s16b(player_ptr->concent);

    wr_s16b(0); /* old "rest" */
    wr_s16b(player_ptr->blind);
    wr_s16b(player_ptr->paralyzed);
    wr_s16b(player_ptr->confused);
    wr_s16b(player_ptr->food);
    wr_s16b(0); /* old "food_digested" */
    wr_s16b(0); /* old "protection" */
    wr_s16b(player_ptr->energy_need);
    wr_s16b(player_ptr->enchant_energy_need);
    wr_s16b(player_ptr->fast);
    wr_s16b(player_ptr->slow);
    wr_s16b(player_ptr->afraid);
    wr_s16b(player_ptr->cut);
    wr_s16b(player_ptr->stun);
    wr_s16b(player_ptr->poisoned);
    wr_s16b(player_ptr->image);
    wr_s16b(player_ptr->protevil);
    wr_s16b(player_ptr->invuln);
    wr_s16b(player_ptr->ult_res);
    wr_s16b(player_ptr->hero);
    wr_s16b(player_ptr->shero);
    wr_s16b(player_ptr->shield);
    wr_s16b(player_ptr->blessed);
    wr_s16b(player_ptr->tim_invis);
    wr_s16b(player_ptr->word_recall);
    wr_s16b(player_ptr->recall_dungeon);
    wr_s16b(player_ptr->alter_reality);
    wr_s16b(player_ptr->see_infra);
    wr_s16b(player_ptr->tim_infra);
    wr_s16b(player_ptr->oppose_fire);
    wr_s16b(player_ptr->oppose_cold);
    wr_s16b(player_ptr->oppose_acid);
    wr_s16b(player_ptr->oppose_elec);
    wr_s16b(player_ptr->oppose_pois);
    wr_s16b(player_ptr->tsuyoshi);
    wr_s16b(player_ptr->tim_esp);
    wr_s16b(player_ptr->wraith_form);
    wr_s16b(player_ptr->resist_magic);
    wr_s16b(player_ptr->tim_regen);
    wr_s16b(player_ptr->tim_pass_wall);
    wr_s16b(player_ptr->tim_stealth);
    wr_s16b(player_ptr->tim_levitation);
    wr_s16b(player_ptr->tim_sh_touki);
    wr_s16b(player_ptr->lightspeed);
    wr_s16b(player_ptr->tsubureru);
    wr_s16b(player_ptr->magicdef);
    wr_s16b(player_ptr->tim_res_nether);
    wr_s16b(player_ptr->tim_res_time);
    wr_byte((byte)player_ptr->mimic_form);
    wr_s16b(player_ptr->tim_mimic);
    wr_s16b(player_ptr->tim_sh_fire);
    wr_s16b(player_ptr->tim_sh_holy);
    wr_s16b(player_ptr->tim_eyeeye);

    wr_s16b(player_ptr->tim_reflect);
    wr_s16b(player_ptr->multishadow);
    wr_s16b(player_ptr->dustrobe);

    wr_s16b(player_ptr->chaos_patron);
    wr_FlagGroup(player_ptr->muta, wr_byte);

    for (int i = 0; i < 8; i++)
        wr_s16b(player_ptr->virtues[i]);

    for (int i = 0; i < 8; i++)
        wr_s16b(player_ptr->vir_types[i]);

    wr_s16b(player_ptr->ele_attack);
    wr_u32b(player_ptr->special_attack);
    wr_s16b(player_ptr->ele_immune);
    wr_u32b(player_ptr->special_defense);
    wr_byte(player_ptr->knowledge);
    wr_byte(player_ptr->autopick_autoregister);
    wr_byte(0);
    wr_byte((byte)player_ptr->action);
    wr_byte(0);
    wr_byte(preserve_mode);
    wr_byte(player_ptr->wait_report_score);

    for (int i = 0; i < 12; i++)
        wr_u32b(0L);

    /* Ignore some flags */
    wr_u32b(0L);
    wr_u32b(0L);
    wr_u32b(0L);

    wr_u32b(w_ptr->seed_flavor);
    wr_u32b(w_ptr->seed_town);
    wr_u16b(player_ptr->panic_save);
    wr_u16b(w_ptr->total_winner);
    wr_u16b(w_ptr->noscore);
    wr_byte(player_ptr->is_dead);
    wr_byte(player_ptr->feeling);
    wr_s32b(player_ptr->current_floor_ptr->generated_turn);
    wr_s32b(player_ptr->feeling_turn);
    wr_s32b(w_ptr->game_turn);
    wr_s32b(w_ptr->dungeon_turn);
    wr_s32b(w_ptr->arena_start_turn);
    wr_s16b(w_ptr->today_mon);
    wr_s16b(player_ptr->today_mon);
    wr_s16b(player_ptr->riding);
    wr_s16b(player_ptr->floor_id);

    /* Save temporary preserved pets (obsolated) */
    wr_s16b(0);
    wr_u32b(w_ptr->play_time);
    wr_s32b(player_ptr->visit);
    wr_u32b(player_ptr->count);
}
