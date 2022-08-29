#include "load/player-info-loader.h"
#include "cmd-building/cmd-building.h"
#include "dungeon/dungeon.h"
#include "load/angband-version-comparer.h"
#include "load/birth-loader.h"
#include "load/dummy-loader.h"
#include "load/load-util.h"
#include "load/load-zangband.h"
#include "load/old/load-v1-7-0.h"
#include "load/player-attack-loader.h"
#include "load/player-class-specific-data-loader.h"
#include "load/savedata-old-flag-types.h"
#include "load/world-loader.h"
#include "market/arena.h"
#include "monster-race/race-ability-flags.h"
#include "mutation/mutation-calculator.h"
#include "player-base/player-class.h"
#include "player-info/mane-data-type.h"
#include "player-info/sniper-data-type.h"
#include "player/attack-defense-types.h"
#include "player/player-skill.h"
#include "spell-realm/spells-song.h"
#include "system/angband-exceptions.h"
#include "system/floor-type-definition.h"
#include "system/player-type-definition.h"
#include "timed-effect/player-acceleration.h"
#include "timed-effect/player-confusion.h"
#include "timed-effect/player-cut.h"
#include "timed-effect/player-deceleration.h"
#include "timed-effect/player-fear.h"
#include "timed-effect/player-hallucination.h"
#include "timed-effect/player-paralysis.h"
#include "timed-effect/player-poison.h"
#include "timed-effect/player-stun.h"
#include "timed-effect/timed-effects.h"
#include "world/world.h"

