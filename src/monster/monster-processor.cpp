/*!
 * @brief モンスターの特殊技能とターン経過処理 (移動等)/ Monster spells and movement for passaging a turn
 * @date 2014/01/17
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2014 Deskull rearranged comment for Doxygen.\n
 * @details
 * This file has several additions to it by Keldon Jones (keldon@umr.edu)
 * to improve the general quality of the AI (version 0.1.1).
 */

#include "monster/monster-processor.h"
#include "avatar/avatar.h"
#include "cmd-io/cmd-dump.h"
#include "core/disturbance.h"
#include "core/speed-table.h"
#include "floor/geometry.h"
#include "game-option/birth-options.h"
#include "game-option/play-record-options.h"
#include "grid/grid.h"
#include "io/write-diary.h"
#include "melee/melee-postprocess.h"
#include "melee/melee-spell.h"
#include "monster-floor/monster-direction.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/monster-move.h"
#include "monster-floor/monster-remover.h"
#include "monster-floor/monster-runaway.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster-floor/quantum-effect.h"
#include "monster-race/race-brightness-mask.h"
#include "monster-race/race-flags-resistance.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-processor-util.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "monster/monster-util.h"
#include "mspell/mspell-attack.h"
#include "mspell/mspell-judgement.h"
#include "mspell/mspell-util.h"
#include "object-enchant/trc-types.h"
#include "pet/pet-fall-off.h"
#include "player-base/player-class.h"
#include "player-info/ninja-data-type.h"
#include "player/player-move.h"
#include "player/player-skill.h"
#include "player/player-status-flags.h"
#include "player/special-defense-types.h"
#include "spell-kind/spells-teleport.h"
#include "spell-realm/spells-hex.h"
#include "spell/summon-types.h"
#include "system/angband-system.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/projection-path-calculator.h"
#include "tracking/lore-tracker.h"
#include "view/display-messages.h"
#include "world/world.h"

void decide_drop_from_monster(PlayerType *player_ptr, MONSTER_IDX m_idx, bool is_riding_mon);
bool process_stealth(PlayerType *player_ptr, MONSTER_IDX m_idx);
bool vanish_summoned_children(PlayerType *player_ptr, MONSTER_IDX m_idx, bool see_m);
bool awake_monster(PlayerType *player_ptr, MONSTER_IDX m_idx);
void process_angar(PlayerType *player_ptr, MONSTER_IDX m_idx, bool see_m);
bool explode_grenade(PlayerType *player_ptr, MONSTER_IDX m_idx);
bool decide_monster_multiplication(PlayerType *player_ptr, MONSTER_IDX m_idx, POSITION oy, POSITION ox);
void process_special(PlayerType *player_ptr, MONSTER_IDX m_idx);
bool cast_spell(PlayerType *player_ptr, MONSTER_IDX m_idx, bool aware);

bool process_monster_fear(PlayerType *player_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx);

void sweep_monster_process(PlayerType *player_ptr);
bool decide_process_continue(PlayerType *player_ptr, MonsterEntity &monster);
bool process_stalking(PlayerType *player_ptr, MONSTER_IDX m_idx);

constexpr auto STALKER_CHANCE_DENOMINATOR = 32; //!< モンスターが背後に忍び寄る確率分母
constexpr auto STALKER_DISTANCE_THRESHOLD = 20; //!< モンスターが背後に忍び寄る距離の閾値

/*!
 * @brief モンスター単体の1ターン行動処理メインルーチン /
 * Process a monster
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 行動モンスターの参照ID
 * @details
 * The monster is known to be within 100 grids of the player\n
 *\n
 * In several cases, we directly update the monster lore\n
 *\n
 * Note that a monster is only allowed to "reproduce" if there\n
 * are a limited number of "reproducing" monsters on the current\n
 * level.  This should prevent the level from being "swamped" by\n
 * reproducing monsters.  It also allows a large mass of mice to\n
 * prevent a louse from multiplying, but this is a small price to\n
 * pay for a simple ENERGY_MULTIPLICATION method.\n
 *\n
 * XXX Monster fear is slightly odd, in particular, monsters will\n
 * fixate on opening a door even if they cannot open it.  Actually,\n
 * the same thing happens to normal monsters when they hit a door\n
 *\n
 * In addition, monsters which *cannot* open or bash\n
 * down a door will still stand there trying to open it...\n
 *\n
 * XXX Technically, need to check for monster in the way\n
 * combined with that monster being in a wall (or door?)\n
 *\n
 * A "direction" of "5" means "pick a random direction".\n
 */
