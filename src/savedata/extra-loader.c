/*!
 * todo 「その他」が雑多すぎて肥大化している。今後の課題として分割を検討する
 * @brief その他の情報を読み込む処理
 * @date 2020/07/05
 * @author Hourier
 */

#include "savedata/extra-loader.h"
#include "cmd-building/cmd-building.h"
#include "dungeon/dungeon.h"
#include "floor/floor.h"
#include "game-option/birth-options.h"
#include "market/arena.h"
#include "market/bounty.h"
#include "monster-race/monster-race.h"
#include "mutation/mutation.h"
#include "object-enchant/tr-types.h"
#include "player/attack-defense-types.h"
#include "player/avatar.h"
#include "player/patron.h"
#include "player/player-skill.h"
#include "player/special-defense-types.h"
#include "realm/realm-types.h"
#include "savedata/angband-version-comparer.h"
#include "savedata/birth-loader.h"
#include "savedata/load-util.h"
#include "savedata/load-v1-3-0.h"
#include "savedata/load-v1-7-0.h"
#include "savedata/load-zangband.h"
#include "savedata/monster-loader.h"
#include "savedata/player-info-loader.h"
#include "world/world.h"

/*!
 * @brief 腕力などの基本ステータス情報を読み込む
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
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
    if (z_older_than(10, 0, 1)) {
        for (int i = 0; i < MAX_MANE; i++) {
            creature_ptr->mane_spell[i] = -1;
            creature_ptr->mane_dam[i] = 0;
        }

        creature_ptr->mane_num = 0;
        return;
    }

    if (z_older_than(10, 2, 3)) {
        s16b tmp16s;
        const int OLD_MAX_MANE = 22;
        for (int i = 0; i < OLD_MAX_MANE; i++) {
            rd_s16b(&tmp16s);
            rd_s16b(&tmp16s);
        }

        for (int i = 0; i < MAX_MANE; i++) {
            creature_ptr->mane_spell[i] = -1;
            creature_ptr->mane_dam[i] = 0;
        }

        rd_s16b(&tmp16s);
        creature_ptr->mane_num = 0;
        return;
    }

    for (int i = 0; i < MAX_MANE; i++) {
        s16b tmp16s;
        rd_s16b(&tmp16s);
        creature_ptr->mane_spell[i] = (SPELL_IDX)tmp16s;
        rd_s16b(&tmp16s);
        creature_ptr->mane_dam[i] = (SPELL_IDX)tmp16s;
    }

    rd_s16b(&creature_ptr->mane_num);
}

static void set_mutations(player_type *creature_ptr)
{
    rd_u32b(&creature_ptr->muta1);
    rd_u32b(&creature_ptr->muta2);
    rd_u32b(&creature_ptr->muta3);
}

static void set_virtues(player_type *creature_ptr)
{
    for (int i = 0; i < 8; i++)
        rd_s16b(&creature_ptr->virtues[i]);

    for (int i = 0; i < 8; i++)
        rd_s16b(&creature_ptr->vir_types[i]);
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
    if (z_older_than(10, 4, 7))
        creature_ptr->magicdef = 0;
    else
        rd_s16b(&creature_ptr->magicdef);

    rd_s16b(&creature_ptr->tim_res_nether);
    if (z_older_than(10, 4, 11))
        set_zangband_mimic(creature_ptr);
    else {
        rd_s16b(&creature_ptr->tim_res_time);

        byte tmp8u;
        rd_byte(&tmp8u);
        creature_ptr->mimic_form = (IDX)tmp8u;
        rd_s16b(&creature_ptr->tim_mimic);
        rd_s16b(&creature_ptr->tim_sh_fire);
    }

    if (z_older_than(11, 0, 99))
        set_zangband_holy_aura(creature_ptr);
    else {
        rd_s16b(&creature_ptr->tim_sh_holy);
        rd_s16b(&creature_ptr->tim_eyeeye);
    }

    if (z_older_than(11, 0, 3))
        set_zangband_reflection(creature_ptr);
    else {
        rd_s16b(&creature_ptr->tim_reflect);
        rd_s16b(&creature_ptr->multishadow);
        rd_s16b(&creature_ptr->dustrobe);
    }
}

static void rd_hengband_dungeons(void)
{
    byte max = (byte)current_world_ptr->max_d_idx;
    rd_byte(&max);
    s16b tmp16s;
    for (int i = 0; i < max; i++) {
        rd_s16b(&tmp16s);
        max_dlv[i] = tmp16s;
        if (max_dlv[i] > d_info[i].maxdepth)
            max_dlv[i] = d_info[i].maxdepth;
    }
}

/*!
 * @brief 変愚蛮怒 v1.5.0より大きなバージョンにおいて、ダミーでモンスターを読み込む
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void rd_dummy_monsters(player_type *creature_ptr)
{
    if (h_older_than(1, 5, 0, 2))
        return;

    s16b tmp16s;
    rd_s16b(&tmp16s);
    for (int i = 0; i < tmp16s; i++) {
        monster_type dummy_mon;
        rd_monster(creature_ptr, &dummy_mon);
    }
}

static void set_gambling_monsters(void)
{
    const int max_gambling_monsters = 4;
    for (int i = 0; i < max_gambling_monsters; i++) {
        rd_s16b(&battle_mon[i]);
        if (z_older_than(10, 3, 4))
            set_zangband_gambling_monsters(i);
        else
            rd_u32b(&mon_odds[i]);
    }
}

static void rd_phase_out(player_type *creature_ptr)
{
    s16b tmp16s;
    rd_s16b(&tmp16s);
    creature_ptr->current_floor_ptr->inside_arena = (bool)tmp16s;
    rd_s16b(&creature_ptr->current_floor_ptr->inside_quest);
    if (z_older_than(10, 3, 5))
        creature_ptr->phase_out = FALSE;
    else {
        rd_s16b(&tmp16s);
        creature_ptr->phase_out = (bool)tmp16s;
    }
}

static void rd_arena(player_type *creature_ptr)
{
    if (z_older_than(10, 0, 3))
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

    s16b tmp16s;
    rd_s16b(&tmp16s);
    creature_ptr->oldpx = (POSITION)tmp16s;
    rd_s16b(&tmp16s);
    creature_ptr->oldpy = (POSITION)tmp16s;
    if (z_older_than(10, 3, 13) && !creature_ptr->current_floor_ptr->dun_level && !creature_ptr->current_floor_ptr->inside_arena) {
        creature_ptr->oldpy = 33;
        creature_ptr->oldpx = 131;
    }
}

/*!
 * @brief ダミーバイトを読み込む
 * @param なし
 * @return なし
 * @details もはや何に使われていたのか不明
 */