/*!
 * @brief セーブデータから領域情報を読み込む / Read player realms
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void rd_realms(PlayerType *player_ptr)
{
    if (PlayerClass(player_ptr).equals(PlayerClassType::ELEMENTALIST)) {
        player_ptr->element = rd_byte();
    } else {
        player_ptr->realm1 = rd_byte();
    }

    player_ptr->realm2 = rd_byte();
    if (player_ptr->realm2 == 255) {
        player_ptr->realm2 = 0;
    }
}

/*!
 * @brief セーブデータからプレイヤー基本情報を読み込む / Read player's basic info
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void rd_base_info(PlayerType *player_ptr)
{
    rd_string(player_ptr->name, sizeof(player_ptr->name));
    rd_string(player_ptr->died_from, 1024);
    if (!h_older_than(1, 7, 0, 1)) {
        char buf[1024];
        rd_string(buf, sizeof buf);
        if (buf[0]) {
            player_ptr->last_message = string_make(buf);
        }
    }

    load_quick_start();
    const int max_history_lines = 4;
    for (int i = 0; i < max_history_lines; i++) {
        rd_string(player_ptr->history[i], sizeof(player_ptr->history[i]));
    }

    player_ptr->prace = i2enum<PlayerRaceType>(rd_byte());
    player_ptr->pclass = i2enum<PlayerClassType>(rd_byte());
    player_ptr->ppersonality = i2enum<player_personality_type>(rd_byte());
    player_ptr->psex = i2enum<player_sex>(rd_byte());

    rd_realms(player_ptr);

    strip_bytes(1);
    if (h_older_than(0, 4, 4)) {
        set_zangband_realm(player_ptr);
    }

    player_ptr->hitdie = rd_byte();
    player_ptr->expfact = rd_u16b();

    player_ptr->age = rd_s16b();
    player_ptr->ht = rd_s16b();
    player_ptr->wt = rd_s16b();
}

void rd_experience(PlayerType *player_ptr)
{
    player_ptr->max_exp = rd_s32b();
    if (h_older_than(1, 5, 4, 1)) {
        player_ptr->max_max_exp = player_ptr->max_exp;
    } else {
        player_ptr->max_max_exp = rd_s32b();
    }

    player_ptr->exp = rd_s32b();
    if (h_older_than(1, 7, 0, 3)) {
        set_exp_frac_old(player_ptr);
    } else {
        player_ptr->exp_frac = rd_u32b();
    }

    player_ptr->lev = rd_s16b();
    for (int i = 0; i < 64; i++) {
        player_ptr->spell_exp[i] = rd_s16b();
    }

    if (PlayerClass(player_ptr).equals(PlayerClassType::SORCERER) && h_older_than(0, 4, 2)) {
        for (int i = 0; i < 64; i++) {
            player_ptr->spell_exp[i] = PlayerSkill::spell_exp_at(PlayerSkillRank::MASTER);
        }
    }

    const int max_weapon_exp_size = h_older_than(0, 3, 6) ? 60 : 64;
    for (auto tval : TV_WEAPON_RANGE) {
        for (int j = 0; j < max_weapon_exp_size; j++) {
            player_ptr->weapon_exp[tval][j] = rd_s16b();
        }
    }

    for (auto i : PLAYER_SKILL_KIND_TYPE_RANGE) {
        player_ptr->skill_exp[i] = rd_s16b();
    }

    // resreved skills
    strip_bytes(sizeof(int16_t) * (MAX_SKILLS - PLAYER_SKILL_KIND_TYPE_RANGE.size()));
}

void rd_skills(PlayerType *player_ptr)
{
    if (h_older_than(0, 4, 1)) {
        set_zangband_skill(player_ptr);
    }

    PlayerClass(player_ptr).init_specific_data();
    std::visit(PlayerClassSpecificDataLoader(), player_ptr->class_specific_data);

    if (music_singing_any(player_ptr)) {
        player_ptr->action = ACTION_SING;
    }
}

static void set_race(PlayerType *player_ptr)
{
    player_ptr->start_race = i2enum<PlayerRaceType>(rd_byte());
    player_ptr->old_race1 = rd_u32b();
    player_ptr->old_race2 = rd_u32b();
    player_ptr->old_realm = rd_s16b();
}

void rd_race(PlayerType *player_ptr)
{
    if (h_older_than(1, 0, 7)) {
        set_zangband_race(player_ptr);
        return;
    }

    set_race(player_ptr);
}

void rd_bounty_uniques(PlayerType *player_ptr)
{
    if (h_older_than(0, 0, 3)) {
        set_zangband_bounty_uniques(player_ptr);
        return;
    }

    for (auto &[r_idx, is_achieved] : w_ptr->bounties) {
        auto r_idx_num = rd_s16b();

        if (loading_savefile_version_is_older_than(16)) {
            constexpr auto old_achieved_flag = 10000; // かつて賞金首達成フラグとしてモンスター種族番号を10000増やしていた
            is_achieved = false;
            if (r_idx_num >= old_achieved_flag) {
                r_idx_num -= old_achieved_flag;
                is_achieved = true;
            }
        } else {
            is_achieved = rd_bool();
        }

        r_idx = i2enum<MonsterRaceId>(r_idx_num);
    }
}

/*!
 * @brief 腕力などの基本ステータス情報を読み込む
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void rd_base_status(PlayerType *player_ptr)
{
    for (int i = 0; i < A_MAX; i++) {
        player_ptr->stat_max[i] = rd_s16b();
    }

    for (int i = 0; i < A_MAX; i++) {
        player_ptr->stat_max_max[i] = rd_s16b();
    }

    for (int i = 0; i < A_MAX; i++) {
        player_ptr->stat_cur[i] = rd_s16b();
    }
}

static void set_imitation(PlayerType *player_ptr)
{
    if (h_older_than(0, 0, 1)) {
        return;
    }

    if (h_older_than(0, 2, 3)) {
        const int OLD_MAX_MANE = 22;
        for (int i = 0; i < OLD_MAX_MANE; i++) {
            strip_bytes(2);
            strip_bytes(2);
        }

        strip_bytes(2);
        return;
    }

    if (loading_savefile_version_is_older_than(9)) {
        auto mane_data = PlayerClass(player_ptr).get_specific_data<mane_data_type>();
        if (!mane_data) {
            // ものまね師でない場合に読み捨てるためのダミーデータ領域
            mane_data = std::make_shared<mane_data_type>();
        }

        for (int i = 0; i < MAX_MANE; ++i) {
            auto spell = rd_s16b();
            auto damage = rd_s16b();
            mane_data->mane_list.push_back({ i2enum<MonsterAbilityType>(spell), damage });
        }
        auto count = rd_s16b();
        mane_data->mane_list.resize(count);
    }
}

static void rd_phase_out(PlayerType *player_ptr)
{
    player_ptr->current_floor_ptr->inside_arena = rd_s16b() != 0;
    const auto quest_number = rd_s16b();
    if (loading_savefile_version_is_older_than(15)) {
        if (quest_number == enum2i(OldQuestId15::CITY_SEA)) {
            const std::string msg(_("海底都市クエストにいるデータはサポート外です。",
                "The save data in the quest of The City beneath the Sea is unsupported."));
            throw(SaveDataNotSupportedException(msg));
        }
    }
    player_ptr->current_floor_ptr->quest_number = i2enum<QuestId>(quest_number);
    if (h_older_than(0, 3, 5)) {
        player_ptr->phase_out = false;
    } else {
        player_ptr->phase_out = rd_s16b() != 0;
    }
}

static void rd_arena(PlayerType *player_ptr)
{
    if (h_older_than(0, 0, 3)) {
        update_gambling_monsters(player_ptr);
    } else {
        set_gambling_monsters();
    }

    player_ptr->town_num = rd_s16b();
    player_ptr->arena_number = rd_s16b();
    if (h_older_than(1, 5, 0, 1)) {
        if (player_ptr->arena_number >= 99) {
            player_ptr->arena_number = ARENA_DEFEATED_OLD_VER;
        }
    }

    rd_phase_out(player_ptr);
    player_ptr->exit_bldg = rd_byte();
    strip_bytes(1);

    player_ptr->oldpx = rd_s16b();
    player_ptr->oldpy = rd_s16b();
    if (h_older_than(0, 3, 13) && !is_in_dungeon(player_ptr) && !player_ptr->current_floor_ptr->inside_arena) {
        player_ptr->oldpy = 33;
        player_ptr->oldpx = 131;
    }
}

/*!
 * @brief プレイヤーの最大HP/現在HPを読み込む
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void rd_hp(PlayerType *player_ptr)
{
    if (h_older_than(1, 7, 0, 3)) {
        set_hp_old(player_ptr);
        return;
    }

    player_ptr->mhp = rd_s32b();
    player_ptr->chp = rd_s32b();
    player_ptr->chp_frac = rd_u32b();
}

/*!
 * @brief プレイヤーの最大MP/現在MPを読み込む
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void rd_mana(PlayerType *player_ptr)
{
    if (h_older_than(1, 7, 0, 3)) {
        set_mana_old(player_ptr);
        return;
    }

    player_ptr->msp = rd_s32b();
    player_ptr->csp = rd_s32b();
    player_ptr->csp_frac = rd_u32b();
}

/*!
 * @brief プレイヤーのバッドステータス (と空腹)を読み込む
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void rd_bad_status(PlayerType *player_ptr)
{
    auto effects = player_ptr->effects();
    strip_bytes(2); /* Old "rest" */
    player_ptr->blind = rd_s16b();
    effects->paralysis()->set(rd_s16b());
    effects->confusion()->set(rd_s16b());
    player_ptr->food = rd_s16b();
    strip_bytes(4); /* Old "food_digested" / "protection" */
}

