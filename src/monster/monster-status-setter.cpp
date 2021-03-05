﻿#include "monster/monster-status-setter.h"
#include "cmd-visual/cmd-draw.h"
#include "core/player-update-types.h"
#include "core/player-redraw-types.h"
#include "core/speed-table.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "dungeon/quest-completion-checker.h"
#include "floor/cave.h"
#include "monster-floor/monster-move.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-processor.h"
#include "monster/monster-status.h" // todo 相互依存. 後で何とかする.
#include "monster/monster-util.h"
#include "monster/smart-learn-types.h"
#include "player-info/avatar.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"
#include "target/projection-path-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief モンスターをペットにする
 * @param player_type プレーヤーへの参照ポインタ
 * @param m_ptr モンスター情報構造体の参照ポインタ
 * @return なし
 */
void set_pet(player_type *player_ptr, monster_type *m_ptr)
{
    check_quest_completion(player_ptr, m_ptr);
    m_ptr->smart |= SM_PET;
    if (!(r_info[m_ptr->r_idx].flags3 & (RF3_EVIL | RF3_GOOD)))
        m_ptr->sub_align = SUB_ALIGN_NEUTRAL;
}

/*!
 * @brief モンスターを敵に回す
 * Makes the monster hostile towards the player
 * @param m_ptr モンスター情報構造体の参照ポインタ
 * @return なし
 */
void set_hostile(player_type *player_ptr, monster_type *m_ptr)
{
    if (player_ptr->phase_out)
        return;

    m_ptr->smart &= ~SM_PET;
    m_ptr->smart &= ~SM_FRIENDLY;
}

/*!
 * @brief モンスターを怒らせる
 * Anger the monster
 * @param m_ptr モンスター情報構造体の参照ポインタ
 * @return なし
 */
void anger_monster(player_type *player_ptr, monster_type *m_ptr)
{
    if (player_ptr->phase_out || !is_friendly(m_ptr))
        return;

    GAME_TEXT m_name[MAX_NLEN];
    monster_desc(player_ptr, m_name, m_ptr, 0);
    msg_format(_("%^sは怒った！", "%^s gets angry!"), m_name);
    set_hostile(player_ptr, m_ptr);
    chg_virtue(player_ptr, V_INDIVIDUALISM, 1);
    chg_virtue(player_ptr, V_HONOUR, -1);
    chg_virtue(player_ptr, V_JUSTICE, -1);
    chg_virtue(player_ptr, V_COMPASSION, -1);
}

/*!
 * @brief モンスターの時限ステータスリストを削除
 * @param floor_ptr 現在フロアへの参照ポインタ
 * @return m_idx モンスターの参照ID
 * @return mproc_type 削除したいモンスターの時限ステータスID
 * @return なし
 */
static void mproc_remove(floor_type *floor_ptr, MONSTER_IDX m_idx, int mproc_type)
{
    int mproc_idx = get_mproc_idx(floor_ptr, m_idx, mproc_type);
    if (mproc_idx >= 0)
        floor_ptr->mproc_list[mproc_type][mproc_idx] = floor_ptr->mproc_list[mproc_type][--floor_ptr->mproc_max[mproc_type]];
}

/*!
 * @brief モンスターの睡眠状態値をセットする。0で起きる。 /
 * Set "m_ptr->mtimed[MTIMED_CSLEEP]", notice observable changes
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスター参照ID
 * @param v セットする値
 * @return 別途更新処理が必要な場合TRUEを返す
 */
bool set_monster_csleep(player_type *target_ptr, MONSTER_IDX m_idx, int v)
{
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;
    if (v) {
        if (!monster_csleep_remaining(m_ptr)) {
            mproc_add(floor_ptr, m_idx, MTIMED_CSLEEP);
            notice = TRUE;
        }
    } else {
        if (monster_csleep_remaining(m_ptr)) {
            mproc_remove(floor_ptr, m_idx, MTIMED_CSLEEP);
            notice = TRUE;
        }
    }

    m_ptr->mtimed[MTIMED_CSLEEP] = (s16b)v;
    if (!notice)
        return FALSE;

    if (m_ptr->ml) {
        if (target_ptr->health_who == m_idx)
            target_ptr->redraw |= PR_HEALTH;

        if (target_ptr->riding == m_idx)
            target_ptr->redraw |= PR_UHEALTH;
    }

    if (r_info[m_ptr->r_idx].flags7 & RF7_HAS_LD_MASK)
        target_ptr->update |= PU_MON_LITE;

    return TRUE;
}