static void rd_dummy1(void)
{
    s16b tmp16s;
    rd_s16b(&tmp16s);
    for (int i = 0; i < tmp16s; i++) {
        s16b tmp16s2;
        rd_s16b(&tmp16s2);
    }
}

/*!
 * @brief プレーヤーの最大HP/現在HPを読み込む
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
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
 * @return なし
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

static void rd_dungeons(player_type *creature_ptr)
{
    if (z_older_than(10, 3, 8))
        rd_zangband_dungeon();
    else
        rd_hengband_dungeons();

    if (creature_ptr->max_plv < creature_ptr->lev)
        creature_ptr->max_plv = creature_ptr->lev;
}

/*!
 * todo 順番的に朦朧がない……
 * @brief プレーヤーのバッドステータス (と空腹)を読み込む
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
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
    if (z_older_than(11, 0, 13))
        creature_ptr->energy_need = 100 - creature_ptr->energy_need;

    if (h_older_than(2, 1, 2, 0))
        creature_ptr->enchant_energy_need = 0;
    else
        rd_s16b(&creature_ptr->enchant_energy_need);
}

/*!
 * todo 明らかに関数名がビッグワードだが他に思いつかなかった
 * @brief プレーヤーのグッド/バッドステータスを読み込む
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
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
    if (z_older_than(10, 0, 0))
        creature_ptr->ult_res = 0;
    else
        rd_s16b(&creature_ptr->ult_res);
}

/*!
 * @brief 現実変容処理の有無及びその残りターン数を読み込む
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void rd_alter_reality(player_type *creature_ptr)
{
    s16b tmp16s;
    if (z_older_than(10, 3, 8))
        creature_ptr->recall_dungeon = DUNGEON_ANGBAND;
    else {
        rd_s16b(&tmp16s);
        creature_ptr->recall_dungeon = (byte)tmp16s;
    }

    if (h_older_than(1, 5, 0, 0))
        creature_ptr->alter_reality = 0;
    else
        rd_s16b(&creature_ptr->alter_reality);
}

static void rd_tsuyoshi(player_type *creature_ptr)
{
    if (z_older_than(10, 0, 2))
        creature_ptr->tsuyoshi = 0;
    else
        rd_s16b(&creature_ptr->tsuyoshi);
}

/*!
 * @brief 各種時限効果を読み込む
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details ZAngbandとの互換性を保つ都合上、突然変異と徳の処理も追加している
 */
static void rd_timed_effects(player_type *creature_ptr)
{
    if ((current_world_ptr->z_major == 2) && (current_world_ptr->z_minor == 0) && (current_world_ptr->z_patch == 6)) {
        set_zangband_timed_effects(creature_ptr);
        return;
    }

    set_timed_effects(creature_ptr);
    rd_s16b(&creature_ptr->chaos_patron);
    set_mutations(creature_ptr);
    set_virtues(creature_ptr);
}

static void rd_special_attack(player_type *creature_ptr)
{
    if (z_older_than(10, 0, 9)) {
        set_zangband_special_attack(creature_ptr);
        return;
    }
    
    rd_s16b(&creature_ptr->ele_attack);
    rd_u32b(&creature_ptr->special_attack);
}