static void rd_energy(PlayerType *player_ptr)
{
    player_ptr->energy_need = rd_s16b();
    if (h_older_than(1, 0, 13)) {
        player_ptr->energy_need = 100 - player_ptr->energy_need;
    }

    if (h_older_than(2, 1, 2, 0)) {
        player_ptr->enchant_energy_need = 0;
    } else {
        player_ptr->enchant_energy_need = rd_s16b();
    }
}

/*!
 * @brief プレイヤーのグッド/バッドステータスを読み込む
 * @param player_ptr プレイヤーへの参照ポインタ
 * @todo 明らかに関数名がビッグワードだが他に思いつかなかった
 */
static void rd_status(PlayerType *player_ptr)
{
    const auto effects = player_ptr->effects();
    effects->acceleration()->set(rd_s16b());
    effects->deceleration()->set(rd_s16b());
    effects->fear()->set(rd_s16b());
    effects->cut()->set(rd_s16b());
    effects->stun()->set(rd_s16b());
    effects->poison()->set(rd_s16b());
    effects->hallucination()->set(rd_s16b());
    player_ptr->protevil = rd_s16b();
    player_ptr->invuln = rd_s16b();
    if (h_older_than(0, 0, 0)) {
        player_ptr->ult_res = 0;
    } else {
        player_ptr->ult_res = rd_s16b();
    }
}

static void rd_tsuyoshi(PlayerType *player_ptr)
{
    if (h_older_than(0, 0, 2)) {
        player_ptr->tsuyoshi = 0;
    } else {
        player_ptr->tsuyoshi = rd_s16b();
    }
}

