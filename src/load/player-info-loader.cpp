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
#include "load/player-class-specific-data-loader.h"
#include "load/world-loader.h"
#include "market/arena.h"
#include "monster-race/race-ability-flags.h"
#include "mutation/mutation-calculator.h"
#include "player-base/player-class.h"
#include "player/attack-defense-types.h"
#include "player/player-skill.h"
#include "spell-realm/spells-song.h"
#include "system/floor-type-definition.h"
#include "system/player-type-definition.h"
#include "timed-effect/player-cut.h"
#include "timed-effect/player-stun.h"
#include "timed-effect/timed-effects.h"
#include "world/world.h"

/*!
 * @brief セーブデータから領域情報を読み込む / Read player realms
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void rd_realms(player_type *player_ptr)
{
    byte tmp8u;

    rd_byte(&tmp8u);
    if (player_ptr->pclass == CLASS_ELEMENTALIST)
        player_ptr->element = (int16_t)tmp8u;
    else
        player_ptr->realm1 = (int16_t)tmp8u;

    rd_byte(&tmp8u);
    player_ptr->realm2 = (int16_t)tmp8u;
    if (player_ptr->realm2 == 255)
        player_ptr->realm2 = 0;
}

/*!
 * @brief セーブデータからプレイヤー基本情報を読み込む / Read player's basic info
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void rd_base_info(player_type *player_ptr)
{
    rd_string(player_ptr->name, sizeof(player_ptr->name));
    rd_string(player_ptr->died_from, sizeof(player_ptr->died_from));
    if (!h_older_than(1, 7, 0, 1)) {
        char buf[1024];
        rd_string(buf, sizeof buf);
        if (buf[0])
            player_ptr->last_message = string_make(buf);
    }

    load_quick_start();
    const int max_history_lines = 4;
    for (int i = 0; i < max_history_lines; i++)
        rd_string(player_ptr->history[i], sizeof(player_ptr->history[i]));

    byte tmp8u;
    rd_byte(&tmp8u);
    player_ptr->prace = (player_race_type)tmp8u;

    rd_byte(&tmp8u);
    player_ptr->pclass = (player_class_type)tmp8u;

    rd_byte(&tmp8u);
    player_ptr->pseikaku = (player_personality_type)tmp8u;

    rd_byte(&tmp8u);
    player_ptr->psex = i2enum<player_sex>(tmp8u);

    rd_realms(player_ptr);

    rd_byte(&tmp8u);
    if (h_older_than(0, 4, 4))
        set_zangband_realm(player_ptr);

    rd_byte(&tmp8u);
    player_ptr->hitdie = (DICE_SID)tmp8u;
    rd_u16b(&player_ptr->expfact);

    rd_s16b(&player_ptr->age);
    rd_s16b(&player_ptr->ht);
    rd_s16b(&player_ptr->wt);
}

void rd_experience(player_type *player_ptr)
{
    rd_s32b(&player_ptr->max_exp);
    if (h_older_than(1, 5, 4, 1))
        player_ptr->max_max_exp = player_ptr->max_exp;
    else
        rd_s32b(&player_ptr->max_max_exp);

    rd_s32b(&player_ptr->exp);
    if (h_older_than(1, 7, 0, 3))
        set_exp_frac_old(player_ptr);
    else
        rd_u32b(&player_ptr->exp_frac);

    rd_s16b(&player_ptr->lev);
    for (int i = 0; i < 64; i++)
        rd_s16b(&player_ptr->spell_exp[i]);

    if ((player_ptr->pclass == CLASS_SORCERER) && h_older_than(0, 4, 2))
        for (int i = 0; i < 64; i++)
            player_ptr->spell_exp[i] = SPELL_EXP_MASTER;

    const int max_weapon_exp_size = h_older_than(0, 3, 6) ? 60 : 64;
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < max_weapon_exp_size; j++)
            rd_s16b(&player_ptr->weapon_exp[i][j]);

    for (int i = 0; i < MAX_SKILLS; i++)
        rd_s16b(&player_ptr->skill_exp[i]);
}

static void set_spells(player_type *player_ptr)
{
    for (int i = 0; i < MAX_SPELLS; i++)
        rd_s32b(&player_ptr->magic_num1[i]);

    for (int i = 0; i < MAX_SPELLS; i++)
        rd_byte(&player_ptr->magic_num2[i]);

    if (h_older_than(1, 3, 0, 1))
        set_spells_old(player_ptr);
}

void rd_skills(player_type *player_ptr)
{
    if (h_older_than(0, 4, 1))
        set_zangband_skill(player_ptr);

    if (h_older_than(0, 3, 14))
        set_zangband_spells(player_ptr);
    else
        set_spells(player_ptr);

    PlayerClass(player_ptr).init_specific_data();
    std::visit(PlayerClassSpecificDataLoader(player_ptr->magic_num1, player_ptr->magic_num2),
        player_ptr->class_specific_data);

    if (music_singing_any(player_ptr))
        player_ptr->action = ACTION_SING;
}

static void set_race(player_type *player_ptr)
{
    byte tmp8u;
    rd_byte(&tmp8u);
    player_ptr->start_race = (player_race_type)tmp8u;
    int32_t tmp32s;
    rd_s32b(&tmp32s);
    player_ptr->old_race1 = (BIT_FLAGS)tmp32s;
    rd_s32b(&tmp32s);
    player_ptr->old_race2 = (BIT_FLAGS)tmp32s;
    rd_s16b(&player_ptr->old_realm);
}

void rd_race(player_type *player_ptr)
{
    if (h_older_than(1, 0, 7)) {
        set_zangband_race(player_ptr);
        return;
    }

    set_race(player_ptr);
}

void rd_bounty_uniques(player_type *player_ptr)
{
    if (h_older_than(0, 0, 3)) {
        set_zangband_bounty_uniques(player_ptr);
        return;
    }

    for (int i = 0; i < MAX_BOUNTY; i++)
        rd_s16b(&w_ptr->bounty_r_idx[i]);
}

/*!
 * @brief 腕力などの基本ステータス情報を読み込む
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void rd_base_status(player_type *player_ptr)
{
    for (int i = 0; i < A_MAX; i++)
        rd_s16b(&player_ptr->stat_max[i]);

    for (int i = 0; i < A_MAX; i++)
        rd_s16b(&player_ptr->stat_max_max[i]);

    for (int i = 0; i < A_MAX; i++)
        rd_s16b(&player_ptr->stat_cur[i]);
}

static void set_imitation(player_type *player_ptr)
{
    if (h_older_than(0, 0, 1)) {
        for (int i = 0; i < MAX_MANE; i++) {
            player_ptr->mane_spell[i] = RF_ABILITY::MAX;
            player_ptr->mane_dam[i] = 0;
        }

        player_ptr->mane_num = 0;
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
            player_ptr->mane_spell[i] = RF_ABILITY::MAX;
            player_ptr->mane_dam[i] = 0;
        }

        rd_s16b(&tmp16s);
        player_ptr->mane_num = 0;
        return;
    }

    for (int i = 0; i < MAX_MANE; i++) {
        int16_t tmp16s;
        rd_s16b(&tmp16s);
        player_ptr->mane_spell[i] = i2enum<RF_ABILITY>(tmp16s);
        rd_s16b(&tmp16s);
        player_ptr->mane_dam[i] = (SPELL_IDX)tmp16s;
    }

    rd_s16b(&player_ptr->mane_num);
}

static void rd_phase_out(player_type *player_ptr)
{
    int16_t tmp16s;
    rd_s16b(&tmp16s);
    player_ptr->current_floor_ptr->inside_arena = (bool)tmp16s;
    rd_s16b(&player_ptr->current_floor_ptr->inside_quest);
    if (h_older_than(0, 3, 5))
        player_ptr->phase_out = false;
    else {
        rd_s16b(&tmp16s);
        player_ptr->phase_out = (bool)tmp16s;
    }
}

static void rd_arena(player_type *player_ptr)
{
    if (h_older_than(0, 0, 3))
        update_gambling_monsters(player_ptr);
    else
        set_gambling_monsters();

    rd_s16b(&player_ptr->town_num);
    rd_s16b(&player_ptr->arena_number);
    if (h_older_than(1, 5, 0, 1))
        if (player_ptr->arena_number >= 99)
            player_ptr->arena_number = ARENA_DEFEATED_OLD_VER;

    rd_phase_out(player_ptr);
    byte tmp8u;
    rd_byte(&player_ptr->exit_bldg);
    rd_byte(&tmp8u);

    int16_t tmp16s;
    rd_s16b(&tmp16s);
    player_ptr->oldpx = (POSITION)tmp16s;
    rd_s16b(&tmp16s);
    player_ptr->oldpy = (POSITION)tmp16s;
    if (h_older_than(0, 3, 13) && !is_in_dungeon(player_ptr) && !player_ptr->current_floor_ptr->inside_arena) {
        player_ptr->oldpy = 33;
        player_ptr->oldpx = 131;
    }
}

/*!
 * @brief プレイヤーの最大HP/現在HPを読み込む
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void rd_hp(player_type *player_ptr)
{
    if (h_older_than(1, 7, 0, 3)) {
        set_hp_old(player_ptr);
        return;
    }

    rd_s32b(&player_ptr->mhp);
    rd_s32b(&player_ptr->chp);
    rd_u32b(&player_ptr->chp_frac);
}

/*!
 * @brief プレイヤーの最大MP/現在MPを読み込む
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void rd_mana(player_type *player_ptr)
{
    if (h_older_than(1, 7, 0, 3)) {
        set_mana_old(player_ptr);
        return;
    }

    rd_s32b(&player_ptr->msp);
    rd_s32b(&player_ptr->csp);
    rd_u32b(&player_ptr->csp_frac);
}

/*!
 * @brief プレイヤーのバッドステータス (と空腹)を読み込む
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void rd_bad_status(player_type *player_ptr)
{
    strip_bytes(2); /* Old "rest" */
    rd_s16b(&player_ptr->blind);
    rd_s16b(&player_ptr->paralyzed);
    rd_s16b(&player_ptr->confused);
    rd_s16b(&player_ptr->food);
    strip_bytes(4); /* Old "food_digested" / "protection" */
}