void process_monster(PlayerType *player_ptr, MONSTER_IDX m_idx)
{
    auto &monster = player_ptr->current_floor_ptr->m_list[m_idx];
    turn_flags tmp_flags;
    turn_flags *turn_flags_ptr = init_turn_flags(monster.is_riding(), &tmp_flags);
    turn_flags_ptr->see_m = is_seen(player_ptr, monster);

    decide_drop_from_monster(player_ptr, m_idx, turn_flags_ptr->is_riding_mon);
    if (monster.mflag2.has(MonsterConstantFlagType::CHAMELEON) && one_in_(13) && !monster.is_asleep()) {
        const auto &floor = *player_ptr->current_floor_ptr;
        const auto old_m_name = monster_desc(player_ptr, monster, 0);
        const auto &monrace = monster.get_monrace();
        const auto m_pos = monster.get_position();
        const auto &grid = floor.get_grid(m_pos);
        choose_chameleon_polymorph(player_ptr, m_idx, grid.get_terrain_id());
        update_monster(player_ptr, m_idx, false);
        lite_spot(player_ptr, m_pos);
        const auto &new_monrace = monster.get_monrace();

        if (new_monrace.brightness_flags.has_any_of(ld_mask) || monrace.brightness_flags.has_any_of(ld_mask)) {
            RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::MONSTER_LITE);
        }

        if (turn_flags_ptr->is_riding_mon) {
            msg_format(_("突然%sが変身した。", "Suddenly, %s transforms!"), old_m_name.data());
            if (new_monrace.misc_flags.has_not(MonsterMiscType::RIDING)) {
                if (process_fall_off_horse(player_ptr, 0, true)) {
                    const auto m_name = monster_desc(player_ptr, monster, 0);
                    msg_print(_("地面に落とされた。", format("You have fallen from %s.", m_name.data())));
                }
            }
        }

        monster.set_individual_speed(floor.inside_arena);

        const auto old_maxhp = monster.max_maxhp;
        if (new_monrace.misc_flags.has(MonsterMiscType::FORCE_MAXHP)) {
            monster.max_maxhp = new_monrace.hit_dice.maxroll();
        } else {
            monster.max_maxhp = new_monrace.hit_dice.roll();
        }

        if (ironman_nightmare) {
            const auto hp = monster.max_maxhp * 2;
            monster.max_maxhp = std::min(MONSTER_MAXHP, hp);
        }

        monster.maxhp = monster.maxhp * monster.max_maxhp / old_maxhp;
        if (monster.maxhp < 1) {
            monster.maxhp = 1;
        }
        monster.hp = monster.hp * monster.max_maxhp / old_maxhp;
        monster.dealt_damage = 0;
    }

    auto &monrace = monster.get_monrace();

    mark_monsters_present(player_ptr);

    turn_flags_ptr->aware = process_stealth(player_ptr, m_idx);
    if (vanish_summoned_children(player_ptr, m_idx, turn_flags_ptr->see_m)) {
        return;
    }

    if (process_quantum_effect(player_ptr, m_idx, turn_flags_ptr->see_m)) {
        return;
    }

    if (explode_grenade(player_ptr, m_idx)) {
        return;
    }

    if (runaway_monster(player_ptr, turn_flags_ptr, m_idx)) {
        return;
    }

    if (!awake_monster(player_ptr, m_idx)) {
        return;
    }

    if (monster.is_stunned() && one_in_(2)) {
        return;
    }

    if (process_stalking(player_ptr, m_idx)) {
        return;
    }

    if (turn_flags_ptr->is_riding_mon) {
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
    }

    process_angar(player_ptr, m_idx, turn_flags_ptr->see_m);

    POSITION oy = monster.fy;
    POSITION ox = monster.fx;
    if (decide_monster_multiplication(player_ptr, m_idx, oy, ox)) {
        return;
    }

    process_special(player_ptr, m_idx);
    process_sound(player_ptr, m_idx);
    process_speak(player_ptr, m_idx, oy, ox, turn_flags_ptr->aware);
    if (cast_spell(player_ptr, m_idx, turn_flags_ptr->aware)) {
        return;
    }

    const auto mmdl = decide_monster_movement_direction(player_ptr, m_idx, turn_flags_ptr->aware);
    if (!mmdl) {
        return;
    }

    int count = 0;
    if (!process_monster_movement(player_ptr, turn_flags_ptr, *mmdl, { oy, ox }, &count)) {
        return;
    }

    /*
     *  Forward movements failed, but now received LOS attack!
     *  Try to flow by smell.
     */
    if (player_ptr->no_flowed && count > 2 && monster.target_y) {
        monster.mflag2.reset(MonsterConstantFlagType::NOFLOW);
    }

    if (!turn_flags_ptr->do_turn && !turn_flags_ptr->do_move && !monster.is_fearful() && !turn_flags_ptr->is_riding_mon && turn_flags_ptr->aware) {
        if (monrace.freq_spell && randint1(100) <= monrace.freq_spell) {
            if (make_attack_spell(player_ptr, m_idx)) {
                return;
            }
        }
    }

    update_map_flags(turn_flags_ptr);
    update_lite_flags(turn_flags_ptr, monrace);
    update_monster_race_flags(player_ptr, turn_flags_ptr, monster);

    if (!process_monster_fear(player_ptr, turn_flags_ptr, m_idx)) {
        return;
    }

    if (monster.ml) {
        chg_virtue(player_ptr, Virtue::COMPASSION, -1);
    }
}

