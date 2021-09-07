#include "load/player-info-loader.h"
#include "cmd-building/cmd-building.h"
#include "dungeon/dungeon.h"
#include "load/angband-version-comparer.h"
#include "load/birth-loader.h"
#include "load/dummy-loader.h"
#include "load/load-util.h"
#include "load/load-v1-3-0.h"
#include "load/load-v1-7-0.h"
#include "load/load-zangband.h"
#include "load/player-attack-loader.h"
#include "load/world-loader.h"
#include "market/arena.h"
#include "mutation/mutation-calculator.h"
#include "monster-race/race-ability-flags.h"
#include "player/attack-defense-types.h"
#include "player/player-skill.h"
#include "spell-realm/spells-song.h"
#include "system/floor-type-definition.h"
#include "system/player-type-definition.h"
#include "world/world.h"

/*!
 * @brief セーブデータから領域情報を読み込む / Read player realms
 * @param creature_ptr プレーヤーへの参照ポインタ
 */
static void rd_realms(player_type *creature_ptr)
{
    byte tmp8u;

    rd_byte(&tmp8u);
    if (creature_ptr->pclass == CLASS_ELEMENTALIST)
        creature_ptr->element = (int16_t)tmp8u;
    else
        creature_ptr->realm1 = (int16_t)tmp8u;

    rd_byte(&tmp8u);
    creature_ptr->realm2 = (int16_t)tmp8u;
    if (creature_ptr->realm2 == 255)
        creature_ptr->realm2 = 0;
}

/*!
 * @brief セーブデータからプレイヤー基本情報を読み込む / Read player's basic info
 * @param creature_ptr プレーヤーへの参照ポインタ
 */
void rd_base_info(player_type *creature_ptr)
{
    rd_string(creature_ptr->name, sizeof(creature_ptr->name));
    rd_string(creature_ptr->died_from, sizeof(creature_ptr->died_from));
    if (!h_older_than(1, 7, 0, 1)) {
        char buf[1024];
        rd_string(buf, sizeof buf);
        if (buf[0])
            creature_ptr->last_message = string_make(buf);
    }

    load_quick_start();
    const int max_history_lines = 4;
    for (int i = 0; i < max_history_lines; i++)
        rd_string(creature_ptr->history[i], sizeof(creature_ptr->history[i]));

    byte tmp8u;
    rd_byte(&tmp8u);
    creature_ptr->prace = (player_race_type)tmp8u;

    rd_byte(&tmp8u);
    creature_ptr->pclass = (player_class_type)tmp8u;

    rd_byte(&tmp8u);
    creature_ptr->pseikaku = (player_personality_type)tmp8u;

    rd_byte(&tmp8u);
    creature_ptr->psex = static_cast<player_sex>(tmp8u);

    rd_realms(creature_ptr);

    rd_byte(&tmp8u);
    if (h_older_than(0, 4, 4))
        set_zangband_realm(creature_ptr);

    rd_byte(&tmp8u);
    creature_ptr->hitdie = (DICE_SID)tmp8u;
    rd_u16b(&creature_ptr->expfact);

    rd_s16b(&creature_ptr->age);
    rd_s16b(&creature_ptr->ht);
    rd_s16b(&creature_ptr->wt);
}

void rd_experience(player_type *creature_ptr)
{
    rd_s32b(&creature_ptr->max_exp);
    if (h_older_than(1, 5, 4, 1))
        creature_ptr->max_max_exp = creature_ptr->max_exp;
    else
        rd_s32b(&creature_ptr->max_max_exp);

    rd_s32b(&creature_ptr->exp);
    if (h_older_than(1, 7, 0, 3))
        set_exp_frac_old(creature_ptr);
    else
        rd_u32b(&creature_ptr->exp_frac);

    rd_s16b(&creature_ptr->lev);
    for (int i = 0; i < 64; i++)
        rd_s16b(&creature_ptr->spell_exp[i]);

    if ((creature_ptr->pclass == CLASS_SORCERER) && h_older_than(0, 4, 2))
        for (int i = 0; i < 64; i++)
            creature_ptr->spell_exp[i] = SPELL_EXP_MASTER;

    const int max_weapon_exp_size = h_older_than(0, 3, 6) ? 60 : 64;
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < max_weapon_exp_size; j++)
            rd_s16b(&creature_ptr->weapon_exp[i][j]);

    for (int i = 0; i < MAX_SKILLS; i++)
        rd_s16b(&creature_ptr->skill_exp[i]);
}