static void rd_energy(player_type *player_ptr)
{
    rd_s16b(&player_ptr->energy_need);
    if (h_older_than(1, 0, 13))
        player_ptr->energy_need = 100 - player_ptr->energy_need;

    if (h_older_than(2, 1, 2, 0))
        player_ptr->enchant_energy_need = 0;
    else
        rd_s16b(&player_ptr->enchant_energy_need);
}

/*!
 * @brief プレイヤーのグッド/バッドステータスを読み込む
 * @param player_ptr プレイヤーへの参照ポインタ
 * @todo 明らかに関数名がビッグワードだが他に思いつかなかった
 */
static void rd_status(player_type *player_ptr)
{
    int16_t tmp16s;
    rd_s16b(&player_ptr->fast);
    rd_s16b(&player_ptr->slow);
    rd_s16b(&player_ptr->afraid);
    rd_s16b(&tmp16s);
    player_ptr->effects()->cut()->set(tmp16s);
    rd_s16b(&tmp16s);
    player_ptr->effects()->stun()->set(tmp16s);
    rd_s16b(&player_ptr->poisoned);
    rd_s16b(&player_ptr->hallucinated);
    rd_s16b(&player_ptr->protevil);
    rd_s16b(&player_ptr->invuln);
    if (h_older_than(0, 0, 0))
        player_ptr->ult_res = 0;
    else
        rd_s16b(&player_ptr->ult_res);
}

