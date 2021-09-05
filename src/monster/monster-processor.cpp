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
#include "core/player-update-types.h"
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
#include "player/player-move.h"
#include "player/player-skill.h"
#include "player/player-status-flags.h"
#include "player/special-defense-types.h"
#include "spell-realm/spells-hex.h"
#include "spell/summon-types.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "view/display-messages.h"

void decide_drop_from_monster(player_type *target_ptr, MONSTER_IDX m_idx, bool is_riding_mon);
bool process_stealth(player_type *target_ptr, MONSTER_IDX m_idx);
bool vanish_summoned_children(player_type *target_ptr, MONSTER_IDX m_idx, bool see_m);
bool awake_monster(player_type *target_ptr, MONSTER_IDX m_idx);
void process_angar(player_type *target_ptr, MONSTER_IDX m_idx, bool see_m);
bool explode_grenade(player_type *target_ptr, MONSTER_IDX m_idx);
bool decide_monster_multiplication(player_type *target_ptr, MONSTER_IDX m_idx, POSITION oy, POSITION ox);
void process_special(player_type *target_ptr, MONSTER_IDX m_idx);
bool cast_spell(player_type *target_ptr, MONSTER_IDX m_idx, bool aware);

bool process_monster_fear(player_type *target_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx);

void sweep_monster_process(player_type *target_ptr);
bool decide_process_continue(player_type *target_ptr, monster_type *m_ptr);

/*!
 * @brief モンスター単体の1ターン行動処理メインルーチン /
 * Process a monster
 * @param target_ptr プレーヤーへの参照ポインタ
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
void process_monster(player_type *target_ptr, MONSTER_IDX m_idx)
{
    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    turn_flags tmp_flags;
    turn_flags *turn_flags_ptr = init_turn_flags(target_ptr->riding, m_idx, &tmp_flags);
    turn_flags_ptr->see_m = is_seen(target_ptr, m_ptr);

    decide_drop_from_monster(target_ptr, m_idx, turn_flags_ptr->is_riding_mon);
    if (m_ptr->mflag2.has(MFLAG2::CHAMELEON) && one_in_(13) && !monster_csleep_remaining(m_ptr)) {
        choose_new_monster(target_ptr, m_idx, false, 0);
        r_ptr = &r_info[m_ptr->r_idx];
    }

    turn_flags_ptr->aware = process_stealth(target_ptr, m_idx);
    if (vanish_summoned_children(target_ptr, m_idx, turn_flags_ptr->see_m) || process_quantum_effect(target_ptr, m_idx, turn_flags_ptr->see_m)
        || explode_grenade(target_ptr, m_idx) || runaway_monster(target_ptr, turn_flags_ptr, m_idx) || !awake_monster(target_ptr, m_idx))
        return;

    if (monster_stunned_remaining(m_ptr) && one_in_(2))
        return;

    if (turn_flags_ptr->is_riding_mon)
        target_ptr->update |= PU_BONUS;

    process_angar(target_ptr, m_idx, turn_flags_ptr->see_m);

    POSITION oy = m_ptr->fy;
    POSITION ox = m_ptr->fx;
    if (decide_monster_multiplication(target_ptr, m_idx, oy, ox))
        return;

    process_special(target_ptr, m_idx);
    process_speak_sound(target_ptr, m_idx, oy, ox, turn_flags_ptr->aware);
    if (cast_spell(target_ptr, m_idx, turn_flags_ptr->aware))
        return;

    DIRECTION mm[8];
    mm[0] = mm[1] = mm[2] = mm[3] = 0;
    mm[4] = mm[5] = mm[6] = mm[7] = 0;

    if (!decide_monster_movement_direction(target_ptr, mm, m_idx, turn_flags_ptr->aware))
        return;

    int count = 0;
    if (!process_monster_movement(target_ptr, turn_flags_ptr, m_idx, mm, oy, ox, &count))
        return;

    /*
     *  Forward movements failed, but now received LOS attack!
     *  Try to flow by smell.
     */
    if (target_ptr->no_flowed && count > 2 && m_ptr->target_y)
        m_ptr->mflag2.reset(MFLAG2::NOFLOW);

    if (!turn_flags_ptr->do_turn && !turn_flags_ptr->do_move && !monster_fear_remaining(m_ptr) && !turn_flags_ptr->is_riding_mon && turn_flags_ptr->aware) {
        if (r_ptr->freq_spell && randint1(100) <= r_ptr->freq_spell) {
            if (make_attack_spell(target_ptr, m_idx))
                return;
        }
    }

    update_player_type(target_ptr, turn_flags_ptr, r_ptr);
    update_monster_race_flags(target_ptr, turn_flags_ptr, m_ptr);

    if (!process_monster_fear(target_ptr, turn_flags_ptr, m_idx))
        return;

    if (m_ptr->ml)
        chg_virtue(target_ptr, V_COMPASSION, -1);
}