static void set_spells(player_type *creature_ptr)
{
    for (int i = 0; i < MAX_SPELLS; i++)
        rd_s32b(&creature_ptr->magic_num1[i]);

    for (int i = 0; i < MAX_SPELLS; i++)
        rd_byte(&creature_ptr->magic_num2[i]);

    if (h_older_than(1, 3, 0, 1))
        set_spells_old(creature_ptr);
}

void rd_skills(player_type *creature_ptr)
{
    if (h_older_than(0, 4, 1))
        set_zangband_skill(creature_ptr);

    if (h_older_than(0, 3, 14))
        set_zangband_spells(creature_ptr);
    else
        set_spells(creature_ptr);

    if (music_singing_any(creature_ptr))
        creature_ptr->action = ACTION_SING;
}

static void set_race(player_type *creature_ptr)
{
    byte tmp8u;
    rd_byte(&tmp8u);
    creature_ptr->start_race = (player_race_type)tmp8u;
    int32_t tmp32s;
    rd_s32b(&tmp32s);
    creature_ptr->old_race1 = (BIT_FLAGS)tmp32s;
    rd_s32b(&tmp32s);
    creature_ptr->old_race2 = (BIT_FLAGS)tmp32s;
    rd_s16b(&creature_ptr->old_realm);
}

void rd_race(player_type *creature_ptr)
{
    if (h_older_than(1, 0, 7)) {
        set_zangband_race(creature_ptr);
        return;
    }

    set_race(creature_ptr);
}

void rd_bounty_uniques(player_type *creature_ptr)
{
    if (h_older_than(0, 0, 3)) {
        set_zangband_bounty_uniques(creature_ptr);
        return;
    }

    for (int i = 0; i < MAX_BOUNTY; i++)
        rd_s16b(&current_world_ptr->bounty_r_idx[i]);
}

/*!
 * @brief 腕力などの基本ステータス情報を読み込む
 * @param creature_ptr プレーヤーへの参照ポインタ
 */
static void rd_base_status(player_type *creature_ptr)
{
    for (int i = 0; i < A_MAX; i++)
        rd_s16b(&creature_ptr->stat_max[i]);

    for (int i = 0; i < A_MAX; i++)
        rd_s16b(&creature_ptr->stat_max_max[i]);

    for (int i = 0; i < A_MAX; i++)
        rd_s16b(&creature_ptr->stat_cur[i]);
}

static void set_imitation(player_type *creature_ptr)
{
    if (h_older_than(0, 0, 1)) {
        for (int i = 0; i < MAX_MANE; i++) {
            creature_ptr->mane_spell[i] = RF_ABILITY::MAX;
            creature_ptr->mane_dam[i] = 0;
        }

        creature_ptr->mane_num = 0;
        return;
    }

    if (h_older_than(0, 2, 3)) {
        int16_t tmp16s;
        const int OLD_MAX_MANE = 22;
        for (int i = 0; i < OLD_MAX_MANE; i++) {
            rd_s16b(&tmp16s);
            rd_s16b(&tmp16s);
        }

        for (int i = 0; i < MAX_MANE; i++) {
            creature_ptr->mane_spell[i] = RF_ABILITY::MAX;
            creature_ptr->mane_dam[i] = 0;
        }

        rd_s16b(&tmp16s);
        creature_ptr->mane_num = 0;
        return;
    }

    for (int i = 0; i < MAX_MANE; i++) {
        int16_t tmp16s;
        rd_s16b(&tmp16s);
        creature_ptr->mane_spell[i] = static_cast<RF_ABILITY>(tmp16s);
        rd_s16b(&tmp16s);
        creature_ptr->mane_dam[i] = (SPELL_IDX)tmp16s;
    }

    rd_s16b(&creature_ptr->mane_num);
}

static void rd_phase_out(player_type *creature_ptr)
{
    int16_t tmp16s;
    rd_s16b(&tmp16s);
    creature_ptr->current_floor_ptr->inside_arena = (bool)tmp16s;
    rd_s16b(&creature_ptr->current_floor_ptr->inside_quest);
    if (h_older_than(0, 3, 5))
        creature_ptr->phase_out = false;
    else {
        rd_s16b(&tmp16s);
        creature_ptr->phase_out = (bool)tmp16s;
    }
}

