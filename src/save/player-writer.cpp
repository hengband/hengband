#include "save/player-writer.h"
#include "action/run-execution.h"
#include "action/travel-execution.h"
#include "floor/dungeon-feeling.h"
#include "game-option/birth-options.h"
#include "grid/grid.h"
#include "io/input-key-requester.h"
#include "market/arena-entry.h"
#include "object/tval-types.h"
#include "player-base/player-class.h"
#include "player/player-realm.h"
#include "player/player-skill.h"
#include "save/info-writer.h"
#include "save/player-class-specific-data-writer.h"
#include "save/save-util.h"
#include "system/angband-system.h"
#include "system/building-type-definition.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/dungeon/dungeon-record.h"
#include "system/floor/floor-info.h"
#include "system/floor/town-records.h"
#include "system/inner-game-data.h"
#include "system/player-type-definition.h"
#include "target/target.h"
#include "timed-effect/timed-effects.h"
#include "util/flag-group.h"
#include "window/main-window-util.h"
#include "world/world.h"
#include <variant>

/*!
 * @brief Write recall depths and dungeon visit history to the savefile.
 */
void wr_dungeons()
{
    const auto &records = DungeonRecords::get_instance();
    wr_byte(static_cast<uint8_t>(records.size()));
    for (const auto &[_, record] : records) {
        wr_s16b(static_cast<int16_t>(record->get_max_level()));
        wr_bool(record->has_visited());
    }
}

/*!
 * @brief セーブデータに領域情報を書き込む / Write player realms
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void wr_relams(PlayerType *player_ptr)
{
    PlayerRealm pr(player_ptr);
    if (PlayerClass(player_ptr).equals(PlayerClassType::ELEMENTALIST)) {
        wr_byte((byte)player_ptr->element_realm);
    } else {
        wr_byte((byte)pr.realm1().to_enum());
    }
    wr_byte((byte)pr.realm2().to_enum());
}

/*!
 * @brief 最後にターゲットしたものを書き込む
 * @details 種類 (0: なし、1: マス、2: モンスター) に続けて、マスなら座標、モンスターならフロアのモンスター番号を書く。
 * モンスターのターゲットは、今はターゲットできない (target_able() が偽) ときも、メモリ上と同じく番号を書く
 */
static void wr_last_target()
{
    const auto target = Target::get_last_target();
    if (const auto m_idx = target.get_m_idx()) {
        wr_byte(2);
        wr_s16b(*m_idx);
        return;
    }

    if (const auto pos = target.get_position()) {
        wr_byte(1);
        wr_s16b(static_cast<int16_t>(pos->y));
        wr_s16b(static_cast<int16_t>(pos->x));
        return;
    }

    wr_byte(0);
}

/*!
 * @brief 休憩・釣り・コマンドの繰り返しの続きと、画面に表示している範囲を書き込む
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details 休憩の残り、釣りの向き、繰り返し中のコマンドとその引数・残りの回数・方向を書く。
 * 方向がターゲットを指すときは、最後のターゲット (wr_last_target()) を指すものとして印だけを書く。
 * 表示している範囲は、再開したときに範囲が変わったとして disturb() しないよう書く
 */
static void wr_continuous_actions(PlayerType *player_ptr)
{
    wr_s32b(player_ptr->resting);
    wr_byte(static_cast<uint8_t>(player_ptr->fishing_dir));
    wr_s16b(command_cmd);
    wr_s16b(command_arg);
    wr_s16b(command_rep);
    wr_byte(static_cast<uint8_t>(command_dir.dir()));
    wr_bool(command_dir.is_targetting());
    wr_s16b(static_cast<int16_t>(panel_row_min));
    wr_s16b(static_cast<int16_t>(panel_col_min));
}

/*!
 * @brief 走りとトラベルの続きを書き込む
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details 走りは、残りの歩数・止まる位置・進み方と、モンスターの経路情報を前回探索した位置 (走っている間は
 * 探索を省くことがある) を書く。トラベルは目的地と状態・方向を書き、続けている途中なら、目的地を決めたときに
 * 作った行程を今のフロアの大きさの分だけ書く (歩くうちに分かった地形では作り直さないので、読み込みで作り直すと
 * 道筋が変わることがある)。
 */