/*!
 * @brief 超隠密処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @return モンスターがプレイヤーに気付いているならばTRUE、超隠密状態ならばFALSE
 */
bool process_stealth(PlayerType *player_ptr, MONSTER_IDX m_idx)
{
    auto ninja_data = PlayerClass(player_ptr).get_specific_data<ninja_data_type>();
    if (!ninja_data || !ninja_data->s_stealth) {
        return true;
    }

    const auto &monster = player_ptr->current_floor_ptr->m_list[m_idx];
    const auto &monrace = monster.get_monrace();
    int tmp = player_ptr->lev * 6 + (player_ptr->skill_stl + 10) * 4;
    if (player_ptr->monlite) {
        tmp /= 3;
    }

    if (has_aggravate(player_ptr)) {
        tmp /= 2;
    }

    if (monrace.level > (player_ptr->lev * player_ptr->lev / 20 + 10)) {
        tmp /= 3;
    }

    return randint0(tmp) <= (monrace.level + 20);
}

/*!
 * @brief 死亡したモンスターが乗馬中のモンスターだった場合に落馬処理を行う
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @param is_riding_mon 騎乗中であればTRUE
 */
void decide_drop_from_monster(PlayerType *player_ptr, MONSTER_IDX m_idx, bool is_riding_mon)
{
    const auto &monster = player_ptr->current_floor_ptr->m_list[m_idx];
    const auto &monrace = monster.get_monrace();
    if (!is_riding_mon || monrace.misc_flags.has(MonsterMiscType::RIDING)) {
        return;
    }

    if (process_fall_off_horse(player_ptr, 0, true)) {
#ifdef JP
        msg_print("地面に落とされた。");
#else
        const auto m_name = monster_desc(player_ptr, player_ptr->current_floor_ptr->m_list[player_ptr->riding], 0);
        msg_format("You have fallen from %s.", m_name.data());
#endif
    }
}

/*!
 * @brief 召喚の親元が消滅した時、子供も消滅させる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @param see_m モンスターが視界内にいたらTRUE
 * @return 召喚モンスターが消滅したらTRUE
 */