/*!
 * @brief 超隠密処理
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @return モンスターがプレーヤーに気付いているならばTRUE、超隠密状態ならばFALSE
 */
bool process_stealth(player_type *target_ptr, MONSTER_IDX m_idx)
{
    if ((target_ptr->special_defense & NINJA_S_STEALTH) == 0)
        return true;

    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    int tmp = target_ptr->lev * 6 + (target_ptr->skill_stl + 10) * 4;
    if (target_ptr->monlite)
        tmp /= 3;

    if (has_aggravate(target_ptr))
        tmp /= 2;

    if (r_ptr->level > (target_ptr->lev * target_ptr->lev / 20 + 10))
        tmp /= 3;

    return (randint0(tmp) <= (r_ptr->level + 20));
}

/*!
 * @brief 死亡したモンスターが乗馬中のモンスターだった場合に落馬処理を行う
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @param is_riding_mon 騎乗中であればTRUE
 */
void decide_drop_from_monster(player_type *target_ptr, MONSTER_IDX m_idx, bool is_riding_mon)
{
    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    if (!is_riding_mon || ((r_ptr->flags7 & RF7_RIDING) != 0))
        return;

    if (process_fall_off_horse(target_ptr, 0, true)) {
#ifdef JP
        msg_print("地面に落とされた。");
#else
        GAME_TEXT m_name[MAX_NLEN];
        monster_desc(target_ptr, m_name, &target_ptr->current_floor_ptr->m_list[target_ptr->riding], 0);
        msg_format("You have fallen from %s.", m_name);
#endif
    }
}

/*!
 * @brief 召喚の親元が消滅した時、子供も消滅させる
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @param see_m モンスターが視界内にいたらTRUE
 * @return 召喚モンスターが消滅したらTRUE
 */
bool vanish_summoned_children(player_type *target_ptr, MONSTER_IDX m_idx, bool see_m)
{
    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];

    if (m_ptr->parent_m_idx == 0)
        return false;

    // parent_m_idxが自分自身を指している場合は召喚主は消滅している
    if (m_ptr->parent_m_idx != m_idx && (target_ptr->current_floor_ptr->m_list[m_ptr->parent_m_idx].r_idx > 0))
        return false;

    if (see_m) {
        GAME_TEXT m_name[MAX_NLEN];
        monster_desc(target_ptr, m_name, m_ptr, 0);
        msg_format(_("%sは消え去った！", "%^s disappears!"), m_name);
    }

    if (record_named_pet && is_pet(m_ptr) && m_ptr->nickname) {
        GAME_TEXT m_name[MAX_NLEN];
        monster_desc(target_ptr, m_name, m_ptr, MD_INDEF_VISIBLE);
        exe_write_diary(target_ptr, DIARY_NAMED_PET, RECORD_NAMED_PET_LOSE_PARENT, m_name);
    }

    delete_monster_idx(target_ptr, m_idx);
    return true;
}

/*!
 * @brief 寝ているモンスターの起床を判定する
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @return 寝たままならFALSE、起きているor起きたらTRUE
 */
bool awake_monster(player_type *target_ptr, MONSTER_IDX m_idx)
{
    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    if (!monster_csleep_remaining(m_ptr))
        return true;

    if (!has_aggravate(target_ptr))
        return false;

    (void)set_monster_csleep(target_ptr, m_idx, 0);
    if (m_ptr->ml) {
        GAME_TEXT m_name[MAX_NLEN];
        monster_desc(target_ptr, m_name, m_ptr, 0);
        msg_format(_("%^sが目を覚ました。", "%^s wakes up."), m_name);
    }

    if (is_original_ap_and_seen(target_ptr, m_ptr) && (r_ptr->r_wake < MAX_UCHAR))
        r_ptr->r_wake++;

    return true;
}

/*!
 * @brief モンスターの怒り状態を判定する (怒っていたら敵に回す)
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @param see_m モンスターが視界内にいたらTRUE
 */