static void rd_arena(player_type *creature_ptr)
{
    if (h_older_than(0, 0, 3))
        update_gambling_monsters(creature_ptr);
    else
        set_gambling_monsters();

    rd_s16b(&creature_ptr->town_num);
    rd_s16b(&creature_ptr->arena_number);
    if (h_older_than(1, 5, 0, 1))
        if (creature_ptr->arena_number >= 99)
            creature_ptr->arena_number = ARENA_DEFEATED_OLD_VER;

    rd_phase_out(creature_ptr);
    byte tmp8u;
    rd_byte(&creature_ptr->exit_bldg);
    rd_byte(&tmp8u);

    int16_t tmp16s;
    rd_s16b(&tmp16s);
    creature_ptr->oldpx = (POSITION)tmp16s;
    rd_s16b(&tmp16s);
    creature_ptr->oldpy = (POSITION)tmp16s;
    if (h_older_than(0, 3, 13) && !is_in_dungeon(creature_ptr) && !creature_ptr->current_floor_ptr->inside_arena) {
        creature_ptr->oldpy = 33;
        creature_ptr->oldpx = 131;
    }
}

/*!
 * @brief プレーヤーの最大HP/現在HPを読み込む
 * @param creature_ptr プレーヤーへの参照ポインタ
 */
static void rd_hp(player_type *creature_ptr)
{
    if (h_older_than(1, 7, 0, 3)) {
        set_hp_old(creature_ptr);
        return;
    }

    rd_s32b(&creature_ptr->mhp);
    rd_s32b(&creature_ptr->chp);
    rd_u32b(&creature_ptr->chp_frac);
}

/*!
 * @brief プレーヤーの最大MP/現在MPを読み込む
 * @param creature_ptr プレーヤーへの参照ポインタ
 */
static void rd_mana(player_type *creature_ptr)
{
    if (h_older_than(1, 7, 0, 3)) {
        set_mana_old(creature_ptr);
        return;
    }

    rd_s32b(&creature_ptr->msp);
    rd_s32b(&creature_ptr->csp);
    rd_u32b(&creature_ptr->csp_frac);
}

/*!
 * @brief プレーヤーのバッドステータス (と空腹)を読み込む
 * @param creature_ptr プレーヤーへの参照ポインタ
 */
static void rd_bad_status(player_type *creature_ptr)
{
    strip_bytes(2); /* Old "rest" */
    rd_s16b(&creature_ptr->blind);
    rd_s16b(&creature_ptr->paralyzed);
    rd_s16b(&creature_ptr->confused);
    rd_s16b(&creature_ptr->food);
    strip_bytes(4); /* Old "food_digested" / "protection" */
}

static void rd_energy(player_type *creature_ptr)
{
    rd_s16b(&creature_ptr->energy_need);
    if (h_older_than(1, 0, 13))
        creature_ptr->energy_need = 100 - creature_ptr->energy_need;

    if (h_older_than(2, 1, 2, 0))
        creature_ptr->enchant_energy_need = 0;
    else
        rd_s16b(&creature_ptr->enchant_energy_need);
}

/*!
 * @brief プレーヤーのグッド/バッドステータスを読み込む
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @todo 明らかに関数名がビッグワードだが他に思いつかなかった
 */
static void rd_status(player_type *creature_ptr)
{
    rd_s16b(&creature_ptr->fast);
    rd_s16b(&creature_ptr->slow);
    rd_s16b(&creature_ptr->afraid);
    rd_s16b(&creature_ptr->cut);
    rd_s16b(&creature_ptr->stun);
    rd_s16b(&creature_ptr->poisoned);
    rd_s16b(&creature_ptr->image);
    rd_s16b(&creature_ptr->protevil);
    rd_s16b(&creature_ptr->invuln);
    if (h_older_than(0, 0, 0))
        creature_ptr->ult_res = 0;
    else
        rd_s16b(&creature_ptr->ult_res);
}

static void rd_tsuyoshi(player_type *creature_ptr)
{
    if (h_older_than(0, 0, 2))
        creature_ptr->tsuyoshi = 0;
    else
        rd_s16b(&creature_ptr->tsuyoshi);
}