bool vanish_summoned_children(PlayerType *player_ptr, MONSTER_IDX m_idx, bool see_m)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &monster = floor.m_list[m_idx];

    if (!monster.has_parent()) {
        return false;
    }

    // parent_m_idxが自分自身を指している場合は召喚主は消滅している
    if (monster.parent_m_idx != m_idx && floor.m_list[monster.parent_m_idx].is_valid()) {
        return false;
    }

    if (see_m) {
        const auto m_name = monster_desc(player_ptr, monster, 0);
        msg_format(_("%sは消え去った！", "%s^ disappears!"), m_name.data());
    }

    if (record_named_pet && monster.is_named_pet()) {
        const auto m_name = monster_desc(player_ptr, monster, MD_INDEF_VISIBLE);
        exe_write_diary(floor, DiaryKind::NAMED_PET, RECORD_NAMED_PET_LOSE_PARENT, m_name);
    }

    delete_monster_idx(player_ptr, m_idx);
    return true;
}

/*!
 * @brief 寝ているモンスターの起床を判定する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @return 寝たままならFALSE、起きているor起きたらTRUE
 */
bool awake_monster(PlayerType *player_ptr, MONSTER_IDX m_idx)
{
    const auto &monster = player_ptr->current_floor_ptr->m_list[m_idx];
    auto &monrace = monster.get_monrace();
    if (!monster.is_asleep()) {
        return true;
    }

    if (!has_aggravate(player_ptr)) {
        return false;
    }

    (void)set_monster_csleep(player_ptr, m_idx, 0);
    if (monster.ml) {
        const auto m_name = monster_desc(player_ptr, monster, 0);
        msg_format(_("%s^が目を覚ました。", "%s^ wakes up."), m_name.data());
    }

    if (is_original_ap_and_seen(player_ptr, monster) && (monrace.r_wake < MAX_UCHAR)) {
        monrace.r_wake++;
    }

    return true;
}

/*!
 * @brief モンスターの怒り状態を判定する (怒っていたら敵に回す)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @param see_m モンスターが視界内にいたらTRUE
 */
void process_angar(PlayerType *player_ptr, MONSTER_IDX m_idx, bool see_m)
{
    auto &monster = player_ptr->current_floor_ptr->m_list[m_idx];
    const auto &monrace = monster.get_monrace();
    auto gets_angry = monster.is_friendly() && has_aggravate(player_ptr);
    const auto should_aggravate = monster.is_pet();
    auto has_hostile = monrace.kind_flags.has(MonsterKindType::UNIQUE) || (monrace.population_flags.has(MonsterPopulationType::NAZGUL));
    has_hostile &= monster_has_hostile_to_player(player_ptr, 10, -10, monrace);
    const auto has_resist_all = monrace.resistance_flags.has(MonsterResistanceType::RESIST_ALL);
    if (should_aggravate && (has_hostile || has_resist_all)) {
        gets_angry = true;
    }

    if (AngbandSystem::get_instance().is_phase_out() || !gets_angry) {
        return;
    }

    const auto m_name = monster_desc(player_ptr, monster, monster.is_pet() ? MD_ASSUME_VISIBLE : 0);

    /* When riding a hostile alignment pet */
    if (monster.is_riding()) {
        if (abs(player_ptr->alignment / 10) < randint0(player_ptr->skill_exp[PlayerSkillKindType::RIDING])) {
            return;
        }

        msg_format(_("%s^が突然暴れだした！", "%s^ suddenly begins unruly!"), m_name.data());
        if (!process_fall_off_horse(player_ptr, 1, true)) {
            return;
        }

        msg_format(_("あなたは振り落とされた。", "You have fallen."));
    }

    if (monster.is_pet() || see_m) {
        msg_format(_("%s^は突然敵にまわった！", "%s^ suddenly becomes hostile!"), m_name.data());
    }

    monster.set_hostile();
}

/*!
 * @brief 手榴弾の爆発処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @return 爆死したらTRUE
 */
bool explode_grenade(PlayerType *player_ptr, MONSTER_IDX m_idx)
{
    const auto &monster = player_ptr->current_floor_ptr->m_list[m_idx];
    if (monster.r_idx != MonraceId::GRENADE) {
        return false;
    }

    bool fear, dead;
    mon_take_hit_mon(player_ptr, m_idx, 1, &dead, &fear, _("は爆発して粉々になった。", " explodes into tiny shreds."), m_idx);
    return dead;
}