/*!
 * @brief モンスターの加速状態値をセット /
 * Set "m_ptr->mtimed[MTIMED_FAST]", notice observable changes
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスター参照ID
 * @param v セットする値
 * @return 別途更新処理が必要な場合TRUEを返す
 */
bool set_monster_fast(player_type *target_ptr, MONSTER_IDX m_idx, int v)
{
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    bool notice = FALSE;
    v = (v > 200) ? 200 : (v < 0) ? 0 : v;
    if (v) {
        if (!monster_fast_remaining(m_ptr)) {
            mproc_add(floor_ptr, m_idx, MTIMED_FAST);
            notice = TRUE;
        }
    } else {
        if (monster_fast_remaining(m_ptr)) {
            mproc_remove(floor_ptr, m_idx, MTIMED_FAST);
            notice = TRUE;
        }
    }

    m_ptr->mtimed[MTIMED_FAST] = (s16b)v;
    if (!notice)
        return FALSE;

    if ((target_ptr->riding == m_idx) && !target_ptr->leaving)
        target_ptr->update |= PU_BONUS;

    return TRUE;
}

/*
 * Set "m_ptr->mtimed[MTIMED_SLOW]", notice observable changes
 */
bool set_monster_slow(player_type *target_ptr, MONSTER_IDX m_idx, int v)
{
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    bool notice = FALSE;
    v = (v > 200) ? 200 : (v < 0) ? 0 : v;
    if (v) {
        if (!monster_slow_remaining(m_ptr)) {
            mproc_add(floor_ptr, m_idx, MTIMED_SLOW);
            notice = TRUE;
        }
    } else {
        if (monster_slow_remaining(m_ptr)) {
            mproc_remove(floor_ptr, m_idx, MTIMED_SLOW);
            notice = TRUE;
        }
    }

    m_ptr->mtimed[MTIMED_SLOW] = (s16b)v;
    if (!notice)
        return FALSE;

    if ((target_ptr->riding == m_idx) && !target_ptr->leaving)
        target_ptr->update |= PU_BONUS;

    return TRUE;
}

/*!
 * @brief モンスターの朦朧状態値をセット /
 * Set "m_ptr->mtimed[MTIMED_STUNNED]", notice observable changes
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスター参照ID
 * @param v セットする値
 * @return 別途更新処理が必要な場合TRUEを返す
 */
bool set_monster_stunned(player_type *target_ptr, MONSTER_IDX m_idx, int v)
{
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    bool notice = FALSE;
    v = (v > 200) ? 200 : (v < 0) ? 0 : v;
    if (v) {
        if (!monster_stunned_remaining(m_ptr)) {
            mproc_add(floor_ptr, m_idx, MTIMED_STUNNED);
            notice = TRUE;
        }
    } else {
        if (monster_stunned_remaining(m_ptr)) {
            mproc_remove(floor_ptr, m_idx, MTIMED_STUNNED);
            notice = TRUE;
        }
    }

    m_ptr->mtimed[MTIMED_STUNNED] = (s16b)v;
    return notice;
}

/*!
 * @brief モンスターの混乱状態値をセット /
 * Set "m_ptr->mtimed[MTIMED_CONFUSED]", notice observable changes
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスター参照ID
 * @param v セットする値
 * @return 別途更新処理が必要な場合TRUEを返す
 */
bool set_monster_confused(player_type *target_ptr, MONSTER_IDX m_idx, int v)
{
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    bool notice = FALSE;
    v = (v > 200) ? 200 : (v < 0) ? 0 : v;
    if (v) {
        if (!monster_confused_remaining(m_ptr)) {
            mproc_add(floor_ptr, m_idx, MTIMED_CONFUSED);
            notice = TRUE;
        }
    } else {
        if (monster_confused_remaining(m_ptr)) {
            mproc_remove(floor_ptr, m_idx, MTIMED_CONFUSED);
            notice = TRUE;
        }
    }

    m_ptr->mtimed[MTIMED_CONFUSED] = (s16b)v;
    return notice;
}

/*!
 * @brief モンスターの恐慌状態値をセット /
 * Set "m_ptr->mtimed[MTIMED_MONFEAR]", notice observable changes
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスター参照ID
 * @param v セットする値
 * @return 別途更新処理が必要な場合TRUEを返す
 */