void process_angar(player_type *target_ptr, MONSTER_IDX m_idx, bool see_m)
{
    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    bool gets_angry = false;
    if (is_friendly(m_ptr) && has_aggravate(target_ptr))
        gets_angry = true;

    if (is_pet(m_ptr)
        && ((((r_ptr->flags1 & RF1_UNIQUE) || (r_ptr->flags7 & RF7_NAZGUL)) && monster_has_hostile_align(target_ptr, nullptr, 10, -10, r_ptr))
            || (r_ptr->flagsr & RFR_RES_ALL)))
        gets_angry = true;

    if (target_ptr->phase_out || !gets_angry)
        return;

    GAME_TEXT m_name[MAX_NLEN];
    monster_desc(target_ptr, m_name, m_ptr, is_pet(m_ptr) ? MD_ASSUME_VISIBLE : 0);

    /* When riding a hostile alignment pet */
    if (target_ptr->riding == m_idx) {
        if (abs(target_ptr->alignment / 10) < randint0(target_ptr->skill_exp[SKILL_RIDING]))
            return;

        msg_format(_("%^sが突然暴れだした！", "%^s suddenly begins unruly!"), m_name);
        if (!process_fall_off_horse(target_ptr, 1, true))
            return;

        msg_format(_("あなたは振り落とされた。", "You have fallen."));
    }

    if (is_pet(m_ptr) || see_m) {
        msg_format(_("%^sは突然敵にまわった！", "%^s suddenly becomes hostile!"), m_name);
    }

    set_hostile(target_ptr, m_ptr);
}

/*!
 * @brief 手榴弾の爆発処理
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @return 爆死したらTRUE
 */
bool explode_grenade(player_type *target_ptr, MONSTER_IDX m_idx)
{
    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    if (m_ptr->r_idx != MON_GRENADE)
        return false;

    bool fear, dead;
    mon_take_hit_mon(target_ptr, m_idx, 1, &dead, &fear, _("は爆発して粉々になった。", " explodes into tiny shreds."), m_idx);
    return dead;
}

/*!
 * @brief モンスター依存の特別な行動を取らせる
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスターID
 */
void process_special(player_type *target_ptr, MONSTER_IDX m_idx)
{
    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    if (r_ptr->ability_flags.has_not(RF_ABILITY::SPECIAL) || (m_ptr->r_idx != MON_OHMU) || target_ptr->current_floor_ptr->inside_arena || target_ptr->phase_out
        || (r_ptr->freq_spell == 0) || (randint1(100) > r_ptr->freq_spell))
        return;

    int count = 0;
    DEPTH rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);
    BIT_FLAGS p_mode = is_pet(m_ptr) ? PM_FORCE_PET : PM_NONE;

    for (int k = 0; k < A_MAX; k++) {
        if (summon_specific(target_ptr, m_idx, m_ptr->fy, m_ptr->fx, rlev, SUMMON_MOLD, (PM_ALLOW_GROUP | p_mode))) {
            if (target_ptr->current_floor_ptr->m_list[hack_m_idx_ii].ml)
                count++;
        }
    }

    if (count && is_original_ap_and_seen(target_ptr, m_ptr))
        r_ptr->r_ability_flags.set(RF_ABILITY::SPECIAL);
}

/*!
 * @brief モンスターを分裂させるかどうかを決定する (分裂もさせる)
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @param oy 分裂元モンスターのY座標
 * @param ox 分裂元モンスターのX座標
 * @return 実際に分裂したらTRUEを返す
 */
bool decide_monster_multiplication(player_type *target_ptr, MONSTER_IDX m_idx, POSITION oy, POSITION ox)
{
    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    if (((r_ptr->flags2 & RF2_MULTIPLY) == 0) || (target_ptr->current_floor_ptr->num_repro >= MAX_REPRO))
        return false;

    int k = 0;
    for (POSITION y = oy - 1; y <= oy + 1; y++) {
        for (POSITION x = ox - 1; x <= ox + 1; x++) {
            if (!in_bounds2(target_ptr->current_floor_ptr, y, x))
                continue;

            if (target_ptr->current_floor_ptr->grid_array[y][x].m_idx)
                k++;
        }
    }

    if (multiply_barrier(target_ptr, m_idx))
        k = 8;

    if ((k < 4) && (!k || !randint0(k * MON_MULT_ADJ))) {
        if (multiply_monster(target_ptr, m_idx, false, (is_pet(m_ptr) ? PM_FORCE_PET : 0))) {
            if (target_ptr->current_floor_ptr->m_list[hack_m_idx_ii].ml && is_original_ap_and_seen(target_ptr, m_ptr))
                r_ptr->r_flags2 |= RF2_MULTIPLY;

            return true;
        }
    }

    return false;
}

/*!
 * @brief モンスターに魔法を試行させる
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @param aware モンスターがプレーヤーに気付いているならばTRUE、超隠密状態ならばFALSE
 * @return 魔法を唱えられなければ強制的にFALSE、その後モンスターが実際に魔法を唱えればTRUE
 */