static void rd_action(player_type *creature_ptr)
{
    if (creature_ptr->special_attack & KAMAE_MASK) {
        creature_ptr->action = ACTION_KAMAE;
        return;
    }
    
    if (creature_ptr->special_attack & KATA_MASK)
        creature_ptr->action = ACTION_KATA;
}

static void rd_special_defense(player_type *creature_ptr)
{
    if (z_older_than(10, 0, 12)) {
        set_zangband_special_defense(creature_ptr);
        return;
    }
    
    rd_s16b(&creature_ptr->ele_immune);
    rd_u32b(&creature_ptr->special_defense);
}

static void set_undead_turn_limit(player_type *creature_ptr)
{
    switch (creature_ptr->start_race) {
    case RACE_VAMPIRE:
    case RACE_SKELETON:
    case RACE_ZOMBIE:
    case RACE_SPECTRE:
        current_world_ptr->game_turn_limit = TURNS_PER_TICK * TOWN_DAWN * MAX_DAYS + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
        break;
    default:
        current_world_ptr->game_turn_limit = TURNS_PER_TICK * TOWN_DAWN * (MAX_DAYS - 1) + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
        break;
    }
}

static void rd_world_info(player_type *creature_ptr)
{
    set_undead_turn_limit(creature_ptr);
    current_world_ptr->dungeon_turn_limit = TURNS_PER_TICK * TOWN_DAWN * (MAX_DAYS - 1) + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
    rd_s32b(&creature_ptr->current_floor_ptr->generated_turn);
    if (h_older_than(1, 7, 0, 4))
        creature_ptr->feeling_turn = creature_ptr->current_floor_ptr->generated_turn;
    else
        rd_s32b(&creature_ptr->feeling_turn);

    rd_s32b(&current_world_ptr->game_turn);
    if (z_older_than(10, 3, 12))
        current_world_ptr->dungeon_turn = current_world_ptr->game_turn;
    else
        rd_s32b(&current_world_ptr->dungeon_turn);

    if (z_older_than(11, 0, 13))
        set_zangband_game_turns(creature_ptr);

    if (z_older_than(10, 3, 13))
        current_world_ptr->arena_start_turn = current_world_ptr->game_turn;
    else
        rd_s32b(&current_world_ptr->arena_start_turn);

    if (z_older_than(10, 0, 3))
        determine_daily_bounty(creature_ptr, TRUE);
    else {
        rd_s16b(&today_mon);
        rd_s16b(&creature_ptr->today_mon);
    }
}

static void rd_visited_towns(player_type *creature_ptr)
{
    if (z_older_than(10, 3, 9)) {
        creature_ptr->visit = 1L;
        return;
    }

    if (z_older_than(10, 3, 10)) {
        set_zangband_visited_towns(creature_ptr);
        return;
    }

    s32b tmp32s;
    rd_s32b(&tmp32s);
    creature_ptr->visit = (BIT_FLAGS)tmp32s;
}

/*!
 * @brief その他の情報を読み込む / Read the "extra" information
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void rd_extra(player_type *creature_ptr)
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
    rd_special_attack(creature_ptr);
    rd_action(creature_ptr);
    rd_special_defense(creature_ptr);
    rd_byte(&creature_ptr->knowledge);
    byte tmp8u;
    rd_byte(&tmp8u);
    creature_ptr->autopick_autoregister = tmp8u != 0;

    rd_byte(&tmp8u);
    rd_byte(&tmp8u);
    creature_ptr->action = (ACTION_IDX)tmp8u;
    if (!z_older_than(10, 4, 3))
        set_zangband_action(creature_ptr);

    rd_byte((byte *)&preserve_mode);
    rd_byte((byte *)&creature_ptr->wait_report_score);

    for (int i = 0; i < 48; i++)
        rd_byte(&tmp8u);

    strip_bytes(12);
    rd_u32b(&current_world_ptr->seed_flavor);
    rd_u32b(&current_world_ptr->seed_town);

    rd_u16b(&creature_ptr->panic_save);
    rd_u16b(&current_world_ptr->total_winner);
    rd_u16b(&current_world_ptr->noscore);

    rd_byte(&tmp8u);
    creature_ptr->is_dead = tmp8u;

    rd_byte(&creature_ptr->feeling);
    rd_world_info(creature_ptr);
    if (z_older_than(10, 0, 7))
        creature_ptr->riding = 0;
    else
        rd_s16b(&creature_ptr->riding);

    if (h_older_than(1, 5, 0, 0))
        creature_ptr->floor_id = 0;
    else
        rd_s16b(&creature_ptr->floor_id);

    rd_dummy_monsters(creature_ptr);
    if (z_older_than(10, 1, 2))
        current_world_ptr->play_time = 0;
    else
        rd_u32b(&current_world_ptr->play_time);

    rd_visited_towns(creature_ptr);
    if (!z_older_than(11, 0, 5))
        rd_u32b(&creature_ptr->count);
}