static void rd_tsuyoshi(player_type *player_ptr)
{
    if (h_older_than(0, 0, 2))
        player_ptr->tsuyoshi = 0;
    else
        rd_s16b(&player_ptr->tsuyoshi);
}

static void set_timed_effects(player_type *player_ptr)
{
    rd_s16b(&player_ptr->tim_esp);
    rd_s16b(&player_ptr->wraith_form);
    rd_s16b(&player_ptr->resist_magic);
    rd_s16b(&player_ptr->tim_regen);
    rd_s16b(&player_ptr->tim_pass_wall);
    rd_s16b(&player_ptr->tim_stealth);
    rd_s16b(&player_ptr->tim_levitation);
    rd_s16b(&player_ptr->tim_sh_touki);
    rd_s16b(&player_ptr->lightspeed);
    rd_s16b(&player_ptr->tsubureru);
    if (h_older_than(0, 4, 7))
        player_ptr->magicdef = 0;
    else
        rd_s16b(&player_ptr->magicdef);

    rd_s16b(&player_ptr->tim_res_nether);
    if (h_older_than(0, 4, 11))
        set_zangband_mimic(player_ptr);
    else {
        rd_s16b(&player_ptr->tim_res_time);

        byte tmp8u;
        rd_byte(&tmp8u);
        player_ptr->mimic_form = (IDX)tmp8u;
        rd_s16b(&player_ptr->tim_mimic);
        rd_s16b(&player_ptr->tim_sh_fire);
    }

    if (h_older_than(1, 0, 99))
        set_zangband_holy_aura(player_ptr);
    else {
        rd_s16b(&player_ptr->tim_sh_holy);
        rd_s16b(&player_ptr->tim_eyeeye);
    }

    if (h_older_than(1, 0, 3))
        set_zangband_reflection(player_ptr);
    else {
        rd_s16b(&player_ptr->tim_reflect);
        rd_s16b(&player_ptr->multishadow);
        rd_s16b(&player_ptr->dustrobe);
    }
}