/*!
 * @brief モンスター依存の特別な行動を取らせる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターID
 */
void process_special(PlayerType *player_ptr, MONSTER_IDX m_idx)
{
    const auto &monster = player_ptr->current_floor_ptr->m_list[m_idx];
    auto &monrace = monster.get_monrace();
    auto can_do_special = monrace.ability_flags.has(MonsterAbilityType::SPECIAL);
    can_do_special &= monster.r_idx == MonraceId::OHMU;
    can_do_special &= !player_ptr->current_floor_ptr->inside_arena;
    can_do_special &= !AngbandSystem::get_instance().is_phase_out();
    can_do_special &= monrace.freq_spell != 0;
    can_do_special &= randint1(100) <= monrace.freq_spell;
    if (!can_do_special) {
        return;
    }

    int count = 0;
    DEPTH rlev = ((monrace.level >= 1) ? monrace.level : 1);
    BIT_FLAGS p_mode = monster.is_pet() ? PM_FORCE_PET : PM_NONE;

    for (int k = 0; k < A_MAX; k++) {
        if (auto summoned_m_idx = summon_specific(player_ptr, monster.fy, monster.fx, rlev, SUMMON_MOLD, (PM_ALLOW_GROUP | p_mode), m_idx)) {
            if (player_ptr->current_floor_ptr->m_list[*summoned_m_idx].ml) {
                count++;
            }
        }
    }

    if (count && is_original_ap_and_seen(player_ptr, monster)) {
        monrace.r_ability_flags.set(MonsterAbilityType::SPECIAL);
    }
}

/*!
 * @brief モンスターを分裂させるかどうかを決定する (分裂もさせる)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @param oy 分裂元モンスターのY座標
 * @param ox 分裂元モンスターのX座標
 * @return 実際に分裂したらTRUEを返す
 */
bool decide_monster_multiplication(PlayerType *player_ptr, MONSTER_IDX m_idx, POSITION oy, POSITION ox)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &monster = floor.m_list[m_idx];
    auto &monrace = monster.get_monrace();
    if (monrace.misc_flags.has_not(MonsterMiscType::MULTIPLY) || (floor.num_repro >= MAX_REPRODUCTION)) {
        return false;
    }

    auto k = 0;
    for (auto y = oy - 1; y <= oy + 1; y++) {
        for (auto x = ox - 1; x <= ox + 1; x++) {
            const Pos2D pos(y, x);
            if (!floor.contains(pos, FloorBoundary::OUTER_WALL_INCLUSIVE)) {
                continue;
            }

            if (floor.get_grid(pos).has_monster()) {
                k++;
            }
        }
    }

    if (SpellHex(player_ptr).check_hex_barrier(m_idx, HEX_ANTI_MULTI)) {
        k = 8;
    }

    constexpr auto chance_reproduction = 8;
    if ((k >= 4) || ((k > 0) && randint0(k * chance_reproduction))) {
        return false;
    }

    const auto multiplied_m_idx = multiply_monster(player_ptr, m_idx, false, (monster.is_pet() ? PM_FORCE_PET : 0));
    if (!multiplied_m_idx) {
        return false;
    }

    if (floor.m_list[*multiplied_m_idx].ml && is_original_ap_and_seen(player_ptr, monster)) {
        monrace.r_misc_flags.set(MonsterMiscType::MULTIPLY);
    }

    return true;
}

/*!
 * @brief モンスターに魔法を試行させる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @param aware モンスターがプレイヤーに気付いているならばTRUE、超隠密状態ならばFALSE
 * @return 魔法を唱えられなければ強制的にFALSE、その後モンスターが実際に魔法を唱えればTRUE
 */