bool set_monster_monfear(player_type *target_ptr, MONSTER_IDX m_idx, int v)
{
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    bool notice = FALSE;
    v = (v > 200) ? 200 : (v < 0) ? 0 : v;
    if (v) {
        if (!monster_fear_remaining(m_ptr)) {
            mproc_add(floor_ptr, m_idx, MTIMED_MONFEAR);
            notice = TRUE;
        }
    } else {
        if (monster_fear_remaining(m_ptr)) {
            mproc_remove(floor_ptr, m_idx, MTIMED_MONFEAR);
            notice = TRUE;
        }
    }

    m_ptr->mtimed[MTIMED_MONFEAR] = (s16b)v;

    if (!notice)
        return FALSE;

    if (m_ptr->ml) {
        if (target_ptr->health_who == m_idx)
            target_ptr->redraw |= PR_HEALTH;

        if (target_ptr->riding == m_idx)
            target_ptr->redraw |= PR_UHEALTH;
    }

    return TRUE;
}

/*!
 * @brief モンスターの無敵状態値をセット /
 * Set "m_ptr->mtimed[MTIMED_INVULNER]", notice observable changes
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスター参照ID
 * @param v セットする値
 * @param energy_need TRUEならば無敵解除時に行動ターン消費を行う
 * @return 別途更新処理が必要な場合TRUEを返す
 */
bool set_monster_invulner(player_type *target_ptr, MONSTER_IDX m_idx, int v, bool energy_need)
{
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    bool notice = FALSE;
    v = (v > 200) ? 200 : (v < 0) ? 0 : v;
    if (v) {
        if (!monster_invulner_remaining(m_ptr)) {
            mproc_add(floor_ptr, m_idx, MTIMED_INVULNER);
            notice = TRUE;
        }
    } else {
        if (monster_invulner_remaining(m_ptr)) {
            mproc_remove(floor_ptr, m_idx, MTIMED_INVULNER);
            if (energy_need && !target_ptr->wild_mode)
                m_ptr->energy_need += ENERGY_NEED();
            notice = TRUE;
        }
    }

    m_ptr->mtimed[MTIMED_INVULNER] = (s16b)v;
    if (!notice)
        return FALSE;

    if (m_ptr->ml) {
        if (target_ptr->health_who == m_idx)
            target_ptr->redraw |= PR_HEALTH;

        if (target_ptr->riding == m_idx)
            target_ptr->redraw |= PR_UHEALTH;
    }

    return TRUE;
}

/*!
 * @brief モンスターの時間停止処理
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param num 時間停止を行った敵が行動できる回数
 * @param who 時間停止処理の主体ID
 * @param vs_player TRUEならば時間停止開始処理を行う
 * @return 時間停止が行われている状態ならばTRUEを返す
 */
bool set_monster_timewalk(player_type *target_ptr, int num, MONSTER_IDX who, bool vs_player)
{
    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[hack_m_idx];
    if (current_world_ptr->timewalk_m_idx)
        return FALSE;

    if (vs_player) {
        GAME_TEXT m_name[MAX_NLEN];
        monster_desc(target_ptr, m_name, m_ptr, 0);

        if (who == 1)
            msg_format(_("「『ザ・ワールド』！時は止まった！」", "%s yells 'The World! Time has stopped!'"), m_name);
        else if (who == 3)
            msg_format(_("「時よ！」", "%s yells 'Time!'"), m_name);
        else
            msg_print("hek!");

        msg_print(NULL);
    }

    current_world_ptr->timewalk_m_idx = hack_m_idx;
    if (vs_player)
        do_cmd_redraw(target_ptr);

    while (num--) {
        if (!monster_is_valid(m_ptr))
            break;

        process_monster(target_ptr, current_world_ptr->timewalk_m_idx);
        reset_target(m_ptr);
        handle_stuff(target_ptr);
        if (vs_player)
            term_xtra(TERM_XTRA_DELAY, 500);
    }

    target_ptr->redraw |= PR_MAP;
    target_ptr->update |= PU_MONSTERS;
    target_ptr->window_flags |= PW_OVERHEAD | PW_DUNGEON;
    current_world_ptr->timewalk_m_idx = 0;
    if (vs_player || (player_has_los_bold(target_ptr, m_ptr->fy, m_ptr->fx) && projectable(target_ptr, target_ptr->y, target_ptr->x, m_ptr->fy, m_ptr->fx))) {
        msg_print(_("「時は動きだす…」", "You feel time flowing around you once more."));
        msg_print(NULL);
    }

    handle_stuff(target_ptr);
    return TRUE;
}