static void wr_running_and_travel(PlayerType *player_ptr)
{
    wr_s16b(player_ptr->running);
    wr_s16b(static_cast<int16_t>(player_ptr->run_py));
    wr_s16b(static_cast<int16_t>(player_ptr->run_px));
    const auto course = get_running_course();
    wr_byte(static_cast<uint8_t>(course.current.dir()));
    wr_byte(static_cast<uint8_t>(course.prevdir.dir()));
    wr_bool(course.openarea);
    wr_bool(course.breakright);
    wr_bool(course.breakleft);
    wr_bool(ignore_avoid_run);
    const auto flow_center = get_flow_center();
    wr_s16b(static_cast<int16_t>(flow_center.y));
    wr_s16b(static_cast<int16_t>(flow_center.x));

    const auto &travel = Travel::get_instance();
    const auto &goal = travel.get_goal();
    wr_bool(goal.has_value());
    if (!goal) {
        return;
    }

    wr_s16b(static_cast<int16_t>(goal->y));
    wr_s16b(static_cast<int16_t>(goal->x));
    wr_byte(static_cast<uint8_t>(travel.get_state()));
    wr_byte(static_cast<uint8_t>(travel.get_direction().dir()));
    if (!travel.is_ongoing()) {
        return;
    }

    const auto &floor = *player_ptr->current_floor_ptr;
    wr_s16b(static_cast<int16_t>(floor.height));
    wr_s16b(static_cast<int16_t>(floor.width));
    for (auto y = 0; y < floor.height; y++) {
        for (auto x = 0; x < floor.width; x++) {
            wr_s16b(static_cast<int16_t>(travel.get_cost({ y, x })));
        }
    }
}

