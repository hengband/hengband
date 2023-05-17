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
#include "core/speed-table.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "game-option/play-record-options.h"
#include "grid/feature.h"
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
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-indice-types.h"
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
#include "object-enchant/trc-types.h"
#include "pet/pet-fall-off.h"
#include "player-base/player-class.h"
#include "player-info/ninja-data-type.h"
#include "player/player-move.h"
#include "player/player-skill.h"
#include "player/player-status-flags.h"
#include "player/special-defense-types.h"
#include "spell-realm/spells-hex.h"
#include "spell/summon-types.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/projection-path-calculator.h"
#include "view/display-messages.h"

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
bool decide_process_continue(PlayerType *player_ptr, MonsterEntity *m_ptr);

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
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    auto *r_ptr = &monraces_info[m_ptr->r_idx];
    turn_flags tmp_flags;
    turn_flags *turn_flags_ptr = init_turn_flags(player_ptr->riding, m_idx, &tmp_flags);
    turn_flags_ptr->see_m = is_seen(player_ptr, m_ptr);

    decide_drop_from_monster(player_ptr, m_idx, turn_flags_ptr->is_riding_mon);
    if (m_ptr->mflag2.has(MonsterConstantFlagType::CHAMELEON) && one_in_(13) && !m_ptr->is_asleep()) {
        choose_new_monster(player_ptr, m_idx, false, MonsterRace::empty_id());
        r_ptr = &monraces_info[m_ptr->r_idx];
    }

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

    if (m_ptr->is_stunned() && one_in_(2)) {
        return;
    }

    if (turn_flags_ptr->is_riding_mon) {
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRedrawingFlag::BONUS);
    }

    process_angar(player_ptr, m_idx, turn_flags_ptr->see_m);

    POSITION oy = m_ptr->fy;
    POSITION ox = m_ptr->fx;
    if (decide_monster_multiplication(player_ptr, m_idx, oy, ox)) {
        return;
    }

    process_special(player_ptr, m_idx);
    process_speak_sound(player_ptr, m_idx, oy, ox, turn_flags_ptr->aware);
    if (cast_spell(player_ptr, m_idx, turn_flags_ptr->aware)) {
        return;
    }

    DIRECTION mm[8];
    mm[0] = mm[1] = mm[2] = mm[3] = 0;
    mm[4] = mm[5] = mm[6] = mm[7] = 0;

    if (!decide_monster_movement_direction(player_ptr, mm, m_idx, turn_flags_ptr->aware)) {
        return;
    }

    int count = 0;
    if (!process_monster_movement(player_ptr, turn_flags_ptr, m_idx, mm, oy, ox, &count)) {
        return;
    }

    /*
     *  Forward movements failed, but now received LOS attack!
     *  Try to flow by smell.
     */
    if (player_ptr->no_flowed && count > 2 && m_ptr->target_y) {
        m_ptr->mflag2.reset(MonsterConstantFlagType::NOFLOW);
    }

    if (!turn_flags_ptr->do_turn && !turn_flags_ptr->do_move && !m_ptr->is_fearful() && !turn_flags_ptr->is_riding_mon && turn_flags_ptr->aware) {
        if (r_ptr->freq_spell && randint1(100) <= r_ptr->freq_spell) {
            if (make_attack_spell(player_ptr, m_idx)) {
                return;
            }
        }
    }

    update_player_type(player_ptr, turn_flags_ptr, r_ptr);
    update_monster_race_flags(player_ptr, turn_flags_ptr, m_ptr);

    if (!process_monster_fear(player_ptr, turn_flags_ptr, m_idx)) {
        return;
    }

    if (m_ptr->ml) {
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

    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    auto *r_ptr = &monraces_info[m_ptr->r_idx];
    int tmp = player_ptr->lev * 6 + (player_ptr->skill_stl + 10) * 4;
    if (player_ptr->monlite) {
        tmp /= 3;
    }

    if (has_aggravate(player_ptr)) {
        tmp /= 2;
    }

    if (r_ptr->level > (player_ptr->lev * player_ptr->lev / 20 + 10)) {
        tmp /= 3;
    }

    return randint0(tmp) <= (r_ptr->level + 20);
}