static void set_mutations(player_type *player_ptr)
{
    if (loading_savefile_version_is_older_than(2)) {
        for (int i = 0; i < 3; i++) {
            uint32_t tmp32u;
            rd_u32b(&tmp32u);
            migrate_bitflag_to_flaggroup(player_ptr->muta, tmp32u, i * 32);
        }
    } else {
        rd_FlagGroup(player_ptr->muta, rd_byte);
    }
}

static void set_virtues(player_type *player_ptr)
{
    for (int i = 0; i < 8; i++)
        rd_s16b(&player_ptr->virtues[i]);

    for (int i = 0; i < 8; i++)
        rd_s16b(&player_ptr->vir_types[i]);
}

/*!
 * @brief 各種時限効果を読み込む
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void rd_timed_effects(player_type *player_ptr)
{
    set_timed_effects(player_ptr);
    rd_s16b(&player_ptr->chaos_patron);
    set_mutations(player_ptr);
    set_virtues(player_ptr);
}

static void rd_player_status(player_type *player_ptr)
{
    rd_base_status(player_ptr);
    strip_bytes(24);
    rd_s32b(&player_ptr->au);
    rd_experience(player_ptr);
    rd_skills(player_ptr);
    rd_race(player_ptr);
    set_imitation(player_ptr);
    rd_bounty_uniques(player_ptr);
    rd_arena(player_ptr);
    rd_dummy1();
    rd_hp(player_ptr);
    rd_mana(player_ptr);
    rd_s16b(&player_ptr->max_plv);
    rd_dungeons(player_ptr);
    strip_bytes(8);
    rd_s16b(&player_ptr->sc);
    rd_s16b(&player_ptr->concent);
    rd_bad_status(player_ptr);
    rd_energy(player_ptr);
    rd_status(player_ptr);
    rd_s16b(&player_ptr->hero);
    rd_s16b(&player_ptr->shero);
    rd_s16b(&player_ptr->shield);
    rd_s16b(&player_ptr->blessed);
    rd_s16b(&player_ptr->tim_invis);
    rd_s16b(&player_ptr->word_recall);
    rd_alter_reality(player_ptr);
    rd_s16b(&player_ptr->see_infra);
    rd_s16b(&player_ptr->tim_infra);
    rd_s16b(&player_ptr->oppose_fire);
    rd_s16b(&player_ptr->oppose_cold);
    rd_s16b(&player_ptr->oppose_acid);
    rd_s16b(&player_ptr->oppose_elec);
    rd_s16b(&player_ptr->oppose_pois);
    rd_tsuyoshi(player_ptr);
    rd_timed_effects(player_ptr);
    player_ptr->mutant_regenerate_mod = calc_mutant_regenerate_mod(player_ptr);
}

void rd_player_info(player_type *player_ptr)
{
    rd_player_status(player_ptr);
    rd_special_attack(player_ptr);
    rd_special_action(player_ptr);
    rd_special_defense(player_ptr);
    rd_byte(&player_ptr->knowledge);
    rd_autopick(player_ptr);
    rd_action(player_ptr);
}