bool cast_spell(PlayerType *player_ptr, MONSTER_IDX m_idx, bool aware)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &monster_from = floor.m_list[m_idx];
    const auto &monrace = monster_from.get_monrace();
    if ((monrace.freq_spell == 0) || (randint1(100) > monrace.freq_spell)) {
        return false;
    }

    auto counter_attack = false;
    if (monster_from.target_y) {
        const auto pos_to = monster_from.get_target_position();
        const auto t_m_idx = floor.get_grid(pos_to).m_idx;
        const auto &monster_to = floor.m_list[t_m_idx];
        const auto pos_from = monster_from.get_position();
        const auto is_projectable = projectable(floor, pos_from, pos_to);
        if (t_m_idx && monster_from.is_hostile_to_melee(monster_to) && is_projectable) {
            counter_attack = true;
        }
    }

    if (counter_attack) {
        if (monst_spell_monst(player_ptr, m_idx) || (aware && make_attack_spell(player_ptr, m_idx))) {
            return true;
        }
    } else {
        if ((aware && make_attack_spell(player_ptr, m_idx)) || monst_spell_monst(player_ptr, m_idx)) {
            return true;
        }
    }

    return false;
}

/*!
 * @brief モンスターの恐怖状態を処理する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_idx モンスターID
 * @param aware モンスターがプレイヤーに気付いているならばTRUE、超隠密状態ならばFALSE
 * @return モンスターが戦いを決意したらTRUE
 */
bool process_monster_fear(PlayerType *player_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx)
{
    const auto &monster = player_ptr->current_floor_ptr->m_list[m_idx];
    bool is_battle_determined = !turn_flags_ptr->do_turn && !turn_flags_ptr->do_move && monster.is_fearful() && turn_flags_ptr->aware;
    if (!is_battle_determined) {
        return false;
    }

    (void)set_monster_monfear(player_ptr, m_idx, 0);
    if (!turn_flags_ptr->see_m) {
        return true;
    }

    const auto m_name = monster_desc(player_ptr, monster, 0);
    msg_format(_("%s^は戦いを決意した！", "%s^ turns to fight!"), m_name.data());
    return true;
}

/*!
 * @brief 全モンスターのターン管理メインルーチン /
 * Process all the "live" monsters, once per game turn.
 * @details
 * During each game current game turn, we scan through the list of all the "live" monsters,\n
 * (backwards, so we can excise any "freshly dead" monsters), energizing each\n
 * monster, and allowing fully energized monsters to move, attack, pass, etc.\n
 *\n
 * Note that monsters can never move in the monster array (except when the\n
 * "compact_monsters()" function is called by "dungeon()" or "save_player()").\n
 *\n
 * This function is responsible for at least half of the processor time\n
 * on a normal system with a "normal" amount of monsters and a player doing\n
 * normal things.\n
 *\n
 * When the player is resting, virtually 90% of the processor time is spent\n
 * in this function, and its children, "process_monster()" and "make_move()".\n
 *\n
 * Most of the rest of the time is spent in "update_view()" and "lite_spot()",\n
 * especially when the player is running.\n
 *\n
 * Note the special "PRESENT_AT_TURN_START" flag.  Monsters without that flag\n
 * are "fresh" and are still being "born".  A monster is "fresh" only\n
 * during the game turn in which it is created, and we use the "hack_m_idx" to\n
 * determine if the monster is yet to be processed during the game turn.\n
 *\n
 * Note the special "PREVENT_MAGIC" flag, which allows the player to get one\n
 * move before any "nasty" monsters get to use their spell attacks.\n
 *\n
 * Note that when the "knowledge" about the currently tracked monster\n
 * changes (flags, attacks, spells), we induce a redraw of the monster\n
 * recall window.\n
 */
void process_monsters(PlayerType *player_ptr)
{
    const auto &tracker = LoreTracker::get_instance();
    const auto old_monrace_id = tracker.get_trackee();
    OldRaceFlags flags(old_monrace_id);
    player_ptr->current_floor_ptr->monster_noise = false;
    sweep_monster_process(player_ptr);
    if (!tracker.is_tracking() || !tracker.is_tracking(old_monrace_id)) {
        return;
    }

    flags.update_lore_window_flag(tracker.get_tracking_monrace());
}