/*!
 * @brief 死亡したモンスターが乗馬中のモンスターだった場合に落馬処理を行う
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @param is_riding_mon 騎乗中であればTRUE
 */
void decide_drop_from_monster(PlayerType *player_ptr, MONSTER_IDX m_idx, bool is_riding_mon)
{
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    auto *r_ptr = &monraces_info[m_ptr->r_idx];
    if (!is_riding_mon || ((r_ptr->flags7 & RF7_RIDING) != 0)) {
        return;
    }

    if (process_fall_off_horse(player_ptr, 0, true)) {
#ifdef JP
        msg_print("地面に落とされた。");
#else
        const auto m_name = monster_desc(player_ptr, &player_ptr->current_floor_ptr->m_list[player_ptr->riding], 0);
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
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];

    if (m_ptr->parent_m_idx == 0) {
        return false;
    }

    // parent_m_idxが自分自身を指している場合は召喚主は消滅している
    if (m_ptr->parent_m_idx != m_idx && MonsterRace(player_ptr->current_floor_ptr->m_list[m_ptr->parent_m_idx].r_idx).is_valid()) {
        return false;
    }

    if (see_m) {
        const auto m_name = monster_desc(player_ptr, m_ptr, 0);
        msg_format(_("%sは消え去った！", "%s^ disappears!"), m_name.data());
    }

    if (record_named_pet && m_ptr->is_named_pet()) {
        const auto m_name = monster_desc(player_ptr, m_ptr, MD_INDEF_VISIBLE);
        exe_write_diary(player_ptr, DIARY_NAMED_PET, RECORD_NAMED_PET_LOSE_PARENT, m_name.data());
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
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    auto *r_ptr = &monraces_info[m_ptr->r_idx];
    if (!m_ptr->is_asleep()) {
        return true;
    }

    if (!has_aggravate(player_ptr)) {
        return false;
    }

    (void)set_monster_csleep(player_ptr, m_idx, 0);
    if (m_ptr->ml) {
        const auto m_name = monster_desc(player_ptr, m_ptr, 0);
        msg_format(_("%s^が目を覚ました。", "%s^ wakes up."), m_name.data());
    }

    if (is_original_ap_and_seen(player_ptr, m_ptr) && (r_ptr->r_wake < MAX_UCHAR)) {
        r_ptr->r_wake++;
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
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    auto *r_ptr = &monraces_info[m_ptr->r_idx];
    bool gets_angry = false;
    if (m_ptr->is_friendly() && has_aggravate(player_ptr)) {
        gets_angry = true;
    }

    if (m_ptr->is_pet() && (((r_ptr->kind_flags.has(MonsterKindType::UNIQUE) || (r_ptr->population_flags.has(MonsterPopulationType::NAZGUL))) && monster_has_hostile_align(player_ptr, nullptr, 10, -10, r_ptr)) || r_ptr->resistance_flags.has(MonsterResistanceType::RESIST_ALL))) {
        gets_angry = true;
    }

    if (player_ptr->phase_out || !gets_angry) {
        return;
    }

    const auto m_name = monster_desc(player_ptr, m_ptr, m_ptr->is_pet() ? MD_ASSUME_VISIBLE : 0);

    /* When riding a hostile alignment pet */
    if (player_ptr->riding == m_idx) {
        if (abs(player_ptr->alignment / 10) < randint0(player_ptr->skill_exp[PlayerSkillKindType::RIDING])) {
            return;
        }

        msg_format(_("%s^が突然暴れだした！", "%s^ suddenly begins unruly!"), m_name.data());
        if (!process_fall_off_horse(player_ptr, 1, true)) {
            return;
        }

        msg_format(_("あなたは振り落とされた。", "You have fallen."));
    }

    if (m_ptr->is_pet() || see_m) {
        msg_format(_("%s^は突然敵にまわった！", "%s^ suddenly becomes hostile!"), m_name.data());
    }

    set_hostile(player_ptr, m_ptr);
}

/*!
 * @brief 手榴弾の爆発処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @return 爆死したらTRUE
 */
bool explode_grenade(PlayerType *player_ptr, MONSTER_IDX m_idx)
{
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    if (m_ptr->r_idx != MonsterRaceId::GRENADE) {
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
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    auto *r_ptr = &monraces_info[m_ptr->r_idx];
    if (r_ptr->ability_flags.has_not(MonsterAbilityType::SPECIAL) || (m_ptr->r_idx != MonsterRaceId::OHMU) || player_ptr->current_floor_ptr->inside_arena || player_ptr->phase_out || (r_ptr->freq_spell == 0) || (randint1(100) > r_ptr->freq_spell)) {
        return;
    }

    int count = 0;
    DEPTH rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);
    BIT_FLAGS p_mode = m_ptr->is_pet() ? PM_FORCE_PET : PM_NONE;

    for (int k = 0; k < A_MAX; k++) {
        if (summon_specific(player_ptr, m_idx, m_ptr->fy, m_ptr->fx, rlev, SUMMON_MOLD, (PM_ALLOW_GROUP | p_mode))) {
            if (player_ptr->current_floor_ptr->m_list[hack_m_idx_ii].ml) {
                count++;
            }
        }
    }

    if (count && is_original_ap_and_seen(player_ptr, m_ptr)) {
        r_ptr->r_ability_flags.set(MonsterAbilityType::SPECIAL);
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
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    auto *r_ptr = &monraces_info[m_ptr->r_idx];
    if (((r_ptr->flags2 & RF2_MULTIPLY) == 0) || (player_ptr->current_floor_ptr->num_repro >= MAX_REPRODUCTION)) {
        return false;
    }

    int k = 0;
    for (POSITION y = oy - 1; y <= oy + 1; y++) {
        for (POSITION x = ox - 1; x <= ox + 1; x++) {
            if (!in_bounds2(player_ptr->current_floor_ptr, y, x)) {
                continue;
            }

            if (player_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
                k++;
            }
        }
    }

    if (SpellHex(player_ptr).check_hex_barrier(m_idx, HEX_ANTI_MULTI)) {
        k = 8;
    }

    constexpr auto chance_reproduction = 8;
    if ((k < 4) && (!k || !randint0(k * chance_reproduction))) {
        if (multiply_monster(player_ptr, m_idx, false, (m_ptr->is_pet() ? PM_FORCE_PET : 0))) {
            if (player_ptr->current_floor_ptr->m_list[hack_m_idx_ii].ml && is_original_ap_and_seen(player_ptr, m_ptr)) {
                r_ptr->r_flags2 |= RF2_MULTIPLY;
            }

            return true;
        }
    }

    return false;
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
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    auto *r_ptr = &monraces_info[m_ptr->r_idx];
    if ((r_ptr->freq_spell == 0) || (randint1(100) > r_ptr->freq_spell)) {
        return false;
    }

    bool counterattack = false;
    if (m_ptr->target_y) {
        MONSTER_IDX t_m_idx = player_ptr->current_floor_ptr->grid_array[m_ptr->target_y][m_ptr->target_x].m_idx;
        const auto &target_ref = player_ptr->current_floor_ptr->m_list[t_m_idx];
        const auto is_projectable = projectable(player_ptr, m_ptr->fy, m_ptr->fx, m_ptr->target_y, m_ptr->target_x);
        if (t_m_idx && are_enemies(player_ptr, *m_ptr, target_ref) && is_projectable) {
            counterattack = true;
        }
    }

    if (counterattack) {
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
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    bool is_battle_determined = !turn_flags_ptr->do_turn && !turn_flags_ptr->do_move && m_ptr->is_fearful() && turn_flags_ptr->aware;
    if (!is_battle_determined) {
        return false;
    }

    (void)set_monster_monfear(player_ptr, m_idx, 0);
    if (!turn_flags_ptr->see_m) {
        return true;
    }

    const auto m_name = monster_desc(player_ptr, m_ptr, 0);
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
 * Note the special "MFLAG_BORN" flag, which allows us to ignore "fresh"\n
 * monsters while they are still being "born".  A monster is "fresh" only\n
 * during the game turn in which it is created, and we use the "hack_m_idx" to\n
 * determine if the monster is yet to be processed during the game turn.\n
 *\n
 * Note the special "MFLAG_PREVENT_MAGIC" flag, which allows the player to get one\n
 * move before any "nasty" monsters get to use their spell attacks.\n
 *\n
 * Note that when the "knowledge" about the currently tracked monster\n
 * changes (flags, attacks, spells), we induce a redraw of the monster\n
 * recall window.\n
 */
void process_monsters(PlayerType *player_ptr)
{
    old_race_flags tmp_flags;
    old_race_flags *old_race_flags_ptr = init_old_race_flags(&tmp_flags);
    player_ptr->current_floor_ptr->monster_noise = false;
    MonsterRaceId old_monster_race_idx = player_ptr->monster_race_idx;
    save_old_race_flags(player_ptr->monster_race_idx, old_race_flags_ptr);
    sweep_monster_process(player_ptr);
    hack_m_idx = 0;
    if (!MonsterRace(player_ptr->monster_race_idx).is_valid() || (player_ptr->monster_race_idx != old_monster_race_idx)) {
        return;
    }

    update_player_window(player_ptr, old_race_flags_ptr);
}

/*!
 * @brief フロア内のモンスターについてターン終了時の処理を繰り返す
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void sweep_monster_process(PlayerType *player_ptr)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    for (MONSTER_IDX i = floor_ptr->m_max - 1; i >= 1; i--) {
        MonsterEntity *m_ptr;
        m_ptr = &floor_ptr->m_list[i];

        if (player_ptr->leaving) {
            return;
        }

        if (!m_ptr->is_valid() || player_ptr->wild_mode) {
            continue;
        }

        if (m_ptr->mflag.has(MonsterTemporaryFlagType::BORN)) {
            m_ptr->mflag.reset(MonsterTemporaryFlagType::BORN);
            continue;
        }

        if ((m_ptr->cdis >= MAX_MONSTER_SENSING) || !decide_process_continue(player_ptr, m_ptr)) {
            continue;
        }

        byte speed = (player_ptr->riding == i) ? player_ptr->pspeed : m_ptr->get_temporary_speed();
        m_ptr->energy_need -= speed_to_energy(speed);
        if (m_ptr->energy_need > 0) {
            continue;
        }

        m_ptr->energy_need += ENERGY_NEED();
        hack_m_idx = i;
        process_monster(player_ptr, i);
        reset_target(m_ptr);
        if (player_ptr->no_flowed && one_in_(3)) {
            m_ptr->mflag2.set(MonsterConstantFlagType::NOFLOW);
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
bool decide_process_continue(PlayerType *player_ptr, MonsterEntity *m_ptr)
{
    MonsterRaceInfo *r_ptr;
    r_ptr = &monraces_info[m_ptr->r_idx];
    if (!player_ptr->no_flowed) {
        m_ptr->mflag2.reset(MonsterConstantFlagType::NOFLOW);
    }

    if (m_ptr->cdis <= (m_ptr->is_pet() ? (r_ptr->aaf > MAX_PLAYER_SIGHT ? MAX_PLAYER_SIGHT : r_ptr->aaf) : r_ptr->aaf)) {
        return true;
    }

    if ((m_ptr->cdis <= MAX_PLAYER_SIGHT || player_ptr->phase_out) && (player_has_los_bold(player_ptr, m_ptr->fy, m_ptr->fx) || has_aggravate(player_ptr))) {
        return true;
    }

    if (m_ptr->target_y) {
        return true;
    }

    return false;
}