/*!
 * @brief セーブデータにプレイヤー情報を書き込む / Write some "player" info
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void wr_player(PlayerType *player_ptr)
{
    wr_string(player_ptr->name);
    wr_string(player_ptr->died_from);
    wr_string(player_ptr->last_message);

    save_quick_start();
    for (int i = 0; i < 4; i++) {
        wr_string(player_ptr->history[i]);
    }

    wr_byte((byte)player_ptr->prace);
    wr_byte((byte)player_ptr->pclass);
    wr_byte((byte)player_ptr->ppersonality);
    wr_byte((byte)player_ptr->psex);
    wr_relams(player_ptr);
    wr_byte(0);

    wr_byte((byte)player_ptr->hit_dice.sides);
    wr_u16b(player_ptr->expfact);

    wr_s16b(player_ptr->age);
    wr_s16b(player_ptr->ht);
    wr_s16b(player_ptr->wt);

    for (int i = 0; i < A_MAX; ++i) {
        wr_s16b(player_ptr->stat_max[i]);
    }

    for (int i = 0; i < A_MAX; ++i) {
        wr_s16b(player_ptr->stat_max_max[i]);
    }

    for (int i = 0; i < A_MAX; ++i) {
        wr_s16b(player_ptr->stat_cur[i]);
    }

    for (int i = 0; i < 12; ++i) {
        wr_s16b(0);
    }

    wr_u32b(player_ptr->au);
    wr_u32b(player_ptr->max_exp);
    wr_u32b(player_ptr->max_max_exp);
    wr_u32b(player_ptr->exp);
    wr_u32b(player_ptr->exp_frac);
    wr_s16b(player_ptr->lev);

    for (int i = 0; i < 64; i++) {
        wr_s16b(player_ptr->spell_exp[i]);
    }

    for (auto tval : TV_WEAPON_RANGE) {
        for (int j = 0; j < 64; j++) {
            wr_s16b(player_ptr->weapon_exp[tval][j]);
        }
    }

    for (auto i : PLAYER_SKILL_KIND_TYPE_RANGE) {
        wr_s16b(player_ptr->skill_exp[i]);
    }
    for (auto i = 0U; i < MAX_SKILLS - PLAYER_SKILL_KIND_TYPE_RANGE.size(); ++i) {
        // resreved skills
        wr_s16b(0);
    }

    std::visit(PlayerClassSpecificDataWriter(), player_ptr->class_specific_data);

    const auto &igd = InnerGameData::get_instance();
    wr_byte(static_cast<uint8_t>(igd.get_start_race()));
    wr_s32b(player_ptr->old_race1);
    wr_s32b(player_ptr->old_race2);
    wr_s16b(player_ptr->old_realm);

    const auto &world = AngbandWorld::get_instance();
    for (const auto &[monrace_id, is_achieved] : world.bounties) {
        wr_s16b(enum2i(monrace_id));
        wr_bool(is_achieved);
    }

    const auto &melee_arena = MeleeArena::get_instance();
    for (const auto &gladiator : melee_arena.get_gladiators()) {
        wr_s16b(enum2i(gladiator.monrace_id));
        wr_u32b(gladiator.odds);
    }

    wr_s16b(static_cast<int16_t>(world.get_town_index()));
    const auto &entries = ArenaEntryList::get_instance();
    wr_s16b(static_cast<int16_t>(entries.get_current_entry()));
    const auto defeated_entry = entries.get_defeated_entry();
    wr_s16b(static_cast<int16_t>(defeated_entry.value_or(-1)));
    wr_s16b(player_ptr->current_floor_ptr->inside_arena);
    wr_s16b(enum2i(player_ptr->current_floor_ptr->quest_number));
    wr_s16b(AngbandSystem::get_instance().is_phase_out());
    wr_byte(world.get_arena());
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

    wr_dungeons();

    wr_s16b(0);
    wr_s16b(0);
    wr_s16b(0);
    wr_s16b(0);
    wr_s16b(player_ptr->sc);

    auto effects = player_ptr->effects();
    wr_s16b(0); /* old "rest" */
    wr_s16b(effects->blindness().current());
    wr_s16b(effects->paralysis().current());
    wr_s16b(effects->confusion().current());
    wr_s16b(player_ptr->food);
    wr_s16b(0); /* old "food_digested" */
    wr_s16b(0); /* old "protection" */
    wr_s16b(player_ptr->energy_need);
    wr_s16b(player_ptr->enchant_energy_need);
    wr_s16b(effects->acceleration().current());
    wr_s16b(effects->deceleration().current());
    wr_s16b(effects->fear().current());
    wr_s16b(effects->cut().current());
    wr_s16b(effects->stun().current());
    wr_s16b(effects->poison().current());
    wr_s16b(effects->hallucination().current());
    wr_s16b(effects->protection().current());
    wr_s16b(player_ptr->invuln);
    wr_s16b(player_ptr->ult_res);
    wr_s16b(player_ptr->hero);
    wr_s16b(player_ptr->berserk);
    wr_s16b(player_ptr->shield);
    wr_s16b(player_ptr->blessed);
    wr_s16b(player_ptr->tim_invis);
    wr_s16b(player_ptr->word_recall);
    wr_s16b(static_cast<int16_t>(player_ptr->recall_dungeon));
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
    wr_s16b(player_ptr->tim_res_lite);
    wr_s16b(player_ptr->tim_res_dark);
    wr_s16b(player_ptr->tim_res_fear);
    wr_s16b(player_ptr->tim_res_time);
    wr_byte((byte)player_ptr->mimic_form);
    wr_s16b(player_ptr->tim_mimic);
    wr_s16b(player_ptr->tim_sh_fire);
    wr_s16b(player_ptr->tim_sh_holy);
    wr_s16b(player_ptr->tim_eyeeye);

    wr_s16b(player_ptr->tim_reflect);
    wr_s16b(player_ptr->multishadow);
    wr_s16b(player_ptr->dustrobe);
    wr_s16b(player_ptr->tim_emission);
    wr_s16b(player_ptr->tim_exorcism);
    wr_s16b(player_ptr->tim_imm_dark);

    wr_s16b(player_ptr->chaos_patron);
    wr_FlagGroup(player_ptr->muta, wr_byte);

    for (int i = 0; i < 8; i++) {
        wr_s16b(player_ptr->virtues[i]);
    }

    for (int i = 0; i < 8; i++) {
        wr_s16b(enum2i(player_ptr->vir_types[i]));
    }

    const auto &system = AngbandSystem::get_instance();
    wr_s16b(player_ptr->ele_attack);
    wr_u32b(player_ptr->special_attack);
    wr_s16b(player_ptr->ele_immune);
    wr_u32b(player_ptr->special_defense);
    wr_byte(player_ptr->knowledge);
    wr_bool(player_ptr->autopick_autoregister);
    wr_byte(0);
    wr_byte((byte)player_ptr->action);
    wr_byte(0);
    wr_bool(preserve_mode);
    wr_bool(system.is_awaiting_report_status());

    for (int i = 0; i < 12; i++) {
        wr_u32b(0L);
    }

    /* Ignore some flags */
    wr_u32b(0L);
    wr_u32b(0L);
    wr_u32b(0L);

    wr_u32b(system.get_seed_flavor());
    wr_u32b(system.get_seed_town());
    wr_u16b(system.is_panic_save_executed() ? 1 : 0);
    wr_u16b(world.total_winner);
    wr_u16b(igd.get_no_score());
    wr_bool(player_ptr->is_dead);
    const auto &df = DungeonFeeling::get_instance();
    wr_byte(static_cast<uint8_t>(df.get_feeling()));
    wr_s32b(player_ptr->current_floor_ptr->generated_turn);
    wr_s32b(df.get_turns());
    wr_s32b(world.game_turn);
    wr_s32b(world.dungeon_turn);
    wr_s32b(world.arena_start_turn);
    wr_s16b(enum2i(world.today_mon));
    wr_s16b(world.knows_daily_bounty ? 1 : 0); // 現在bool型だが、かつてモンスター種族IDを保存していた仕様に合わせる
    wr_s16b(player_ptr->riding);
    wr_s16b(player_ptr->floor_id);

    /* Save temporary preserved pets (obsolated) */
    wr_s16b(0);
    wr_u32b(world.play_time.elapsed_sec());
    wr_FlagGroup(TownRecords::get_instance().get_ids(), wr_byte);
    wr_u32b(player_ptr->count);
    wr_s16b(player_ptr->pet_t_m_idx);
    wr_s16b(player_ptr->riding_t_m_idx);
    wr_last_target();
    wr_continuous_actions(player_ptr);
    wr_running_and_travel(player_ptr);
}