static void set_timed_effects(player_type *creature_ptr)
{
    rd_s16b(&creature_ptr->tim_esp);
    rd_s16b(&creature_ptr->wraith_form);
    rd_s16b(&creature_ptr->resist_magic);
    rd_s16b(&creature_ptr->tim_regen);
    rd_s16b(&creature_ptr->tim_pass_wall);
    rd_s16b(&creature_ptr->tim_stealth);
    rd_s16b(&creature_ptr->tim_levitation);
    rd_s16b(&creature_ptr->tim_sh_touki);
    rd_s16b(&creature_ptr->lightspeed);
    rd_s16b(&creature_ptr->tsubureru);
    if (h_older_than(0, 4, 7))
        creature_ptr->magicdef = 0;
    else
        rd_s16b(&creature_ptr->magicdef);

    rd_s16b(&creature_ptr->tim_res_nether);
    if (h_older_than(0, 4, 11))
        set_zangband_mimic(creature_ptr);
    else {
        rd_s16b(&creature_ptr->tim_res_time);

        byte tmp8u;
        rd_byte(&tmp8u);
        creature_ptr->mimic_form = (IDX)tmp8u;
        rd_s16b(&creature_ptr->tim_mimic);
        rd_s16b(&creature_ptr->tim_sh_fire);
    }

    if (h_older_than(1, 0, 99))
        set_zangband_holy_aura(creature_ptr);
    else {
        rd_s16b(&creature_ptr->tim_sh_holy);
        rd_s16b(&creature_ptr->tim_eyeeye);
    }

    if (h_older_than(1, 0, 3))
        set_zangband_reflection(creature_ptr);
    else {
        rd_s16b(&creature_ptr->tim_reflect);
        rd_s16b(&creature_ptr->multishadow);
        rd_s16b(&creature_ptr->dustrobe);
    }
}

static void set_mutations(player_type *creature_ptr)
{
    if (loading_savefile_version_is_older_than(2)) {
        for (int i = 0; i < 3; i++) {
            uint32_t tmp32u;
            rd_u32b(&tmp32u);
            migrate_bitflag_to_flaggroup(creature_ptr->muta, tmp32u, i * 32);
        }
    } else {
        rd_FlagGroup(creature_ptr->muta, rd_byte);
    }
}

static void set_virtues(player_type *creature_ptr)
{
    for (int i = 0; i < 8; i++)
        rd_s16b(&creature_ptr->virtues[i]);

    for (int i = 0; i < 8; i++)
        rd_s16b(&creature_ptr->vir_types[i]);
}

/*!
 * @brief 各種時限効果を読み込む
 * @param creature_ptr プレーヤーへの参照ポインタ
 */
static void rd_timed_effects(player_type *creature_ptr)
{
    set_timed_effects(creature_ptr);
    rd_s16b(&creature_ptr->chaos_patron);
    set_mutations(creature_ptr);
    set_virtues(creature_ptr);
}

static void rd_player_status(player_type *creature_ptr)
{
    rd_base_status(creature_ptr);
    strip_bytes(24);
    rd_s32b(&creature_ptr->au);
    rd_experience(creature_ptr);
    rd_skills(creature_ptr);
    rd_race(creature_ptr);
    set_imitation(creature_ptr);
    rd_bounty_uniques(creature_ptr);
    rd_arena(creature_ptr);
    rd_dummy1();
    rd_hp(creature_ptr);
    rd_mana(creature_ptr);
    rd_s16b(&creature_ptr->max_plv);
    rd_dungeons(creature_ptr);
    strip_bytes(8);
    rd_s16b(&creature_ptr->sc);
    rd_s16b(&creature_ptr->concent);
    rd_bad_status(creature_ptr);
    rd_energy(creature_ptr);
    rd_status(creature_ptr);
    rd_s16b(&creature_ptr->hero);
    rd_s16b(&creature_ptr->shero);
    rd_s16b(&creature_ptr->shield);
    rd_s16b(&creature_ptr->blessed);
    rd_s16b(&creature_ptr->tim_invis);
    rd_s16b(&creature_ptr->word_recall);
    rd_alter_reality(creature_ptr);
    rd_s16b(&creature_ptr->see_infra);
    rd_s16b(&creature_ptr->tim_infra);
    rd_s16b(&creature_ptr->oppose_fire);
    rd_s16b(&creature_ptr->oppose_cold);
    rd_s16b(&creature_ptr->oppose_acid);
    rd_s16b(&creature_ptr->oppose_elec);
    rd_s16b(&creature_ptr->oppose_pois);
    rd_tsuyoshi(creature_ptr);
    rd_timed_effects(creature_ptr);
    creature_ptr->mutant_regenerate_mod = calc_mutant_regenerate_mod(creature_ptr);
}

void rd_player_info(player_type *creature_ptr)
{
    rd_player_status(creature_ptr);
    rd_special_attack(creature_ptr);
    rd_special_action(creature_ptr);
    rd_special_defense(creature_ptr);
    rd_byte(&creature_ptr->knowledge);
    rd_autopick(creature_ptr);
    rd_action(creature_ptr);
}