bool cast_spell(player_type *target_ptr, MONSTER_IDX m_idx, bool aware)
{
    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    if ((r_ptr->freq_spell == 0) || (randint1(100) > r_ptr->freq_spell))
        return false;

    bool counterattack = false;
    if (m_ptr->target_y) {
        MONSTER_IDX t_m_idx = target_ptr->current_floor_ptr->grid_array[m_ptr->target_y][m_ptr->target_x].m_idx;
        if (t_m_idx && are_enemies(target_ptr, m_ptr, &target_ptr->current_floor_ptr->m_list[t_m_idx])
            && projectable(target_ptr, m_ptr->fy, m_ptr->fx, m_ptr->target_y, m_ptr->target_x)) {
            counterattack = true;
        }
    }

    if (counterattack) {
        if (monst_spell_monst(target_ptr, m_idx) || (aware && make_attack_spell(target_ptr, m_idx)))
            return true;
    } else {
        if ((aware && make_attack_spell(target_ptr, m_idx)) || monst_spell_monst(target_ptr, m_idx))
            return true;
    }

    return false;
}

/*!
 * @brief モンスターの恐怖状態を処理する
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_idx モンスターID
 * @param aware モンスターがプレーヤーに気付いているならばTRUE、超隠密状態ならばFALSE
 * @return モンスターが戦いを決意したらTRUE
 */
bool process_monster_fear(player_type *target_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx)
{
    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    bool is_battle_determined = !turn_flags_ptr->do_turn && !turn_flags_ptr->do_move && monster_fear_remaining(m_ptr) && turn_flags_ptr->aware;
    if (!is_battle_determined)
        return false;

    (void)set_monster_monfear(target_ptr, m_idx, 0);
    if (!turn_flags_ptr->see_m)
        return true;

    GAME_TEXT m_name[MAX_NLEN];
    monster_desc(target_ptr, m_name, m_ptr, 0);
    msg_format(_("%^sは戦いを決意した！", "%^s turns to fight!"), m_name);
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
void process_monsters(player_type *target_ptr)
{
    old_race_flags tmp_flags;
    old_race_flags *old_race_flags_ptr = init_old_race_flags(&tmp_flags);
    target_ptr->current_floor_ptr->monster_noise = false;
    MONRACE_IDX old_monster_race_idx = target_ptr->monster_race_idx;
    save_old_race_flags(target_ptr->monster_race_idx, old_race_flags_ptr);
    sweep_monster_process(target_ptr);
    hack_m_idx = 0;
    if (!target_ptr->monster_race_idx || (target_ptr->monster_race_idx != old_monster_race_idx))
        return;

    update_player_window(target_ptr, old_race_flags_ptr);
}

/*!
 * @brief フロア内のモンスターについてターン終了時の処理を繰り返す
 * @param target_ptr プレーヤーへの参照ポインタ
 */
void sweep_monster_process(player_type *target_ptr)
{
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    for (MONSTER_IDX i = floor_ptr->m_max - 1; i >= 1; i--) {
        monster_type *m_ptr;
        m_ptr = &floor_ptr->m_list[i];

        if (target_ptr->leaving)
            return;

        if (!monster_is_valid(m_ptr) || target_ptr->wild_mode)
            continue;

        if (m_ptr->mflag.has(MFLAG::BORN)) {
            m_ptr->mflag.reset(MFLAG::BORN);
            continue;
        }

        if ((m_ptr->cdis >= AAF_LIMIT) || !decide_process_continue(target_ptr, m_ptr))
            continue;

        SPEED speed = (target_ptr->riding == i) ? target_ptr->pspeed : decide_monster_speed(m_ptr);
        m_ptr->energy_need -= SPEED_TO_ENERGY(speed);
        if (m_ptr->energy_need > 0)
            continue;

        m_ptr->energy_need += ENERGY_NEED();
        hack_m_idx = i;
        process_monster(target_ptr, i);
        reset_target(m_ptr);
        if (target_ptr->no_flowed && one_in_(3))
            m_ptr->mflag2.set(MFLAG2::NOFLOW);

        if (!target_ptr->playing || target_ptr->is_dead || target_ptr->leaving)
            return;
    }
}

/*!
 * @brief 後続のモンスター処理が必要かどうか判定する (要調査)
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_ptr モンスターへの参照ポインタ
 * @return 後続処理が必要ならTRUE
 */
bool decide_process_continue(player_type *target_ptr, monster_type *m_ptr)
{
    monster_race *r_ptr;
    r_ptr = &r_info[m_ptr->r_idx];
    if (!target_ptr->no_flowed) {
        m_ptr->mflag2.reset(MFLAG2::NOFLOW);
    }

    if (m_ptr->cdis <= (is_pet(m_ptr) ? (r_ptr->aaf > MAX_SIGHT ? MAX_SIGHT : r_ptr->aaf) : r_ptr->aaf))
        return true;

    if ((m_ptr->cdis <= MAX_SIGHT || target_ptr->phase_out) && (player_has_los_bold(target_ptr, m_ptr->fy, m_ptr->fx) || has_aggravate(target_ptr)))
        return true;

    if (m_ptr->target_y)
        return true;

    return false;
}