static void set_timed_effects(PlayerType *player_ptr)
{
    player_ptr->tim_esp = rd_s16b();
    player_ptr->wraith_form = rd_s16b();
    player_ptr->resist_magic = rd_s16b();
    player_ptr->tim_regen = rd_s16b();
    player_ptr->tim_pass_wall = rd_s16b();
    player_ptr->tim_stealth = rd_s16b();
    player_ptr->tim_levitation = rd_s16b();
    player_ptr->tim_sh_touki = rd_s16b();
    player_ptr->lightspeed = rd_s16b();
    player_ptr->tsubureru = rd_s16b();
    if (h_older_than(0, 4, 7)) {
        player_ptr->magicdef = 0;
    } else {
        player_ptr->magicdef = rd_s16b();
    }

    player_ptr->tim_res_nether = rd_s16b();
    if (h_older_than(0, 4, 11)) {
        set_zangband_mimic(player_ptr);
    } else {
        player_ptr->tim_res_time = rd_s16b();

        player_ptr->mimic_form = i2enum<MimicKindType>(rd_byte());
        player_ptr->tim_mimic = rd_s16b();
        player_ptr->tim_sh_fire = rd_s16b();
    }

    if (h_older_than(1, 0, 99)) {
        set_zangband_holy_aura(player_ptr);
    } else {
        player_ptr->tim_sh_holy = rd_s16b();
        player_ptr->tim_eyeeye = rd_s16b();
    }

    if (h_older_than(1, 0, 3)) {
        set_zangband_reflection(player_ptr);
    } else {
        player_ptr->tim_reflect = rd_s16b();
        player_ptr->multishadow = rd_s16b();
        player_ptr->dustrobe = rd_s16b();
    }
}

static void set_mutations(PlayerType *player_ptr)
{
    if (loading_savefile_version_is_older_than(2)) {
        for (int i = 0; i < 3; i++) {
            auto tmp32u = rd_u32b();
            migrate_bitflag_to_flaggroup(player_ptr->muta, tmp32u, i * 32);
        }
    } else {
        rd_FlagGroup(player_ptr->muta, rd_byte);
    }
}

static void set_virtues(PlayerType *player_ptr)
{
    for (int i = 0; i < 8; i++) {
        player_ptr->virtues[i] = rd_s16b();
    }

    for (int i = 0; i < 8; i++) {
        player_ptr->vir_types[i] = rd_s16b();
    }
}

/*!
 * @brief 各種時限効果を読み込む
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void rd_timed_effects(PlayerType *player_ptr)
{
    set_timed_effects(player_ptr);
    player_ptr->chaos_patron = rd_s16b();
    set_mutations(player_ptr);
    set_virtues(player_ptr);
}

static void rd_player_status(PlayerType *player_ptr)
{
    rd_base_status(player_ptr);
    strip_bytes(24);
    player_ptr->au = rd_s32b();
    rd_experience(player_ptr);
    rd_skills(player_ptr);
    rd_race(player_ptr);
    set_imitation(player_ptr);
    rd_bounty_uniques(player_ptr);
    rd_arena(player_ptr);
    rd_dummy1();
    rd_hp(player_ptr);
    rd_mana(player_ptr);
    player_ptr->max_plv = rd_s16b();
    rd_dungeons(player_ptr);
    strip_bytes(8);
    player_ptr->sc = rd_s16b();
    if (loading_savefile_version_is_older_than(9)) {
        auto sniper_data = PlayerClass(player_ptr).get_specific_data<sniper_data_type>();
        if (sniper_data) {
            sniper_data->concent = rd_s16b();
        } else {
            // 職業がスナイパーではないので読み捨てる
            strip_bytes(2);
        }
    }
    rd_bad_status(player_ptr);
    rd_energy(player_ptr);
    rd_status(player_ptr);
    player_ptr->hero = rd_s16b();
    player_ptr->shero = rd_s16b();
    player_ptr->shield = rd_s16b();
    player_ptr->blessed = rd_s16b();
    player_ptr->tim_invis = rd_s16b();
    player_ptr->word_recall = rd_s16b();
    rd_alter_reality(player_ptr);
    player_ptr->see_infra = rd_s16b();
    player_ptr->tim_infra = rd_s16b();
    player_ptr->oppose_fire = rd_s16b();
    player_ptr->oppose_cold = rd_s16b();
    player_ptr->oppose_acid = rd_s16b();
    player_ptr->oppose_elec = rd_s16b();
    player_ptr->oppose_pois = rd_s16b();
    rd_tsuyoshi(player_ptr);
    rd_timed_effects(player_ptr);
    player_ptr->mutant_regenerate_mod = calc_mutant_regenerate_mod(player_ptr);
}

void rd_player_info(PlayerType *player_ptr)
{
    rd_player_status(player_ptr);
    rd_special_attack(player_ptr);
    rd_special_action(player_ptr);
    rd_special_defense(player_ptr);
    player_ptr->knowledge = rd_byte();
    rd_autopick(player_ptr);
    rd_action(player_ptr);
}