/*!
 * @brief フロア内のモンスターについてターン終了時の処理を繰り返す
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void sweep_monster_process(PlayerType *player_ptr)
{
    auto &floor = *player_ptr->current_floor_ptr;

    // 処理中の召喚などで生成されたモンスターが即座に行動しないようにするため、
    // 先に現在存在するモンスターをリストアップしておく
    std::vector<MONSTER_IDX> valid_m_idx_list;
    for (MONSTER_IDX m_idx = floor.m_max - 1; m_idx >= 1; m_idx--) {
        if (floor.m_list[m_idx].is_valid()) {
            valid_m_idx_list.push_back(m_idx);
        }
    }

    for (const auto m_idx : valid_m_idx_list) {
        auto &monster = floor.m_list[m_idx];

        if (player_ptr->leaving) {
            return;
        }

        if (!monster.is_valid() || AngbandWorld::get_instance().is_wild_mode()) {
            continue;
        }

        if ((monster.cdis >= MAX_MONSTER_SENSING) || !decide_process_continue(player_ptr, monster)) {
            continue;
        }

        byte speed = monster.is_riding() ? player_ptr->pspeed : monster.get_temporary_speed();
        monster.energy_need -= speed_to_energy(speed);
        if (monster.energy_need > 0) {
            continue;
        }

        monster.energy_need += ENERGY_NEED();
        process_monster(player_ptr, m_idx);
        monster.reset_target();
        if (player_ptr->no_flowed && one_in_(3)) {
            monster.mflag2.set(MonsterConstantFlagType::NOFLOW);
        }

        if (!player_ptr->playing || player_ptr->is_dead || player_ptr->leaving) {
            return;
        }
    }
}

/*!
 * @brief 後続のモンスター処理が必要かどうか判定する (要調査)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_ptr モンスターへの参照ポインタ
 * @return 後続処理が必要ならTRUE
 */
bool decide_process_continue(PlayerType *player_ptr, MonsterEntity &monster)
{
    const auto &monrace = monster.get_monrace();
    if (!player_ptr->no_flowed) {
        monster.mflag2.reset(MonsterConstantFlagType::NOFLOW);
    }

    if (monster.cdis <= (monster.is_pet() ? (monrace.aaf > MAX_PLAYER_SIGHT ? MAX_PLAYER_SIGHT : monrace.aaf) : monrace.aaf)) {
        return true;
    }

    auto should_continue = (monster.cdis <= MAX_PLAYER_SIGHT) || AngbandSystem::get_instance().is_phase_out();
    should_continue &= player_ptr->current_floor_ptr->has_los_at({ monster.fy, monster.fx }) || has_aggravate(player_ptr);
    if (should_continue) {
        return true;
    }

    if (monster.target_y) {
        return true;
    }

    return false;
}

bool process_stalking(PlayerType *player_ptr, MONSTER_IDX m_idx)
{
    auto &monster = player_ptr->current_floor_ptr->m_list[m_idx];
    const auto &monrace = monster.get_monrace();

    // モンスターが背後に忍び寄るフラグを持っていないなら何もしない
    if (monrace.misc_flags.has_not(MonsterMiscType::STALKER)) {
        return false;
    }

    // モンスターからプレイヤーの距離が空いているなら何もしない
    if (monster.cdis > STALKER_DISTANCE_THRESHOLD) {
        return false;
    }

    // モンスターが恐怖しているならば寄ってこない
    if (monster.is_fearful()) {
        return false;
    }

    // 確率で寄ってくる
    if (!one_in_(STALKER_CHANCE_DENOMINATOR)) {
        return false;
    }

    const auto m_name = monster_name(player_ptr, m_idx);

    // 呪術魔法によりテレポートが阻害されているならば寄ってこない
    if (SpellHex(player_ptr).check_hex_barrier(m_idx, HEX_ANTI_TELE)) {
        if (see_monster(player_ptr, m_idx)) {
            msg_format(_("魔法のバリアが%s^のテレポートを邪魔した。", "Magic barrier obstructs teleporting of %s^."), m_name.data());
        }
        return false;
    }

    teleport_monster_to(player_ptr, m_idx, player_ptr->y, player_ptr->x, 100, TELEPORT_SPONTANEOUS);

    disturb(player_ptr, true, true);

    if (see_monster(player_ptr, m_idx)) {
        const auto message_stalker = monrace.get_message(m_name, MonsterMessageType::MESSAGE_STALKER);
        if (message_stalker) {
            msg_print(*message_stalker);
        }
    }

    return true;
}
